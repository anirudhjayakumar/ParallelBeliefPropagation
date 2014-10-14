#include <math.h>
#include <stdio.h>
#include <stdlib.h>

enum direction {NORTH, SOUTH, EAST, WEST};

template<size_t n>
double psi(double xi[][n], double xj[][n], direction dir) {
	/*
	xi and xj:  n x n array of doubles
	n: length of one dimension of xi
	dir: how to go from xi to xj
	sigma: noise paramter (scalar)

	Assumes overlap of one pixel.
	*/
	double sigma = 1.0;
	double distance = 0;
	for (int i = 0; i < n; i++) {
		switch(dir) {
			case NORTH : distance += pow(xi[0][i] - xj[n-1][i], 2); break;
			case SOUTH : distance += pow(xi[n-1][i] - xj[0][i], 2); break;
			case WEST : distance += pow(xi[i][0] - xj[i][n-1], 2); break;
			case EAST : distance += pow(xi[i][n-1] - xj[i][0], 2); break;
		}
	}

	return exp(-distance/(2*pow(sigma, 2)));
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
}
