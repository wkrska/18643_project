#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstdint>

#include "cuckoo_hash.h"
#include "hash_functions.h"


struct table_data_t {
  uint32_t *arr;
  uint32_t xdim;
  uint32_t ydim;
};

void print_table_data(table_data_t t) {
  printf("====================\n");
  for (uint32_t i = 0; i < t.ydim; i++) {
    for (uint32_t j = 0; j < t.xdim; j++) {
      printf("%9u ", t.arr[i * t.xdim + j]);
    }
    if (i == 0) {
      printf("\n====================");
    }
    printf("\n");
  }
  printf("\n");
}

table_data_t parse_table_file(char *filename, uint32_t xdim, uint32_t ydim) {
  std::cout << filename << " xdim: " << xdim << ", ydim: " << ydim << std::endl;
  uint32_t *arr = (uint32_t *) malloc(xdim * ydim * sizeof(uint32_t));
  std::fstream fileStream;
  fileStream.open(filename);
  uint32_t holder;
  for (uint32_t i = 0; i < ydim; i++) {
    for (uint32_t j = 0; j < xdim+1; j++) {
      fileStream >> holder;
      if (j != 0) {
        arr[i * xdim + j - 1] = holder;
      }
    }
  }
  fileStream.close();

  table_data_t table;
  table.arr = arr;
  table.xdim = xdim;
  table.ydim = ydim;
  print_table_data(table);

  return table;
}

//#define HASHTEST

int main(int argc, char *argv[])
{
#ifndef HASHTEST
  if (argc != 8) {
    std::cout << "ARGS ERROR" << std::endl;
    std::cout << "./main <t1_filename> <t1_xdim> <t1_ydim> <t2_filename> <t2_xdim> <t2_ydim> <join_column>" << std::endl;
    return 1;
  }

  std::cout << "\nJoin on the column named " << argv[7] << std::endl;
  uint32_t t1_xdim = strtoul(argv[2], nullptr, 0);
  uint32_t t1_ydim = strtoul(argv[3], nullptr, 0);
  table_data_t t1_data = parse_table_file(argv[1], t1_xdim, t1_ydim);
  std::cout << "TABLE 1" << std::endl;
  print_table_data(t1_data);

  uint32_t t2_xdim = strtoul(argv[5], nullptr, 0);
  uint32_t t2_ydim = strtoul(argv[6], nullptr, 0);
  table_data_t t2_data = parse_table_file(argv[4], t2_xdim, t2_ydim);
  std::cout << "TABLE 2" << std::endl;
  print_table_data(t2_data);

  uint32_t join_column_value = strtoul(argv[7], nullptr, 0);
  uint32_t t1_join_column_index;
  uint32_t t2_join_column_index;
  for (uint32_t i = 0; i < t1_data.xdim; i++) {
    if (t1_data.arr[i] == join_column_value) {
      t1_join_column_index = i;
    }
  }
  for (uint32_t i = 0; i < t2_data.xdim; i++) {
    if (t2_data.arr[i] == join_column_value) {
      t2_join_column_index = i;
    }
  }

  for (uint32_t i = 1; i < t1_data.ydim; i++) {
    uint32_t key = t1_data.arr[i * t1_data.xdim + t1_join_column_index];
    struct buffer new_buf;
    new_buf.key = key;
    new_buf.rid = i;
    new_buf.hash0 = hash_function(0, key);
    new_buf.hash1 = hash_function(1, key);
    build(new_buf, 0);
  }
  for (uint32_t i = 1; i < t2_data.ydim; i++) {
    uint32_t key = t2_data.arr[i * t2_data.xdim + t2_join_column_index];
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
#else
  uint32_t testing = 42069;

  uint32_t testing_hash = djb2(&testing, 1);
  std::cout << "testing djb2 hash: " << testing_hash << std::endl;

  testing_hash = sbdm(&testing, 1);
  std::cout << "testing sbdm hash: " << testing_hash << std::endl;

  testing_hash = magic_int_hash(testing);
  std::cout << "testing magic int hash: " << testing_hash << std::endl;

  testing_hash = hash32shift(testing);
  std::cout << "testing hash32shift hash: " << testing_hash << std::endl;

  testing_hash = jenkins32hash(testing);
  std::cout << "testing jenkins32hash hash: " << testing_hash << std::endl;

  testing_hash = hash32shiftmult(testing);
  std::cout << "testing hash32shiftmult hash: " << testing_hash << std::endl;
#endif
}
