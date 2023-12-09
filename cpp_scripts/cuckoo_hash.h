#include <iostream>
#include <climits>
#define R 16 // number of rows in a hash table
#define C 1000  // number of slots in a row
#define MAX 1000 // max number of time of hashing, for cycle detection
#define SIZE 16
// size of the address table

struct address_table {
  uint32_t rid1 = 0;
  uint32_t rid2 = 0;
  uint32_t key = 0;
};

struct buffer {
  // this is the 'tuple' from the paper
  uint32_t rid;
  uint32_t key;
  uint32_t hash0;
  uint32_t hash1;
};

struct table {
  uint32_t status = 0; 	       // indicates whether the slot is occupied
  uint32_t tag = 0;             // candidate bucket number of this element; this slightly differs from paper
  struct buffer head[R];      // basically the collision list. 
};

uint32_t hash_function(uint32_t function, uint32_t key);
uint32_t scan(uint32_t row);
uint32_t compare(uint32_t row, uint32_t key);
uint32_t probe(struct buffer buf);
void build(struct buffer buf, uint32_t cnt);
void print_addr_table();
void print_hash_table();
uint32_t find_free_collision_list_spot (struct buffer buf[]);
