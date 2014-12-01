#include "SuperResolution.h"
#include "SuperResolution.decl.h"



/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_PatchArray arrayProxy;
/*readonly*/ CProxy_DBNode dbProxy;
/*readonly*/ int arrayXDim;
/*readonly*/ int arrayYDim;

class Main: public CBase_Main {
public:
    Main(CkArgMsg *m) {
        //print usage details
        if(m->argc != 3)
        {
            CkPrintf("Usage: ./charmrun +p4 ./superRes <db folder path> <input image path>");
            CkExit();
        }

        mainProxy = thisProxy;

        //get the training set directory
        const string sDBFolderPath = m->argv[1];
        //get the input image path
        const string sInputImagePath = m->argv[2];
        //construct node group
        dbProxy = CProxy_DBNode::ckNew();
        dbProxy.FillDB(sDBFolderPath);
        //construct patcharray
        // placeholder dimensions
        arrayXDim = 10;
        arrayYDim = 10;
        CkArrayOptions opts(arrayXDim, arrayYDim);
        arrayProxy = CProxy_PatchArray::ckNew(opts);

    }


    void CheckConverged(double norm) {
        if (norm < TOL) {
            arrayProxy.GetFinalPatch();
        }
        else {
            arrayProxy.Run();
        }
    }

    void RecvFinalPatch(CkReductionMsg *msg) {
        patch_t *p = (patch_t*)msg->getData();
        int num_elements = msg->getSize() / sizeof(patch_t);
    }

    void DB_Populated()
    {
        arrayProxy.Setup();
    }
};

class DBNode: public CBase_DBNode
{
private:
    ImageDB db;
public:
    DBNode()
    {
    }

    int FillDB(const string &sTrainingSetDirPath)
    {
        vector<string> sFiles;
        if(GetFilesFromDir(sTrainingSetDirPath,sFiles) == FAIL)
        {
            CkExit();
        }

        //sort the file names so that they are read in the same sequence
        //in all the nodes.
        sort(sFiles.begin(),sFiles.end());

        for(vector<string>::iterator itr = sFiles.begin(); itr != sFiles.end(); ++itr)
        {

            ifstream imageFile;
            imageFile.open(itr->c_str());
            int totalPatchPerFile,lowResSize,highResSize;

            /* open the file, read the meta data(total patches,
             * lowRes size and highRes size).
             * Then read each line and fill the data base.
             * Each line has both lowRes and highRes data.
             * */
            if (imageFile.is_open()) {

                imageFile >> totalPatchPerFile;
                imageFile >> lowResSize;
                imageFile >> highResSize;

                //check if all files in the training set have same patch size
                if((db.nImageFiles > 0 && (db.lowResSize != lowResSize || db.highResSize != highResSize)))
                {
                    cout << "ERROR: DB input patch size don't match across inputs" << endl;
                    return FAIL;
                }

                if(db.nImageFiles == 0)
                {
                    db.lowResSize = lowResSize;
                    db.highResSize = highResSize;
                }

                db.nImageFiles++;
                for (int i=0; i < totalPatchPerFile; ++i)
                {
                    Patch pLowRes  = new double[db.lowResSize];
                    Patch pHighRes = new double[db.highResSize];
                    for (int lowResIndex = 0; lowResIndex < db.lowResSize; ++lowResIndex)
                    {
                        imageFile >> pLowRes[lowResIndex];

                    }
                    for (int highResIndex = 0; highResIndex < db.highResSize; ++highResIndex)
                    {
                        imageFile >> pHighRes[highResIndex];
                    }
                    CandidatePair pair = make_pair(pLowRes,pHighRes);
                    db.imageData.push_back(pair);
                }
            }
        }

        contribute(CkCallback(CkReductionTarget(Main, DB_Populated), mainProxy));
        return SUCCESS;

    }

    ImageDB *GetImageDB()
    {
        return &db;
    }

    void PrintDB()
    {
        cout << "=================DB Begin==================" << endl;
        cout << db.lowResSize << " " << db.highResSize << endl;
        for(DBIter itr = db.imageData.begin(); itr != db.imageData.end(); ++itr)
        {
            CandidatePair &dbItem = *itr;
            for(int i =0; i<db.lowResSize; ++i)
                cout << dbItem.first[i] << " ";
            for(int i=0; i<db.highResSize; ++i)
                cout << dbItem.second[i] << " ";
            cout << endl;
        }
        cout << "=================DB End====================" << endl;
    }


};



class PatchArray: public CBase_PatchArray {
    PatchArray_SDAG_CODE
private:
    vector<PatchID> myCandidates;
    vector< vector<PatchID> > neighCandidates;
    vector< vector<double> >  in_msgs, out_msgs;
    vector<double> beliefs;
    std::vector<double> myphis;
    int i, j;
    Patch myPatch;
    ImageDB *DB;
    int iter;

public:
    PatchArray() {
        __sdag_init();

        i = thisIndex.x;
        j = thisIndex.y;

        neighCandidates.resize(4);
        out_msgs.resize(4);
        in_msgs.resize(4);

        //get pointer to the db from the node group
        DB = dbProxy.ckLocalBranch()->GetImageDB();
    }
    PatchArray(CkMigrateMessage *msg) {}

