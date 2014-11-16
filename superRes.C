#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include "pup_stl.h"
using namespace std;
#include "SuperResolution.decl.h"
#include <assert.h>
#include <fstream>
#include <utility>
#include "pup_stl.h"
#include "SuperResolution.decl.h"


// macros and constants 
#define SUCCESS 0
#define FAIL    1
#define CONV_PERIOD 5 // convergence test every CONV_PERIOD
const double TOL = 1.0;


// utility functions
int GetFilesFromDir(const string &sFolderName, vector<string> &);






//typedefs 
typedef pair<double*,double*> CandidatePair;
typedef vector<CandidatePair> ImageData;
typedef unsigned int PatchID; 


struct ImageDB {
  int nImageFiles;
  int lowResSize; 
  int highResSize;
  ImageData imageData;
  ImageDB()
  {
    nImageFiles=lowResSize=highResSize= 0;
  }
};
typedef ImageData::iterator DBIter;




/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_PatchArray arrayProxy; 


class Main: public CBase_Main {
  public:
    Main(CkArgMsg *m) {
      //print usage details
      if(m->argc != 2)
      {
        CkPrintf("Usage: ./charmrun +p4 ./superRes <db folder path>");
      }

      mainProxy = thisProxy;

      //get the training set directory
      const string sDBFolderPath = m->argv[1]; 
      //construct node group
      CProxy_DBNode dbProxy = CProxy_DBNode::ckNew();
      dbProxy.FillDB(sDBFolderPath);
      //construct patcharray
      // placeholder dimensions
      CkArrayOptions opts(10, 10);
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
        patch_t *p = msg->getData();
        int num_elements = msg->size() / sizeof(patch_t);
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
     for (int i=0;i < totalPatchPerFile;++i)
     {
       double *pLowRes  = new double[db.lowResSize];
       double *pHighRes = new double[db.highResSize];
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
    return &DB;
  }

  void PrintDB()
{
  cout << "=================DB Begin==================" << endl;
  cout << db.lowResSize << " " << db.highResSize << endl;
  for(DBIter itr = db.imageData.begin(); itr != db.imageData.end(); ++itr)
  {
    CandidatePair &dbItem = *itr;
    for(int i =0;i<db.lowResSize;++i)
      cout << dbItem.first[i] << " ";
    for(int i=0;i<db.highResSize;++i)
      cout << dbItem.second[i] << " ";
    cout << endl;
  }
  cout << "=================DB End====================" << endl;
}
  

};



class PatchArray: public CBase_PatchArray {
    PatchArray_SDAG_CODE
  private:
    vector<PatchID> nmy;
    vector< vector<PatchID> > n_patches;
    vector< vector<double> >  in_msgs, out_msgs;
    vector<double> beliefs;
	  std::vector<int> myCandidates;
	  std::vector<double> myphis;
    int x, y, dim_x, dim_y;
   
  public:
    PatchArray() {
        __sdag_init();

        x = thisIndex.x;
        y = thisIndex.y;

        n_patches.resize(4);
        out_msgs.resize(4);
        in_msgs.resize(4);
    }

    void SendPatchesToNeighbors() {
        // UP
        if (y > 0)
            thisProxy(x, y-1).RecvCandidatesFromNeighbors(1, nmy);

        // DOWN
        if (y < dim_y-1)
            thisProxy(x, y+1).RecvCandidatesFromNeighbors(0, nmy);

        // LEFT
        if (x > 0)
            thisProxy(x-1, y).RecvCandidatesFromNeighbors(3, nmy);

        // RIGHT
        if (x < dim_x-1)
            thisProxy(x+1, y).RecvCandidatesFromNeighbors(2, nmy);
    }

    void ComputeMessages() {
        // UP
        if (y > 0)
            for (int i = 0; i < nmy.size(); ++i)
                for (int j = 0; j < out_msgs[0].size(); ++j)
                    out_msgs[0][j] += phi(nmy[i]) * psi(nmy[i], n_patches[0]) * (in_msgs[1][i] * in_msgs[2][i] * in_msgs[3][i]);

        // DOWN
        if (y < dim_y-1)
            for (int i = 0; i < nmy.size(); ++i)
                for (int j = 0; j < out_msgs[1].size(); ++j)
                    out_msgs[1][j] += phi(nmy[i]) * psi(nmy[i], n_patches[1]) * (in_msgs[0][i] * in_msgs[2][i] * in_msgs[3][i]);

        // LEFT
        if (x > 0)
            for (int i = 0; i < nmy.size(); ++i)
                for (int j = 0; j < out_msgs[2].size(); ++j)
                    out_msgs[2][j] += phi(nmy[i]) * psi(nmy[i], n_patches[2]) * (in_msgs[0][i] * in_msgs[1][i] * in_msgs[3][i]);

        // RIGHT
        if (x < dim_x-1)
            for (int i = 0; i < nmy.size(); ++i)
                for (int j = 0; j < out_msgs[3].size(); ++j)
                    out_msgs[3][j] += phi(nmy[i]) * psi(nmy[i], n_patches[3]) * (in_msgs[0][i] * in_msgs[1][i] * in_msgs[2][i]);
    }

    void SendMessageToNeighbors() {
        // UP
        if (y > 0)
            thisProxy(x, y-1).RecvMessageFromNeighbor(1, out_msgs[0]);

        // DOWN
        if (y < dim_y-1)
            thisProxy(x, y+1).RecvMessageFromNeighbor(0, out_msgs[1]);

        // LEFT
        if (x > 0)
            thisProxy(x-1, y).RecvMessageFromNeighbor(3, out_msgs[2]);

        // RIGHT
        if (x < dim_x-1)
            thisProxy(x+1, y).RecvMessageFromNeighbor(2, out_msgs[3]);
    }

    void InitMsg() {
        // UP
        if (y > 0)
            out_msgs[0].resize(n_patches[0].size(), 1.0 / n_patches[0].size());

        // DOWN
        if (y < dim_y-1)
            out_msgs[1].resize(n_patches[1].size(), 1.0 / n_patches[1].size());

        // LEFT
        if (x > 0)
            out_msgs[2].resize(n_patches[2].size(), 1.0 / n_patches[2].size());

        // RIGHT
        if (x < dim_x-1)
            out_msgs[3].resize(n_patches[3].size(), 1.0 / n_patches[3].size());
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
        contribute(sizeof(patch_t), data, CkReduction::concat, CkCallback(CkIndex_Main::RecvFinalPatch(NULL), mainProxy));
    }
};


int GetFilesFromDir(const string &sFolderName, vector<string> &vFilePaths)
{
#include <dirent.h>
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






#include "SuperResolution.def.h"
