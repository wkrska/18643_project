#define R 64 // number of rows in a hash table
#define C 4  // number of slots in a row
#define TU_SIZE 4  // size of a tuple

using namespace std;

struct tuple {
	int rid;
	int key;
	int ptr;
};

struct hash_table {
	int status = 0; 		 // indicates whether the slot is occupied
	int tag = 0;             // candidate bucket number of this element
	struct tuple head[TU_SIZE];    // the entry point to the first tuple of this slot
};

struct hash_table hash_table[R][C];


int hash (int function, int key) {
	switch (function) {
		case 0: return key % R;
		case 1: return (key / R) % R;
	}
}


void lookup () {
	return;
}

void insert () {
	return;
}

// void remove ()
// we dont need this