    void SetupPatch()
    {
        /*
        Chooses the best candidate patches and calculates the
        corresponding phi values.
        */
        int myplace, myindex;
        double mydist;
        bool changedVec; 
        int index[CANDIDATE_COUNT];  // Unordered indices of best candidates
        double distance[CANDIDATE_COUNT];  // Distance from me to index[i]
        int order[CANDIDATE_COUNT];  // Order of index values

        // Fill with the first CANDIDATE_COUNT
        for (int i = 0; i < CANDIDATE_COUNT; i++) {
            Patch dbPatch = DB->GetLowResPatch(i);
            index[i] = i;
            distance[i] = phi(myPatch,dbPatch,DB->lowResSize);
            myplace = 0;
            for (int j = 0; j < i; j++) {
                if (distance[j] < distance[i]) {
                    myplace++;
                } else {
                    order[j]++;
                }
            }
        }

        // Go through the rest of the DB, booting out
        // the worst candidate each time a new one
        // is found.
        for (int i = CANDIDATE_COUNT; i < DB->imageData.size(); i++) {
            Patch dbPatch = DB->GetLowResPatch(i);
            mydist = phi(myPatch,dbPatch,DB->lowResSize);
            myplace = 0;
            for (int j = 0; j < CANDIDATE_COUNT; j++) {
                if (distance[j] < mydist) {
                    myplace++;
                } else {
                    order[j]++;
                    if (order[j] == CANDIDATE_COUNT) {
                        myindex = j;
                        changedVec = true;
                    }
                }
            }
            if (changedVec) {
                index[myindex] = i;
                distance[myindex] = mydist;
                order[myindex] = myplace;
            }
        }

        for (int i = 0; i < CANDIDATE_COUNT; i++) {
            myCandidates.push_back(index[i]);
            myphis.push_back(distance[i]);
        }

    }

    void SendPatchesToNeighbors() {
        // EAST
        if (j > 0)
            thisProxy(i, j-1).RecvCandidatesFromNeighbors(EAST, myCandidates);

        // WEST
        if (j < arrayYDim-1)
            thisProxy(i, j+1).RecvCandidatesFromNeighbors(WEST, myCandidates);

        // SOUTH
        if (i > 0)
            thisProxy(i-1, j).RecvCandidatesFromNeighbors(SOUTH, myCandidates);

        // NORTH
        if (i < arrayXDim-1)
            thisProxy(i+1, j).RecvCandidatesFromNeighbors(NORTH, myCandidates);
    }

    void ProcessCandidates(int from, vector<PatchID> patches) {
        neighCandidates[from] = patches;
    }


    void ComputeMessages() {
        // WEST
        if (j > 0)
            for (int i = 0; i < myCandidates.size(); ++i)
                for (int j = 0; j < out_msgs[WEST].size(); ++j)
                    out_msgs[WEST][j] += myphis[i] * psi(myCandidates[i], neighCandidates[WEST][j],WEST) * (in_msgs[EAST][i]
                    * in_msgs[SOUTH][i] * in_msgs[NORTH][i]);

        // EAST
        if (j < arrayYDim-1)

            for (int i = 0; i < myCandidates.size(); ++i)
                for (int j = 0; j < out_msgs[EAST].size(); ++j)
                    out_msgs[EAST][j] += myphis[i] * psi(myCandidates[i], neighCandidates[EAST][j],EAST) *
                    (in_msgs[NORTH][i] * in_msgs[SOUTH][i] * in_msgs[WEST][i]);

        // NORTH
        if (i > 0)
            for (int i = 0; i < myCandidates.size(); ++i)
                for (int j = 0; j < out_msgs[NORTH].size(); ++j)
                    out_msgs[NORTH][j] += myphis[i] * psi(myCandidates[i], neighCandidates[NORTH][j],NORTH) *
                    (in_msgs[EAST][i] * in_msgs[WEST][i] * in_msgs[SOUTH][i]);

        // SOUTH
        if (i < arrayXDim-1)
            for (int i = 0; i < myCandidates.size(); ++i)
                for (int j = 0; j < out_msgs[SOUTH].size(); ++j)
                    out_msgs[SOUTH][j] += myphis[i] * psi(myCandidates[i], neighCandidates[SOUTH][j],SOUTH) *
                    (in_msgs[NORTH][i] * in_msgs[WEST][i] * in_msgs[EAST][i]);
    }

    void SendMessagesToNeighbors() {
        // EAST
        if (j > 0)
            thisProxy(i, j-1).RecvMessageFromNeighbor(iter, EAST, out_msgs[WEST]);

        // WEST
        if (j < arrayYDim-1)
            thisProxy(i, j+1).RecvMessageFromNeighbor(iter, WEST, out_msgs[EAST]);

        // SOUTH
        if (i > 0)
            thisProxy(i-1, j).RecvMessageFromNeighbor(iter, SOUTH, out_msgs[NORTH]);

        // NORTH
        if (i < arrayXDim-1)
            thisProxy(i+1, j).RecvMessageFromNeighbor(iter,NORTH , out_msgs[SOUTH]);
    }

