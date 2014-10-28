#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define SIGMA 1.0

enum direction {NORTH, SOUTH, EAST, WEST};

template<size_t n>
double psi(double xi[][n], double xj[][n], direction dir) {
	/*
	xi and xj:  n x n array of doubles
	n: length of one dimension of xi
	dir: how to go from xi to xj
	SIGMA: noise paramter (scalar)

	Assumes overlap of one pixel.
	*/
	double distance = 0;
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


template<size_t n>
double phi(double xi[][n], double xj[][n]) {
	/*
	xi and xj:  n x n array of doubles
	n: length of one dimension of xi
	SIGMA: noise paramter (scalar)
	*/
	double distance = 0;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			distance += (xi[i][j] - xj[i][j])*(xi[i][j] - xj[i][j]);
		}
	}

	return exp(-distance/(2*SIGMA*SIGMA));
}


int main() {
	direction dir;
	int n;

	n = 2;
	dir = WEST;
	double xi[][2] = {{0, 0},
		              {0, 0}};
	double xj[][2] = {{1, 2},
		              {3, 4}};

	printf("%f\n", psi(xi, xj, dir));
	printf("%g\n", phi(xi, xj));
}
