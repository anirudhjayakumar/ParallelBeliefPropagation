#include "SuperResolution.h"
#include "SuperResolution.decl.h"

#include "lshbox.h"
#include <vector>
#include <utility>


/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_PatchArray arrayProxy;
/*readonly*/ CProxy_DBNode dbProxy;
/*readonly*/ int arrayXDim;
/*readonly*/ int arrayYDim;
/*readonly*/ int nodeCount;


class CandidateList: public CMessage_CandidateList
{
    public:
    PatchID ids[SEARCH_COUNT];
};

class DBNode: public CBase_DBNode
{
private:
    struct ImageDB db;
    lshbox::Matrix<double> data;
    lshbox::itqLsh<double> mylsh;
    int nDBSearchSize, nDBSearchStartIndex,nDBSearchEndIndex;
public:
    DBNode()
    {
    }

    static vector<Patch> ProcessImage(const string &sImagePath, int *dimX, int *dimY)
    {
        ifstream imageFile;
        imageFile.open(sImagePath.c_str());

        int totalPatchPerFile, patchSize;
        imageFile >> totalPatchPerFile;
        imageFile >> *dimX >> *dimY;
        imageFile >> patchSize;

        vector<Patch> patches;
        for (int i = 0; i < totalPatchPerFile; ++i)
        {
            Patch pImage(patchSize);

            for (int j = 0; j < patchSize; ++j)
            {
                imageFile >> pImage[j];
            }

            patches.push_back(pImage);
        }

        return patches;
    }

