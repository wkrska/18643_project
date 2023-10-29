#include <iostream>
#define R 64 // number of rows in a hash table
#define C 4  // number of slots in a row
#define T 128 // size of the list of tuples, not sure how big this should be
#define MAX 1000 // max number of time of hashing, for cycle detection
using namespace std;


/*
	Fig. 2 Dataflow:
		input -> 
        1. buffer & hash -> 
        2. hash table -> 
        3. address table -> 
        stream out
*/

int hash_function (int function, int key) {
	switch (function) {
		case 0: return key % R;
		case 1: return (key / R) % R;
	}
}

struct tuple {
	int rid;
	int key;
	int hash0;
	int hash1;
    tuple (int key, int rid) {
        this->rid = rid;
        this->key = key;
        this->hash0 = hash_function(0, key);
        this->hash1 = hash_function(1, key);
    }
};

struct table {
	int status = 0; 		 // indicates whether the slot is occupied
	int tag = 0;             // candidate bucket number of this element
	struct tuple head[T];    // the entry point to the first tuple of this slot
};

struct table hash_table[R][C];

int scan (int row) {
	/*
    purpose: find the first free slot of a given row/bucket
	        return -1 if all slots are full
    */

	for (int i = 0; i < C; i++) {
		if (hash_table[row][i].status == 0) {
			return i;
		}
	}
	return -1;
}

void insert (struct tuple buffer, int cnt) {
    if (cnt == MAX) {
    // cnt counts how many times this function has been called
    // when cnt == MAX, we treat that as a cycle
        cout<<"There might be an inifinte loop\n"<;
        return;
    }

    int key = buffer.key;
    int rid = buffer.rid;
    int hash0 = buffer.hash0;
    int hash1 = buffer.hash1;
    int index0 = scan(hash0);
    int index1 = scan(hash1);

	if (index0 != -1) {
        hash_table[hash0][index0].status = 1;
        hash_table[hash0][index0].tag = hash1;
        hash_table[hash0][index0].head[0] = buffer;
    } else if (index1 != -1) {
        hash_table[hash1][index1].status = 1;
        hash_table[hash1][index1].tag = hash0;
        hash_table[hash1][index1].head[0] = buffer;
    } else {
        // which slot to evict?
        if (cnt%2 == 0) {
            struct tuple temp = hash_table[hash0][index0].head[0];
            hash_table[hash0][index0].tag = hash1;
            hash_table[hash0][index0].head[0] = buffer;
            insert(temp, cnt + 1);
        } else {
            struct tuple temp = hash_table[hash1][index1].head[0];
            hash_table[hash1][index1].tag = hash0;
            hash_table[hash1][index1].head[0] = buffer;
            insert(temp, cnt + 1);
        }
    }
	return;
}

