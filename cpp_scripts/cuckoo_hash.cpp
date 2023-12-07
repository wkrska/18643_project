#include "cuckoo_hash.h"

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
           return INT_MIN if all slots are full
  */

  for (int i = 0; i < C; i++) {
    if (hash_table[row][i].status == 0) {
      return i;
    }
  }
  return INT_MIN;
}

int compare(int row, int key) {
  /*
  purpose: check if the given key is present in the given row
  
  */
  for (int i = 0; i < C; i++) {
    if (hash_table[row][i].status == 1) {
      if (hash_table[row][i].head[0].key == key) {
        return i;
      }
    }
  }
  return INT_MIN;
}

int probe(struct buffer buf) {
  int key = buf.key;
  int rid = buf.rid;
  int hash0 = buf.hash0;
  int hash1 = buf.hash1;
  int index0 = compare(hash0, key);
  int index1 = compare(hash1, key);

  if (index0 != INT_MIN) {
    addr_table[first_free_index_addr_table] = address_table(hash_table[hash0][index0].head[0].rid, rid, key);
    first_free_index_addr_table++;
    return 1;
  }
  if (index1 != INT_MIN) {
    addr_table[first_free_index_addr_table] = address_table(hash_table[hash1][index1].head[0].rid, rid, key);
    first_free_index_addr_table++;
    return 1;
  }
  return INT_MIN;
}

void build(struct buffer buf, int cnt) {
  
  if (cnt == MAX) {
    // cnt counts how many times this function has been called
    // when cnt == MAX, we treat that as a cycle
    std::cout<<"There might be an inifinte loop!!\n";
    return;
  }

  // int key = buf.key;
  // int rid = buf.rid;
  int hash0 = buf.hash0;
  int hash1 = buf.hash1;
  int index0 = scan(hash0);
  int index1 = scan(hash1);
  
  if (index0 != INT_MIN) {
    hash_table[hash0][index0] = table(1, hash1, buf, 0);
  } else if (index1 != INT_MIN) {
    hash_table[hash1][index1] = table(1, hash0, buf, 0);
  } else {
    // try eviction, else chain stuff
    int is_evicted = 0;
    int i = 0;
    while (i < R && is_evicted == 0) {
      int free_slot = scan(hash_table[hash0][i].tag);
      if (free_slot != INT_MIN) {
        hash_table[hash_table[hash0][i].tag][free_slot] = table(1, hash0, hash_table[hash0][i].head[0], 0);
        hash_table[hash0][i] = table(1, hash1, buf, 0);
        is_evicted = 1;
      }
      i++;
    }
    i = 0;
    while (i < R && is_evicted == 0) {
      int free_slot = scan(hash_table[hash1][i].tag);
      if (free_slot != INT_MIN) {
        hash_table[hash_table[hash1][i].tag][free_slot] = table(1, hash1, hash_table[hash1][i].head[0], 0);
        hash_table[hash_table[hash1][i].tag][free_slot].status = 1;
        hash_table[hash_table[hash1][i].tag][free_slot].tag = hash1;
        hash_table[hash1][i].tag = hash0;
        hash_table[hash1][i].head[0] = buf;
        is_evicted = 1;
      }
      i++;
    }
    if (is_evicted == 0) {
      // ideally this does not happen
      int ind = find_free_collision_list_spot(hash_table[hash0][index0].head);
      hash_table[hash0][index0].head[ind] = buf;
    }
  }
  return;
}

void print_addr_table() {
  int key, rid1, rid2;
  std::cout<<"\nADDR TABLE\n";
  for (int i = 0; i < SIZE; i++) {
    key = addr_table[i].key;
    rid1 = addr_table[i].rid1;;
    rid2 = addr_table[i].rid2;;
    if (key != INT_MIN)  {
      std::cout<<"Key: "<<key<<" rid1: "<<rid1<<" rid2: "<<rid2<<"\n";
    }
  }
  return;
}

void print_hash_table() {
  std::cout<<"\nHASH TABLE\n";
printf("====================\n");
  for (int i = 0; i < R; i++) {
    for (int j = 0; j < C; j++) {
      if (hash_table[i][j].status == 1) {
        std::cout<<hash_table[i][j].head[0].key;
if (hash_table[i][j].head[0].key < 100 and hash_table[i][j].head[0].key > 9) {
          std::cout<<" |";
        } else if (hash_table[i][j].head[0].key < 10) {
          std::cout<<"  |";
        } else {
          std::cout<<"|";
        }
      } else {
        std::cout<<"   |";
      }
          }
    std::cout<<std::endl;
  }
  return;
}

int find_free_collision_list_spot (struct buffer buf[]) {
  for (int i = 0; i < R; i++) {
    if (buf[i].key == INT_MIN) {
      return i;
    }
  }
  return INT_MIN;
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