    void FillDB(const string &sTrainingSetDirPath)
    {
        vector<string> sFiles;
        if (GetFilesFromDir(sTrainingSetDirPath,sFiles) == FAIL)
        {
            CkExit();
        }

        //sort the file names so that they are read in the same sequence
        //in all the nodes.
        sort(sFiles.begin(),sFiles.end());

        for (vector<string>::iterator itr = sFiles.begin(); itr != sFiles.end(); ++itr)
        {
            if (*itr == "." || *itr == "..") continue;
            ifstream imageFile;
            string sFilePath = sTrainingSetDirPath + "/" + itr->c_str();
            imageFile.open(sFilePath.c_str());
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
                if ((db.nImageFiles > 0 && (db.lowResSize != lowResSize || db.highResSize != highResSize)))
                {
                    cout << "ERROR: DB input patch size don't match across inputs" << endl;
                    // return FAIL;
                }

                if (db.nImageFiles == 0)
                {
                    db.lowResSize = lowResSize;
                    db.highResSize = highResSize;
                }

                db.nImageFiles++;
                for (int i=0; i < totalPatchPerFile; ++i)
                {
                    Patch pLowRes(db.lowResSize);
                    Patch pHighRes(db.highResSize);

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

        CkPrintf("%d] DB loaded. Size:%d\n",CkMyNode(),db.imageData.size());        
        // get the size, startindex and endIndex of the db 
        GetSearchSpaceInformation();
        
        vector<double> v(db.lowResSize * nDBSearchSize);
        int c = 0;
        for (int i = 0; i < nDBSearchSize; ++i) {
            for (int j = 0; j < db.lowResSize; ++j) {
                v[c++] = db.imageData[i+nDBSearchStartIndex].first[j];
            }
        }
        CkPrintf("c = %d\n", c);
        CkPrintf("[%d]DB Search Size = %d\n", CkMyNode(),nDBSearchSize);
        CkPrintf("[%d]DB Search start index = %d\n", CkMyNode(),nDBSearchStartIndex);
        CkPrintf("[%d]DB Search endIndex = %d\n", CkMyNode(),nDBSearchEndIndex);
        CkPrintf("[%d]db.lowResSize = %d\n", CkMyNode(),db.lowResSize);
        CkPrintf("[%d]data.getDim() = %d\n", CkMyNode(),data.getDim());


        CkPrintf("[%d]Starting to load\n",CkMyNode());
        data.load(&v[0], nDBSearchSize, db.lowResSize);
                CkPrintf("[%d]Starting to train\n",CkMyNode());
        lshbox::itqLsh<double>::Parameter param;
        param.M = 52100;
        param.L = 5;
        param.D = data.getDim();
        param.N = 8;
        param.S = nDBSearchSize;
        param.I = 50;
        mylsh.reset(param);
        mylsh.train(data);

        CkPrintf("done loading\n");

        contribute(CkCallback(CkReductionTarget(Main, DB_Populated), mainProxy));
        // return SUCCESS;
    }

    ImageDB *GetImageDB()
    {
        return &db;
    }

    lshbox::Matrix<double> *GetData()
    {
        return &data;
    }

    lshbox::itqLsh<double> *GetLSH()
    {
        return &mylsh;
    }

    void PrintDB()
    {
        cout << "=================DB Begin==================" << endl;
        cout << db.lowResSize << " " << db.highResSize << endl;
        for (DBIter itr = db.imageData.begin(); itr != db.imageData.end(); ++itr)
        {
            CandidatePair &dbItem = *itr;
            for (int i =0; i<db.lowResSize; ++i)
                cout << dbItem.first[i] << " ";
            for (int i=0; i<db.highResSize; ++i)
                cout << dbItem.second[i] << " ";
            cout << endl;
        }
        cout << "=================DB End====================" << endl;
    }

    void RequestRemoteDBSearch(Patch in_patch, CkCallback callBack_)
    {
        for(int i =0;i < nodeCount;++i)
        {
            if(i != CkMyNode()) 
                dbProxy[i].SearchDB(in_patch,callBack_);
        }
    }

    void SearchDB(Patch in_patch,CkCallback callBack_)
    {
        lshbox::itqLsh<double> *mylsh = GetLSH();
        lshbox::Matrix<double> *data  = GetData();

        lshbox::Matrix<double>::Accessor accessor(*data);
        lshbox::Metric<double> metric(data->getDim(), L2_DIST);

        
        lshbox::Scanner<lshbox::Matrix<double>::Accessor> scanner(
            accessor,
            metric,
            SEARCH_COUNT,
            std::numeric_limits<double>::max()
        );

        std::cout << "RUNNING QUERY ..." << std::endl;
        scanner.reset(&in_patch[0]);
        mylsh->query(&in_patch[0], scanner);
        std::vector<std::pair<unsigned, float> > topK = scanner.topk().getTopk();

        CandidateList *msg = new CandidateList();
        for (int i = 0; i < SEARCH_COUNT; i++) {
            int idx = topK[i].first;
            msg->ids[i]=idx;
        }
        callBack_.send(msg);

    }
    void GetSearchSpaceInformation()
    {
        int nodeIndex = CkMyNode();
        int mod = db.imageData.size()%nodeCount;
        int perNodeDBSize = db.imageData.size()/nodeCount;
        if(nodeIndex < mod)
        {
            nDBSearchSize = perNodeDBSize + 1;
            nDBSearchStartIndex = nodeIndex*nDBSearchSize;
            nDBSearchEndIndex = nDBSearchStartIndex + nDBSearchSize -1;
        }
        else
        {
            nDBSearchSize = perNodeDBSize;
            nDBSearchStartIndex = nodeIndex*nDBSearchSize + mod;
            nDBSearchEndIndex = nDBSearchStartIndex + nDBSearchSize -1;
        }
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
    int i_index, j_index;
    Patch myPatch;
    ImageDB *DB;
    int iter,recv_count;
public:
    PatchArray(vector<Patch> img) {
        __sdag_init();

        i_index = thisIndex.x;
        j_index = thisIndex.y;

        beliefs.resize(CANDIDATE_COUNT,0);        
        neighCandidates.resize(4);
        out_msgs.resize(4);
        in_msgs.resize(4);

        myPatch = img[i_index + arrayXDim*j_index];
    }
    PatchArray(CkMigrateMessage *msg) {}

    void SetupPatch()
    {
        /*
        Chooses the best candidate patches and calculates the
        corresponding phi values.
        */
        CkPrintf("[%d,%d] DB search begin\n",thisIndex.x,thisIndex.y);


        //send request to nodegroup for remote search
       if( CkNumNodes() > 0)
       {
           CkCallback callbk_(CkIndex_PatchArray::RecvCandidatesFromRemoteDB(NULL),thisProxy(thisIndex.x,thisIndex.y));
           dbProxy.ckLocalBranch()->RequestRemoteDBSearch(myPatch,callbk_);
       }
        
        // Get pointer to the db from the node group
        lshbox::itqLsh<double> *mylsh = dbProxy.ckLocalBranch()->GetLSH();
        lshbox::Matrix<double> *data  = dbProxy.ckLocalBranch()->GetData();

        lshbox::Matrix<double>::Accessor accessor(*data);
        lshbox::Metric<double> metric(data->getDim(), L2_DIST);

        lshbox::Scanner<lshbox::Matrix<double>::Accessor> scanner(
            accessor,
            metric,
            SEARCH_COUNT,
            std::numeric_limits<double>::max()
        );

        std::cout << "RUNNING QUERY ..." << std::endl;
        scanner.reset(&myPatch[0]);
        mylsh->query(&myPatch[0], scanner);
        std::vector<std::pair<unsigned, float> > topK = scanner.topk().getTopk();

        DB = dbProxy.ckLocalBranch()->GetImageDB();

        for (int i = 0; i < SEARCH_COUNT; i++) {
            int idx = topK[i].first;
            myCandidates.push_back(idx);
        }

    }

    void SendPatchesToNeighbors() {
        // EAST
        if (j_index > 0)
            thisProxy(i_index, j_index-1).RecvCandidatesFromNeighbors(EAST, myCandidates);

        // WEST
        if (j_index < arrayYDim-1)
            thisProxy(i_index, j_index+1).RecvCandidatesFromNeighbors(WEST, myCandidates);

        // SOUTH
        if (i_index > 0)
            thisProxy(i_index-1, j_index).RecvCandidatesFromNeighbors(SOUTH, myCandidates);

        // NORTH
        if (i_index < arrayXDim-1)
            thisProxy(i_index+1, j_index).RecvCandidatesFromNeighbors(NORTH, myCandidates);
    }

    void ProcessCandidates(int from, vector<PatchID> patches) {
        neighCandidates[from] = patches;
    }


    void ComputeMessages() {
        double acc;

        // WEST
        acc = 0.0;
        if (j_index > 0) {
            for (int i = 0; i < myCandidates.size(); ++i) {
                for (int j = 0; j < out_msgs[WEST].size(); ++j) {
                    out_msgs[WEST][j] += myphis[i] * psi(myCandidates[i], neighCandidates[WEST][j],WEST) *
                                         (in_msgs[EAST][i] * in_msgs[SOUTH][i] * in_msgs[NORTH][i]);
                    acc += out_msgs[WEST][j];
                }
                for (int j = 0; j < out_msgs[WEST].size(); ++j) {
                    out_msgs[WEST][j] /= acc;
                }
            }
        }

        // EAST
        acc = 0.0;
        if (j_index < arrayYDim-1) {
            for (int i = 0; i < myCandidates.size(); ++i) {
                for (int j = 0; j < out_msgs[EAST].size(); ++j) {
                    out_msgs[EAST][j] += myphis[i] * psi(myCandidates[i], neighCandidates[EAST][j],EAST) *
                                         (in_msgs[NORTH][i] * in_msgs[SOUTH][i] * in_msgs[WEST][i]);
                    acc += out_msgs[EAST][j];
                }
                for (int j = 0; j < out_msgs[EAST].size(); ++j) {
                    out_msgs[EAST][j] /= acc;
                }
            }
        }

        // NORTH
        acc = 0.0;
        if (i_index > 0) {
            for (int i = 0; i < myCandidates.size(); ++i) {
                for (int j = 0; j < out_msgs[NORTH].size(); ++j) {
                    out_msgs[NORTH][j] += myphis[i] * psi(myCandidates[i], neighCandidates[NORTH][j],NORTH) *
                                          (in_msgs[EAST][i] * in_msgs[WEST][i] * in_msgs[SOUTH][i]);
                    acc += out_msgs[NORTH][j];
                }
                for (int j = 0; j < out_msgs[NORTH].size(); ++j) {
                    out_msgs[NORTH][j] /= acc;
                }
            }
        }

        // SOUTH
        acc = 0.0;
        if (i_index < arrayXDim-1) {
            for (int i = 0; i < myCandidates.size(); ++i) {
                for (int j = 0; j < out_msgs[SOUTH].size(); ++j) {
                    out_msgs[SOUTH][j] += myphis[i] * psi(myCandidates[i], neighCandidates[SOUTH][j],SOUTH) *
                                          (in_msgs[NORTH][i] * in_msgs[WEST][i] * in_msgs[EAST][i]);
                    acc += out_msgs[SOUTH][j];
                }
                for (int j = 0; j < out_msgs[SOUTH].size(); ++j) {
                    out_msgs[SOUTH][j] /= acc;
                }
            }
        }
    }

    void SendMessagesToNeighbors() {
        // EAST
        if (j_index > 0)
            thisProxy(i_index, j_index-1).RecvMessageFromNeighbor(iter, EAST, out_msgs[WEST]);

        // WEST
        if (j_index < arrayYDim-1)
            thisProxy(i_index, j_index+1).RecvMessageFromNeighbor(iter, WEST, out_msgs[EAST]);

        // SOUTH
        if (i_index > 0)
            thisProxy(i_index-1, j_index).RecvMessageFromNeighbor(iter, SOUTH, out_msgs[NORTH]);

        // NORTH
        if (i_index < arrayXDim-1)
            thisProxy(i_index+1, j_index).RecvMessageFromNeighbor(iter,NORTH , out_msgs[SOUTH]);
    }

    void InitMsg() {
        // WEST
        if (j_index > 0)
            out_msgs[WEST].resize(neighCandidates[WEST].size(), 1.0 / neighCandidates[WEST].size());

        // EAST
        if (j_index < arrayYDim-1)
            out_msgs[EAST].resize(neighCandidates[EAST].size(), 1.0 / neighCandidates[EAST].size());

        // NORTH
        if (i_index > 0)
            out_msgs[NORTH].resize(neighCandidates[NORTH].size(), 1.0 / neighCandidates[NORTH].size());

        // SOUTH
        if (i_index < arrayXDim-1)
            out_msgs[SOUTH].resize(neighCandidates[SOUTH].size(), 1.0 / neighCandidates[SOUTH].size());

        // init all input msg to 1. 
        in_msgs[NORTH].resize(CANDIDATE_COUNT, 1.0);
        in_msgs[SOUTH].resize(CANDIDATE_COUNT, 1.0);
        in_msgs[EAST].resize(CANDIDATE_COUNT, 1.0);
        in_msgs[WEST].resize(CANDIDATE_COUNT, 1.0);
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
        for (int i=0; i<4; i++) {
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
    void ProcessCandidatesFromRemoteDB(CandidateList* patches)
    {
        myCandidates.insert(myCandidates.end(),patches->ids,patches->ids + SEARCH_COUNT);
    }

    void ConsolidateCandidates()
    {
        vector<pair<PatchID, double> > patchIdtoDistMap;
        double dist = 0;
        for (int i=0;i<myCandidates.size();++i)
        {
            dist = phi(myPatch,DB->GetLowResPatch(myCandidates[i]),DB->lowResSize);
            patchIdtoDistMap.push_back(pair<PatchID,double>(myCandidates[i],dist));
        }
        sort(patchIdtoDistMap.begin(), patchIdtoDistMap.end(), [](const pair<int,int> &left, const pair<int,int> &right) {
                return left.second > right.second;});
        myCandidates.clear();
        for (int i=0;i<CANDIDATE_COUNT;++i)
        {
            myCandidates.push_back(patchIdtoDistMap[i].first);
            myphis.push_back(patchIdtoDistMap[i].second);
        }
    }

	double psi(int index_me, int index_them, int from) {
        /*
        index_me: pointer to my patch in global array
        index_them: pointer to neighbor's patch in global array
        from: how to go from receiver to sender
        SIGMA: noise paramter (scalar)
        Assumes overlap of one pixel.
        */
		//Indices should work with, but have not been fully tested.
        
        int n;
        double distance = 0;

		Patch xi = DB->GetHighResPatch(index_me);
		Patch xj = DB->GetHighResPatch(index_them);
        n = sqrt(xi.size());
		int myInd, theirInd;

        switch(from) {
        	case NORTH :
        		for (int i = 0; i < n; i++) {
					myInd = i;
					theirInd = (n-1)*n + i;
        			distance += (xi[myInd] - xj[theirInd]) * (xi[myInd] - xj[theirInd]);
        		}
        		break;
        	case SOUTH :
        		for (int i = 0; i < n; i++) {
					myInd = (n-1)*n + i;
					theirInd = i;
        			distance += (xi[myInd] - xj[theirInd]) * (xi[myInd] - xj[theirInd]);
        		}
        		break;
        	case WEST :
        		for (int i = 0; i < n; i++ ) {
					myInd = n*i;
					theirInd = n*(i+1) - 1;
        			distance += (xi[myInd] - xj[theirInd]) * (xi[myInd] - xj[theirInd]);
        		}
        		break;
        	case EAST :
        		for (int i = 0; i < n; i++) {
					myInd = n*(i+1) - 1;
					theirInd = n*i;
        			distance += (xi[myInd] - xj[theirInd]) * (xi[myInd] - xj[theirInd]);
        		}
        		break;
        }

        return exp(-distance/(2*SIGMA*SIGMA));
    }


};

class Main: public CBase_Main {
private:
    string sOutputImagePath;
    double dbStartTime;
public:
    Main(CkArgMsg *m) {
        // Print usage details
        if (m->argc != 4)
        {
            CkPrintf("Usage: ./charmrun +p4 ./superRes <db folder path> <input image path> <output image path>");
            CkExit();
        }
        
        mainProxy = thisProxy;
        nodeCount = CkNumNodes();
        // Get the training set directory
        const string sDBFolderPath = m->argv[1];
        // Get the input image path
        const string sInputImagePath = m->argv[2];
        // Get the output image path
        sOutputImagePath = m->argv[3];

        // Construct node group
        dbProxy = CProxy_DBNode::ckNew();
        dbStartTime = CkWallTimer();
        dbProxy.FillDB(sDBFolderPath);

        // Construct patcharray
        vector<Patch> img = DBNode::ProcessImage(sInputImagePath, &arrayXDim, &arrayYDim);

        // Construct patcharray
        CkArrayOptions opts(arrayXDim, arrayYDim);
        arrayProxy = CProxy_PatchArray::ckNew(img, opts);
    }


    void CheckConverged(double norm) {
        if (norm < TOL) {
            CkPrintf("Convergence test: Pass\n");
            arrayProxy.GetFinalPatch();
        } else {
            CkPrintf("Convergence test: Fail\n");
            arrayProxy.Run();
        }
    }

    void RecvFinalPatch(CkReductionMsg *msg) {
        patch_t *p = (patch_t*)msg->getData();
        int num_elements = msg->getSize() / sizeof(patch_t);

        ImageDB *DB = dbProxy.ckLocalBranch()->GetImageDB();
        WriteFinalPatches(sOutputImagePath, DB, p, num_elements);
        CkExit();
    }

    void DB_Populated()
    {
        CkPrintf("Database populated in %f seconds\n",CkWallTimer() - dbStartTime);
        arrayProxy.Setup();
    }
};

void WriteFinalPatches(const string &sOutputImagePath, ImageDB *db, patch_t *p, int num_elements)
{
    ofstream imageFile;
    imageFile.open(sOutputImagePath.c_str());

    int patchSize = db->imageData[0].second.size();

    //imageFile << num_elements << endl;  //Not needed by postprocessing script
    imageFile << arrayXDim << " " << arrayYDim << " " << patchSize << endl;

    for (int i = 0; i < num_elements; ++i) {
        Patch r = db->imageData[p[i].id].second;
		imageFile << p[i].i << " " << p[i].j << " ";
        for (int j = 0; j < patchSize; ++j) {
            imageFile << r[j] << " ";
        }
        imageFile << endl;
    }
}

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
