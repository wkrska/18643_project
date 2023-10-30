#include <iostream>
#define R 10 // number of rows in a hash table
#define C 4  // number of slots in a row
#define T 20 // size of the list of tuples, not sure how big this should be
#define MAX 200 // max number of time of hashing, for cycle detection
#define SIZE 10 // size of the address table

//int hash_function(int function, int key);

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

  //buffer (int key, int rid) {
  //  this->rid = rid;
  //  this->key = key;
  //  this->hash0 = hash_function(0, key);
  //  this->hash1 = hash_function(1, key);
  //}
  buffer (int key, int rid, int hash0, int hash1) {
    this->key = key;
    this->rid = rid;
    this->hash0 = hash0;
    this->hash1 = hash1;
  }
};

struct table {
  int status = 0; 	 // indicates whether the slot is occupied
  int tag = 0;             // candidate bucket number of this element
  struct buffer head[T];   // the entry point to the first tuple of this slot
};

int hash_function(int function, int key);
int scan(int row);
int compare(int row, int key);
int has_joined(int key, int hash0, int hash1, int rid);
void insert(struct buffer buf, int cnt);
void print_addr_table();
