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
    int x, y;
    Patch myPatch;
    ImageDB *DB;
    int iter;

public:
    PatchArray() {
        __sdag_init();

        x = thisIndex.x;
        y = thisIndex.y;

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
        // UP
        if (y > 0)
            thisProxy(x, y-1).RecvCandidatesFromNeighbors(1, myCandidates);

        // DOWN
        if (y < arrayYDim-1)
            thisProxy(x, y+1).RecvCandidatesFromNeighbors(0, myCandidates);

        // LEFT
        if (x > 0)
            thisProxy(x-1, y).RecvCandidatesFromNeighbors(3, myCandidates);

        // RIGHT
        if (x < arrayXDim-1)
            thisProxy(x+1, y).RecvCandidatesFromNeighbors(2, myCandidates);
    }

    void ProcessCandidates(int dir, vector<PatchID> patches) {
        neighCandidates[dir] = patches;
    }


    void ComputeMessages() {
        // UP
        if (y > 0)
            for (int i = 0; i < myCandidates.size(); ++i)
                for (int j = 0; j < out_msgs[0].size(); ++j)
                    out_msgs[0][j] += myphis[i] * psi(myCandidates[i], neighCandidates[0][j],0) * (in_msgs[1][i] * in_msgs[2][i] * in_msgs[3][i]);

        // DOWN
        if (y < arrayYDim-1)

            for (int i = 0; i < myCandidates.size(); ++i)
                for (int j = 0; j < out_msgs[1].size(); ++j)
                    out_msgs[1][j] += myphis[i] * psi(myCandidates[i], neighCandidates[1][j],1) * (in_msgs[0][i] * in_msgs[2][i] * in_msgs[3][i]);

        // LEFT
        if (x > 0)
            for (int i = 0; i < myCandidates.size(); ++i)
                for (int j = 0; j < out_msgs[2].size(); ++j)
                    out_msgs[2][j] += myphis[i] * psi(myCandidates[i], neighCandidates[2][j],2) * (in_msgs[0][i] * in_msgs[1][i] * in_msgs[3][i]);

        // RIGHT
        if (x < arrayXDim-1)
            for (int i = 0; i < myCandidates.size(); ++i)
                for (int j = 0; j < out_msgs[3].size(); ++j)
                    out_msgs[3][j] += myphis[i] * psi(myCandidates[i], neighCandidates[3][j],3) * (in_msgs[0][i] * in_msgs[1][i] * in_msgs[2][i]);
    }

    void SendMessagesToNeighbors() {
        // UP
        if (y > 0)
            thisProxy(x, y-1).RecvMessageFromNeighbor(iter, 1, out_msgs[0]);

        // DOWN
        if (y < arrayYDim-1)
            thisProxy(x, y+1).RecvMessageFromNeighbor(iter, 0, out_msgs[1]);

        // LEFT
        if (x > 0)
            thisProxy(x-1, y).RecvMessageFromNeighbor(iter, 3, out_msgs[2]);

        // RIGHT
        if (x < arrayXDim-1)
            thisProxy(x+1, y).RecvMessageFromNeighbor(iter, 2, out_msgs[3]);
    }

    void InitMsg() {
        // UP
        if (y > 0)
            out_msgs[0].resize(neighCandidates[0].size(), 1.0 / neighCandidates[0].size());

        // DOWN
        if (y < arrayYDim-1)
            out_msgs[1].resize(neighCandidates[1].size(), 1.0 / neighCandidates[1].size());

        // LEFT
        if (x > 0)
            out_msgs[2].resize(neighCandidates[2].size(), 1.0 / neighCandidates[2].size());

        // RIGHT
        if (x < arrayXDim-1)
            out_msgs[3].resize(neighCandidates[3].size(), 1.0 / neighCandidates[3].size());
    }

    void ProcessMsgFromNeighbor(int dir, vector<double> msg) {
        in_msgs[dir] = msg;
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
