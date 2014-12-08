#pragma once

#include <vector>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <utility>
#include "pup_stl.h"
#include <cstring>
#include <cassert>
#include <dirent.h>
#include <algorithm>
using namespace std;



// macros and constants 
#define SUCCESS          0
#define FAIL             1
#define CONV_PERIOD      5 // convergence test every CONV_PERIOD
#define CANDIDATE_COUNT 16
#define NORTH 0
#define SOUTH 1
#define WEST  2
#define EAST  3

const double SIGMA = 255; // Determined via trial and error, may need to adjust this more
const double TOL = .00001; //Tolerance for convergence test


//typedefs 
typedef vector<double> Patch;
typedef pair<Patch,Patch> CandidatePair; // low, high
typedef vector<CandidatePair> ImageData;
typedef unsigned int PatchID; 

typedef struct {
    int i, j;
    PatchID id;
} patch_t;

typedef ImageData::iterator DBIter;


struct ImageDB {
  int nImageFiles;
  int lowResSize; 
  int highResSize;
  ImageData imageData;
  ImageDB()
  {
    nImageFiles=lowResSize=highResSize= 0;
  }

  Patch GetLowResPatch(int index)
  {
    return imageData[index].first;
  }
  Patch GetHighResPatch(int index)
  {
    return imageData[index].second;
  }
};


// utility functions
void WriteFinalPatches(const string &sOutputImagePath, ImageDB *db, patch_t *p, int num_elements);
int GetFilesFromDir(const string &sFolderName, vector<string> &);
double phi(Patch lhs, Patch rhs,int size);
