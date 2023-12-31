#include <iostream>
#include <climits>
#define R 128 // number of rows in a hash table
#define C 2000  // number of slots in a row
#define MAX 2000 // max number of time of hashing, for cycle detection
#define SIZE 2000
// size of the address table

struct address_table {
  int rid1 = INT_MIN;
  int rid2 = INT_MIN;
  int key = INT_MIN;
};

struct buffer {
  // this is the 'tuple' from the paper
  int rid;
  int key;
  int hash0;
  int hash1;
};

struct table {
  int status = 0; 	       // indicates whether the slot is occupied
  int tag = 0;             // candidate bucket number of this element; this slightly differs from paper
  struct buffer head[R];      // basically the collision list. 
};

int hash_function(int function, int key);
int scan(int row);
int compare(int row, int key);
int probe(struct buffer buf);
void build(struct buffer buf, int cnt);
void print_addr_table();
void print_hash_table();
void evaluate_hash_table();
int find_free_collision_list_spot (struct buffer buf[]);
