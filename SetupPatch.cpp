void SetupPatch() {
	/*
	Chooses the best candidate patches and calculates the
	corresponding phi values.
	*/
	int myplace, myindex;
	double mydist;
	int SIZE = 10;

	int index[SIZE];  // Unordered indices of best candidates
	double distance[SIZE];  // Distance from me to index[i]
	int order[SIZE];  // Order of index values
	
	// Fill with the first SIZE
	for (int i = 0; i < SIZE; i++) {
		index[i] = i;
		distance[i] = phi(i);
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
	for (int i = SIZE; i < database_size; i++) {
		mydist = phi(i);
		myplace = 0;
		for (int j = 0; j < SIZE; j++) {
			if (distance[i] < mydist) {
				myplace++;
			} else {
				order[j]++;
				if (order[j] == SIZE) {
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

	for (int i = 0; i < SIZE; i++) {
		myCandidates.push_back(index[i]);
		myphis.push_back(distance[i]);
	}
}
