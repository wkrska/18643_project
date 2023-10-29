#include <iostream>
#define R 10 // number of rows in a hash table
#define C 4  // number of slots in a row
#define T 20 // size of the list of tuples, not sure how big this should be
#define MAX 200 // max number of time of hashing, for cycle detection
#define SIZE 10 // size of the address table
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

struct address_table {
    int rid1 = -1;
    int rid2 = -1;
    int key = -1;
};

struct buffer {
    // this is the 'tuple' from the paper
	int rid;
	int key;
	int hash0;
	int hash1;
	buffer(){};
    buffer (int key, int rid) {
        this->rid = rid;
        this->key = key;
        this->hash0 = hash_function(0, key);
        this->hash1 = hash_function(1, key);
    }
};

struct table {
	int status = 0; 	 // indicates whether the slot is occupied
	int tag = 0;             // candidate bucket number of this element
	struct buffer head[T];   // the entry point to the first tuple of this slot
};

int first_free_index_addr_table = 0;
struct address_table addr_table[SIZE];
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

int compare (int row, int key) {
    /*
    purpose: check if the given key is present in the given row
    
    */
    int index = -1;
    for (int i = 0; i < C; i++) {
        if (hash_table[row][i].status == 1) {
            if (hash_table[row][i].head[0].key == key) {
                return i;
            }
        }
    }
    return index;
}

int has_joined (int key, int hash0, int hash1, int rid) {
    int index0 = compare(hash0, key);
    int index1 = compare(hash1, key);
    if (index0 != -1) {
        addr_table[first_free_index_addr_table].rid1 = hash_table[hash0][index0].head[0].rid;
        addr_table[first_free_index_addr_table].rid2 = rid;
        addr_table[first_free_index_addr_table].key = key;
        first_free_index_addr_table++;
        return 1;
    }
    if (index1 != -1) {
        addr_table[first_free_index_addr_table].rid1 = hash_table[hash1][index1].head[0].rid;
        addr_table[first_free_index_addr_table].rid2 = rid;
        addr_table[first_free_index_addr_table].key = key;
        return 1;
    }
    return -1;
}

void insert (struct buffer buf, int cnt) {
    /*
    purpose: insert an item into hash table. If the same key is present
            perform join.
    
    */
    
    if (cnt == MAX) {
    // cnt counts how many times this function has been called
    // when cnt == MAX, we treat that as a cycle
        cout<<"There might be an inifinte loop!!\n";
        return;
    }

    int key = buf.key;
    int rid = buf.rid;
    int hash0 = buf.hash0;
    int hash1 = buf.hash1;
    int index0 = scan(hash0);
    int index1 = scan(hash1);
    
    if (has_joined(key, hash0, hash1, rid) != -1) {
        return;
    }
    
    if (index0 != -1) {
        hash_table[hash0][index0].status = 1;
        hash_table[hash0][index0].tag = hash1;
        hash_table[hash0][index0].head[0] = buf;
    } else if (index1 != -1) {
        hash_table[hash1][index1].status = 1;
        hash_table[hash1][index1].tag = hash0;
        hash_table[hash1][index1].head[0] = buf;
    } else {
        // which slot to evict? paper does not specify
        if (cnt%2 == 0) {
            struct buffer temp = hash_table[hash0][index0].head[0];
            hash_table[hash0][index0].tag = hash1;
            hash_table[hash0][index0].head[0] = buf;
            insert(temp, cnt + 1);
        } else {
            struct buffer temp = hash_table[hash1][index1].head[0];
            hash_table[hash1][index1].tag = hash0;
            hash_table[hash1][index1].head[0] = buf;
            insert(temp, cnt + 1);
        }
    }
	return;
}

void print_addr_table () {
    int key, rid1, rid2;
    for (int i = 0; i < SIZE; i++) {
        key = addr_table[i].key;
        rid1 = addr_table[i].rid1;;
        rid2 = addr_table[i].rid2;;
        cout<<"Key: "<<key<<" rid1: "<<rid1<<" rid2: "<<rid2<<"\n";
    }
    return;
}

/*
    Test: join these 2 tables
    
    table 1
    key | rid1
    1    0
    3    1
    5    2
    7    3
    
    table 2
    key | rid2
    2    0
    4    1
    5    2
    3    3
    
*/



int main() {
    insert(buffer(1, 0), 0);
    insert(buffer(3, 1), 0);
    insert(buffer(5, 2), 0);
    insert(buffer(7, 3), 0);
    insert(buffer(2, 0), 0);
    insert(buffer(4, 1), 0);
    insert(buffer(5, 2), 0);
    insert(buffer(3, 3), 0);
    print_addr_table();
    return 0;
}
















