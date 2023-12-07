// Core functions implementing a stream key-value aggregator as
// described in "FPGA for Aggregate Processing: The Good, The Bad, and
// The Ugly".

#ifndef CORE_H
#define CORE_H

#include <cstdio>
#include <cstdlib>
#ifndef CXXONLY
#include <sycl/sycl.hpp>
#endif

#include "types.h"

void bucket_scan(HashEntry bucket[], int &slot, BOOL &success, int SLOTS) {
  [[intel::disable_loop_pipelining]] for (int s = 0; s < SLOTS; s++) {
    if (!bucket[s].full) {
      slot = s;
      success = true;
      return;
    }
  }
}

void bucket_copy(HashEntry table_row[], HashEntry bucket[], int SLOTS) {
  #pragma unroll
  for (int s = 0; s < SLOTS; s++) {
    bucket[s /*& (int) (SLOTS-1)*/] = table_row[s /*& (int) (SLOTS-1)*/];
  }
}

void compare(HashEntry bucket[], int &slot, JoinKey &key, BOOL &success, int SLOTS) {
  [[intel::disable_loop_pipelining]] for (int s = 0; s < SLOTS; s++) {
    if (bucket[s].key == key && bucket[s].full) {
      slot = s;
      success = true;
      return;
    }
  }
}

#endif
