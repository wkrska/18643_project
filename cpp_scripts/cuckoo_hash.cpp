#define R 64 // number of rows in a hash table
#define C 4  // number of slots in a row
#define TU_SIZE 4  // size of a tuple

using namespace std;

struct tuple {
	int rid;
	int key;
	int ptr;
};

struct table {
	int status = 0; 		 	   // indicates whether the slot is occupied
	int tag = 0;             	   // candidate bucket number of this element
	struct tuple head[TU_SIZE];    // the entry point to the first tuple of this slot
};

struct table hash_table[R][C];


int hash (int function, int key) {
	switch (function) {
		case 0: return key % R;
		case 1: return (key / R) % R;
	}
}


int lookup (int key) {
	int index = -1;
	for (int i = 0; i < C; i++) {
		if () { index = ; }		
	}
	return index;
}

void insert (int key) {
	int hash0 = hash(0, key);
	int hash1 = hash(1, key);
	if (hash_table[hash0].status == 0) {
		hash_table[hash0].status = 1;
		hash_table[hash0].tag = hash1;
	} else if (hash_table[hash1].status == 0) {
		hash_table[hash1].status = 1;
		hash_table[hash1].tag = hash0;
	} else {
		// eviction
	}
	return;
}

// void remove ()
// we dont need this
