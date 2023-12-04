#include <iostream>
#include <climits>
#define R 10 // number of rows in a hash table
#define C 4  // number of slots in a row
#define MAX 200 // max number of time of hashing, for cycle detection
#define SIZE 10 // size of the address table

struct address_table {
  int rid1 = INT_MIN;
  int rid2 = INT_MIN;
  int key = INT_MIN;
  //address_table(){};
  //address_table (int rid1, int rid2, int key) {
  //  this->rid1 = rid1;
  //  this->rid2 = rid2;
  //  this->key = key;
  //}
};

struct buffer {
  // this is the 'tuple' from the paper
  int rid;
  int key;
  int hash0;
  int hash1;
  //buffer(){};
  //buffer (int key, int rid, int hash0, int hash1) {
  //  this->key = key;
  //  this->rid = rid;
  //  this->hash0 = hash0;
  //  this->hash1 = hash1;
  //}
};

struct table {
  int status = 0; 	       // indicates whether the slot is occupied
  int tag = 0;             // candidate bucket number of this element; this slightly differs from paper
  struct buffer head[R];      // basically the collision list. 
  //table(){};
  //table (int status, int tag, struct buffer head, int index) {
  //  this->status = status;
  //  this->tag = tag;
  //  this->head[index] = head;
  //}
};

int hash_function(int function, int key);
int scan(int row);
int compare(int row, int key);
int probe(struct buffer buf);
void build(struct buffer buf, int cnt);
void print_addr_table();
void print_hash_table();
int find_free_collision_list_spot (struct buffer buf[]);