    void InitMsg() {
        // WEST
        if (j > 0)
            out_msgs[WEST].resize(neighCandidates[WEST].size(), 1.0 / neighCandidates[WEST].size());

        // EAST
        if (j < arrayYDim-1)
            out_msgs[EAST].resize(neighCandidates[EAST].size(), 1.0 / neighCandidates[EAST].size());

        // NORTH
        if (i > 0)
            out_msgs[NORTH].resize(neighCandidates[NORTH].size(), 1.0 / neighCandidates[NORTH].size());

        // SOUTH
        if (i < arrayXDim-1)
            out_msgs[SOUTH].resize(neighCandidates[SOUTH].size(), 1.0 / neighCandidates[SOUTH].size());
    }

    void ProcessMsgFromNeighbor(int from, vector<double> msg) {
        in_msgs[from] = msg;
    }

    void ConvergenceTest() {
        int numCands = myCandidates.size();
        double mynorm = 0;
        double newbelief;
        /* For each candidate patch:  compute my belief
         * sum difference between new belief and old belief
         * replace old belief with new belief*/
        for (int i=0; i<numCands; i++) {
            newbelief = computeBelief(i);
            mynorm += (newbelief - beliefs[i]) * (newbelief - beliefs[i]);
            beliefs[i] = newbelief;
        }
        /*Compute norm of belief, contribute to reduction to main chare*/
        mynorm = sqrt(mynorm);
        contribute(sizeof(double), &mynorm, CkReduction::max_double, CkCallback(CkReductionTarget(Main, CheckConverged), mainProxy));
    }

    double computeBelief(int index) {
        double belief = myphis[index];
        for(int i=0; i<4; i++) {
            belief = belief * in_msgs[i][index];
        }
        return belief;
    }

    void GetFinalPatch() {
        patch_t data;
        data.i = thisIndex.x;
        data.j = thisIndex.y;
        //get index with max belief
        int max_index = max_element(beliefs.begin(),beliefs.end()) - beliefs.begin();
        // get db index for the corresponding max_index
        data.id = myCandidates[max_index];
        contribute(sizeof(patch_t), &data, CkReduction::concat, CkCallback(CkIndex_Main::RecvFinalPatch(NULL), mainProxy));
    }

private:

    //TODO: reimplement: replaced NORTH,EAST etc
    // with 1,2,3,4. may not be correct
    double psi(int index_me,int index_them, int dir) {
        /*
        index_me: pointer to my patch in global array
        index_them: pointer to neighbor's patch in global array
        dir: how to go from me to them
        SIGMA: noise paramter (scalar)

        Assumes overlap of one pixel.
        */

        /*
        int n;
        double **xi, **xj;
        double distance = 0;

        xi = global_patches[index_me];
        xj = global_patches[index_them];
        n = xi.size();
        switch(dir) {
        	case 1 :
        		for (int i = 0; i < n; i++) {
        			distance += (xi[0][i] - xj[n-1][i])*(xi[0][i] - xj[n-1][i]);
        		}
        		break;
        	case 2 :
        		for (int i = 0; i < n; i++) {
        			distance += (xi[n-1][i] - xj[0][i])*(xi[n-1][i] - xj[0][i]);
        		}
        		break;
        	case 3 :
        		for (int i = 0; i < n; i++ ) {
        			distance += (xi[i][0] - xj[i][n-1])*(xi[i][0] - xj[i][n-1]);
        		}
        		break;
        	case 4 :
        		for (int i = 0; i < n; i++) {
        			distance += (xi[i][n-1] - xj[i][0])*(xi[i][n-1] - xj[i][0]);
        		}
        		break;
        }

        return exp(-distance/(2*SIGMA*SIGMA));
         */
        return 0.0;
    }


};


int GetFilesFromDir(const string &sFolderName, vector<string> &vFilePaths)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (sFolderName.c_str())) != NULL) {
        /* add all filepath in the training folder to vector*/
        while ((ent = readdir (dir)) != NULL) {
            vFilePaths.push_back(ent->d_name);
        }
        closedir (dir);
        return SUCCESS;
    } else {
        /* could not open directory */
        cout << "ERROR:" << sFolderName << " contains files that don't open";
        return FAIL;
    }
}


double phi(Patch lhs, Patch rhs,int size) {
    /*
    * the patches are represnetd in a 1D array
    * iterate through each index and calculate distance
    * and sum to get phi
    */
    double distance = 0;

    for (int i = 0; i < size; i++) {
        distance += (lhs[i] - rhs[i])*(lhs[i] - rhs[i]);
    }

    return exp(-distance/(2*SIGMA*SIGMA));
}






#include "SuperResolution.def.h"
