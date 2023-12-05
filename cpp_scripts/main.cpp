#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstdint>

#include "cuckoo_hash.h"
#include "hash_functions.h"

struct table_data_t {
  int *arr;
  int xdim;
  int ydim;
};

void print_table_data(table_data_t t) {
  printf("====================\n");
  for (int i = 0; i < t.ydim; i++) {
    for (int j = 0; j < t.xdim; j++) {
      printf("%3d ", t.arr[i * t.xdim + j]);
    }
    if (i == 0) {
      printf("\n====================");
    }
    printf("\n");
  }
  printf("\n");
}

table_data_t parse_table_file(char *filename, int xdim, int ydim) {
  std::cout << filename << " xdim: " << xdim << ", ydim: " << ydim << std::endl;
  int *arr = (int *) malloc(xdim * ydim * sizeof(int));
  std::fstream fileStream;
  fileStream.open(filename);
  int holder;
  for (int i = 0; i < ydim; i++) {
    for (int j = 0; j < xdim; j++) {
      fileStream >> holder;
      arr[i * xdim + j] = holder;
    }
  }
  fileStream.close();

  table_data_t table;
  table.arr = arr;
  table.xdim = xdim;
  table.ydim = ydim;

  return table;
}

#define HASHTEST

int main(int argc, char *argv[])
{
#ifndef HASHTEST
  if (argc != 11) {
    std::cout << "ARGS ERROR" << std::endl;
    std::cout << "./main <t1_filename> <t1_xdim> <t1_ydim> <t2_filename> <t2_xdim> <t2_ydim> <t3_filename> <t3_xdim> <t3_ydim> <join_column_value>" << std::endl;
    return 1;
  }

  std::cout<<"\nJoin on the column named "<<argv[10]<<std::endl<<"First row is the column names\n";
  int t1_xdim = strtol(argv[2], nullptr, 0);
  int t1_ydim = strtol(argv[3], nullptr, 0);
  table_data_t t1_data = parse_table_file(argv[1], t1_xdim, t1_ydim);
  std::cout << "TABLE 1" << std::endl;
  print_table_data(t1_data);

  int t2_xdim = strtol(argv[5], nullptr, 0);
  int t2_ydim = strtol(argv[6], nullptr, 0);
  table_data_t t2_data = parse_table_file(argv[4], t2_xdim, t2_ydim);
  std::cout << "TABLE 2" << std::endl;
  print_table_data(t2_data);

  int join_column_value = strtol(argv[10], nullptr, 0);
  int t1_join_column_index;
  int t2_join_column_index;
  for (int i = 0; i < t1_data.xdim; i++) {
    if (t1_data.arr[i] == join_column_value) {
      t1_join_column_index = i;
    }
  }
  for (int i = 0; i < t2_data.xdim; i++) {
    if (t2_data.arr[i] == join_column_value) {
      t2_join_column_index = i;
    }
  }

  for (int i = 1; i < t1_data.ydim; i++) {
    int key = t1_data.arr[i * t1_data.xdim + t1_join_column_index];
    struct buffer new_buf;
    new_buf.key = key;
    new_buf.rid = i;
    new_buf.hash0 = hash_function(0, key);
    new_buf.hash1 = hash_function(1, key);
    build(new_buf, 0);
  }
  for (int i = 1; i < t2_data.ydim; i++) {
    int key = t2_data.arr[i * t2_data.xdim + t2_join_column_index];
    struct buffer new_buf;
    new_buf.key = key;
    new_buf.rid = i;
    new_buf.hash0 = hash_function(0, key);
    new_buf.hash1 = hash_function(1, key);
    probe(new_buf);
  }
  print_addr_table();
  print_hash_table();
  return 0;
#endif

#ifdef HASHTEST
  uint32_t testing = 42069;

  uint32_t testing_hash = djb2(&testing, 1);
  std::cout << "testing djb2 hash: " << testing_hash << std::endl;

  testing_hash = sbdm(&testing, 1);
  std::cout << "testing sbdm hash: " << testing_hash << std::endl;

  testing_hash = magic_int_hash(testing);
  std::cout << "testing magic int hash: " << testing_hash << std::endl;
#endif
}
