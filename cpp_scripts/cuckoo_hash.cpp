#include "cuckoo_hash.h"

/*
	Fig. 2 Dataflow:
	input -> 
        1. buffer & hash -> 
        2. hash table -> 
        3. address table -> 
        stream out
*/

uint32_t first_free_index_addr_table = 0;
struct address_table addr_table[SIZE];
struct table hash_table[R][C];

uint32_t hash_function(uint32_t function, uint32_t key) {
  switch (function) {
    case 0: return key % R;
    case 1: return (key / R) % R;
  }
  return 0; // illegal
}


uint32_t scan(uint32_t row) {
  /*
  purpose: find the first free slot of a given row/bucket
           return INT_MIN if all slots are full
  */

  for (uint32_t i = 0; i < C; i++) {
    if (hash_table[row][i].status == 0) {
      return i;
    }
  }
  return 0;
}

uint32_t compare(uint32_t row, uint32_t key) {
  /*
  purpose: check if the given key is present in the given row
  
  */
  for (uint32_t i = 0; i < C; i++) {
    if (hash_table[row][i].status == 1) {
      if (hash_table[row][i].head[0].key == key) {
        return i;
      }
    }
  }
  return 0;
}

uint32_t probe(struct buffer buf) {
  uint32_t key = buf.key;
  uint32_t rid = buf.rid;
  uint32_t hash0 = buf.hash0;
  uint32_t hash1 = buf.hash1;
  uint32_t index0 = compare(hash0, key);
  uint32_t index1 = compare(hash1, key);

  if (index0 != 0) {
    struct address_table new_entry;
    new_entry.rid1 = hash_table[hash0][index0].head[0].rid;
    new_entry.rid2 = rid;
    new_entry.key = key;
    addr_table[first_free_index_addr_table] = new_entry;
    first_free_index_addr_table++;
    return 1;
  }
  if (index1 != 0) {
    struct address_table new_entry;
    new_entry.rid1 = hash_table[hash1][index1].head[0].rid;
    new_entry.rid2 = rid;
    new_entry.key = key;
    addr_table[first_free_index_addr_table] = new_entry;
    first_free_index_addr_table++;
    return 1;
  }
  return 0;
}

void build(struct buffer buf, uint32_t cnt) {
  
  if (cnt == MAX) {
    // cnt counts how many times this function has been called
    // when cnt == MAX, we treat that as a cycle
    std::cout<<"There might be an inifinte loop!!\n";
    return;
  }

  // int key = buf.key;
  // int rid = buf.rid;
  uint32_t hash0 = buf.hash0;
  uint32_t hash1 = buf.hash1;
  uint32_t index0 = scan(hash0);
  uint32_t index1 = scan(hash1);
  
  if (index0 != 0) {
    struct table new_table;
    new_table.status = 1;
    new_table.tag = hash1;
    new_table.head[0] = buf;
    hash_table[hash0][index0] = new_table;
  } else if (index1 != 0) {
    struct table new_table;
    new_table.status = 1;
    new_table.tag = hash0;
    new_table.head[0] = buf;
    hash_table[hash1][index1] = new_table;
  } else {
    // try eviction, else chain stuff
    uint32_t is_evicted = 0;
    uint32_t i = 0;
    while (i < C && is_evicted == 0) {
      uint32_t free_slot = scan(hash_table[hash0][i].tag);
      if (free_slot != 0) {
	struct table new_table1;
        new_table1.status = 1;
        new_table1.tag = hash0;
        new_table1.head[0] = hash_table[hash0][i].head[0];
        hash_table[hash_table[hash0][i].tag][free_slot] = new_table1;
	struct table new_table2;
        new_table2.status = 1;
        new_table2.tag = hash1;
        new_table2.head[0] = buf;
        hash_table[hash0][i] = new_table2;
        is_evicted = 1;
      }
      i++;
    }
    i = 0;
    while (i < C && is_evicted == 0) {
      uint32_t free_slot = scan(hash_table[hash1][i].tag);
      if (free_slot != 0) {
	struct table new_table1;
        new_table1.status = 1;
        new_table1.tag = hash1;
        new_table1.head[0] = hash_table[hash1][i].head[0];
        hash_table[hash_table[hash1][i].tag][free_slot] = new_table1;
	struct table new_table2;
        new_table2.status = 1;
        new_table2.tag = hash0;
        new_table2.head[0] = buf;
        hash_table[hash1][i] = new_table2;
        is_evicted = 1;
      }
      i++;
    }
    if (is_evicted == 0) {
      // ideally this does not happen
      uint32_t ind = find_free_collision_list_spot(hash_table[hash0][index0].head);
      hash_table[hash0][index0].head[ind] = buf;
    }
  }
  return;
}

void print_addr_table() {
  uint32_t key, rid1, rid2;
  std::cout<<"\nADDR TABLE\n";
  for (uint32_t i = 0; i < SIZE; i++) {
    key = addr_table[i].key;
    rid1 = addr_table[i].rid1;;
    rid2 = addr_table[i].rid2;;
    //if (key != 0) {
      std::cout<<"Key: "<<key<<" rid1: "<<rid1<<" rid2: "<<rid2<<"\n";
    //}
  }
  return;
}

void print_hash_table() {
  std::cout<<"\nHASH TABLE\n";
  //printf("====================\n");
  //for (uint32_t i = 0; i < R; i++) {
  //  for (uint32_t j = 0; j < C; j++) {
  //    if (hash_table[i][j].status == 1) {
  //      std::cout<<hash_table[i][j].head[0].key;
  //      if (hash_table[i][j].head[0].key < 100 and hash_table[i][j].head[0].key > 9) {
  //        std::cout<<" |";
  //      } else if (hash_table[i][j].head[0].key < 10) {
  //        std::cout<<"  |";
  //      } else {
  //        std::cout<<"|";
  //      }
  //    } else {
  //      std::cout<<"   |";
  //    }
  //  }
  //  std::cout<<std::endl;
  //}
  return;
}

uint32_t find_free_collision_list_spot (struct buffer buf[]) {
  for (uint32_t i = 0; i < R; i++) {
    if (buf[i].key == 0) {
      return i;
    }
  }
  return 0;
}
