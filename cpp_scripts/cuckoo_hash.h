#include <iostream>
#define R 10 // number of rows in a hash table
#define C 4  // number of slots in a row
#define T 20 // size of the list of tuples, not sure how big this should be
#define MAX 200 // max number of time of hashing, for cycle detection
#define SIZE 10 // size of the address table

int hash_function(int function, int key);
int scan(int row);
int compare(int row, int key);
int has_joined(int key, int hash0, int hash1, int rid);
void insert(struct buffer buf, int cnt);
void print_addr_table();
