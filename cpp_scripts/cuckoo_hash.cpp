#include "cuckoo_hash.h"

using namespace std;

/*
	Fig. 2 Dataflow:
	input -> 
        1. buffer & hash -> 
        2. hash table -> 
        3. address table -> 
        stream out
*/

int first_free_index_addr_table = 0;
struct address_table addr_table[SIZE];
struct table hash_table[R][C];

int hash_function(int function, int key) {
  switch (function) {
    case 0: return key % R;
    case 1: return (key / R) % R;
  }
  return INT_MIN; // illegal
}


int scan(int row) {
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

int compare(int row, int key) {
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

int probe(struct buffer buf) {
  int key = buf.key;
  int rid = buf.rid;
  int hash0 = buf.hash0;
  int hash1 = buf.hash1;
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

void build(struct buffer buf, int cnt) {
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

  // int key = buf.key;
  // int rid = buf.rid;
  int hash0 = buf.hash0;
  int hash1 = buf.hash1;
  int index0 = scan(hash0);
  int index1 = scan(hash1);
  
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
      index0 = 0;
      struct buffer temp = hash_table[hash0][index0].head[0];
      hash_table[hash0][index0].tag = hash1;
      hash_table[hash0][index0].head[0] = buf;
      build(temp, cnt + 1);
    } else {
      index1 = 0;
      struct buffer temp = hash_table[hash1][index1].head[0];
      hash_table[hash1][index1].tag = hash0;
      hash_table[hash1][index1].head[0] = buf;
      build(temp, cnt + 1);
    }
  }
  return;
}

void print_addr_table() {
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


/*
int main() {
    build(buffer(1, 0, hash_function(0, 1), hash_function(1, 1)), 0);
    build(buffer(3, 1, hash_function(0, 3), hash_function(1, 3)), 0);
    build(buffer(5, 2, hash_function(0, 5), hash_function(1, 5)), 0);
    build(buffer(7, 3, hash_function(0, 7), hash_function(1, 7)), 0);
    probe(buffer(2, 0, hash_function(0, 2), hash_function(1, 2)));
    probe(buffer(4, 1, hash_function(0, 4), hash_function(1, 4)));
    probe(buffer(5, 2, hash_function(0, 5), hash_function(1, 5)));
    probe(buffer(3, 3, hash_function(0, 3), hash_function(1, 3)));
    print_addr_table();
    return 0;
}
*/
















