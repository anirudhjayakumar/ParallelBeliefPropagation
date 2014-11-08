#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define SIGMA 1.0

enum direction {NORTH, SOUTH, EAST, WEST};

double psi(int index_me, int index_them, direction dir) {
	/*
	index_me: pointer to my patch in global array
	index_them: pointer to neighbor's patch in global array
	dir: how to go from me to them
	SIGMA: noise paramter (scalar)

	Assumes overlap of one pixel.
	*/
	int n;
	double **xi, **xj;
	double distance = 0;

	xi = global_patches[index_me];
	xj = global_patches[index_them];
	n = xi.size();
	switch(dir) {
		case NORTH :
			for (int i = 0; i < n; i++) {
				distance += (xi[0][i] - xj[n-1][i])*(xi[0][i] - xj[n-1][i]);
			}
			break;
		case SOUTH : 
			for (int i = 0; i < n; i++) {
				distance += (xi[n-1][i] - xj[0][i])*(xi[n-1][i] - xj[0][i]);
			}
			break;
		case WEST : 
			for (int i = 0; i < n; i++ ) {
				distance += (xi[i][0] - xj[i][n-1])*(xi[i][0] - xj[i][n-1]);
			}
			break;
		case EAST :
			for (int i = 0; i < n; i++) {
				distance += (xi[i][n-1] - xj[i][0])*(xi[i][n-1] - xj[i][0]);
			}
			break;
	}

	return exp(-distance/(2*SIGMA*SIGMA));
}


double phi(int index_them) {
	/*
	index_them: pointer to neighbor's patch in global array
	SIGMA: noise paramter (scalar)
	*/
	int n;
	double **xi, **xj;
	double distance = 0;

	xi = myLowresPatch;
	xj = global_patches[index_them];
	n = xi.size();
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			distance += (xi[i][j] - xj[i][j])*(xi[i][j] - xj[i][j]);
		}
	}

	return exp(-distance/(2*SIGMA*SIGMA));
}
