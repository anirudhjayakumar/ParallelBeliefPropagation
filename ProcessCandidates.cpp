void ProcessCandidates(int dir, int size, int indices[size]) {
	/*
	indices: global indices of my neighbors' patches
	
	Stores my neighbors' patches locally, and calculates
	the psi against my patches.
	*/

	Vector<int> *neighborCandidates;
	Vector<Vector<double>> *myPsi;

	switch(dir) {
		case NORTH :
			*neighborCandidates = north;
			*myPsi = myPsiNorth;
			break;
		case EAST :
			*neighborCandidates = east;
			*myPsi = myPsiEast;
			break;
		case SOUTH :
			*neighborCandidates = south;
			*myPsi = myPsiSouth;
			break;
		case WEST :
			*neighborCandidates = west;
			*myPsi = myPsiWest;
			break;
	}
	for (int i = 0; i < size; i++) {
		neighborCandidates->push_back(indices[i]);
		for (int j = 0; j < myCandidates.size(); j++) {
			(*myPsi)[j][i] = psi(myCandidates[j], indices[i], dir);
		}
	}
}
