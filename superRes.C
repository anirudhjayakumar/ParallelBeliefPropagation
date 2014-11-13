#include <stdlib.h>
#include <vector>
#include <string>
#include "pup_stl.h"
#include "SuperResolution.decl.h"
#include <assert.h>

using namespace std;


typedef pair<double*,double*> CandidatePair;
typedef vector<CandidatePair> ImageDB;

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_Cell cellProxy;

class Main: public CBase_Main {
    Main_SDAG_CODE;

  public:
    Main(CkArgMsg *m) {
        __sdag_init();
        Setup();
    }

	void CheckConverged(double norm) {
		if (norm < TOL) {
			arrayProxy.GetFinalPatch();
		}
		else {
			arrayProxy.Run();
		}
	}
};

class TrainingSet: public CBase_TrainingSet
{
private:
  ImageDB DB;
public:
  TrainingSet()
  {
  }
  
  void FillDB(const string &sTrainingSetDirPath)
  {
    //read code goes here
    //iterate through each file
    //read and populate the data base
  }
  
  ImageDB *GetImageDB()
  {
    return &DB;
  }

};



class PatchArray: public CBase_PatchArray {
    PatchArray_SDAG_CODE;

    vector<PatchID> nmy;
    vector< vector<PatchID> > n_patches;
    vector< vector<double> >  in_msgs, out_msgs;
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
            thisProxy(x, y-1).RecvCandidatesFromNeighbors(DOWN, out_msgs[0]);

        // DOWN
        if (y < dim_y-1)
            thisProxy(x, y+1).RecvCandidatesFromNeighbors(UP, out_msgs[1]);

        // LEFT
        if (x > 0)
            thisProxy(x-1, y).RecvCandidatesFromNeighbors(RIGHT, out_msgs[2]);

        // RIGHT
        if (x < dim_x-1)
            thisProxy(x+1, y).RecvCandidatesFromNeighbors(LEFT, out_msgs[3]);
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

	void ConvergenceTest() {
		numCands = myCandidates.size();
		double mynorm = 0;
		double newbelief;
		/* For each candidate patch:  compute my belief
		 * sum difference between new belief and old belief
		 * replace old belief with new belief*/
		for (int i=0; i<numCands i++) {
			newbelief = computeBelief(i);
			mynorm += (newbelief - beliefs[i]) * (newbelief - beliefs[i]);
			beliefs[i] = newbelief
		}
		/*Compute norm of belief, contribute to reduction to main chare*/
		mynorm = sqrt(mynorm);
		contribute(sizeof(double) &mynorm, CkReduction::max_double, CkCallback(CkReductionTarget(Main, checkConverged), mainProxy));
	}

	double computeBelief(int index) {
		double belief = myphis[index];
		for(int i=0; i<4; i++) {
			belief = belief * in_msgs[i][index];
		}
		return belief;
	}
};

#include "SuperResolution.def.h"
