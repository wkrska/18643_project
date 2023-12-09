// SETUP TEST CONDITION
#if 1 // Use default/makefile defined values
  #ifndef _BKTS_
    #define _BKTS_ 1024
  #endif
  #ifndef _SLTS_
    #define _SLTS_ 4
  #endif
  #ifndef _PPLN_
    #define _PPLN_ 1
  #endif
  #define BundleDepth 8
#else // Use one time/user defined values
  #define _BKTS_ 16
  #define _SLTS_ 4
  #define _PPLN_ 2
  #define BundleDepth 8
#endif

#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sycl/sycl.hpp>
using namespace sycl;

#include "core.h"
#include "debug.h"
#include "pipes.h"
#include "stages.h"
#include <sys/time.h>
#include "types.h"

//////////////////////////////////////////////////////////////////////////////
// SYCL boiler plate
//////////////////////////////////////////////////////////////////////////////

#if FPGA_EMULATOR || FPGA_HARDWARE || FPGA_SIMULATOR
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif

//////////////////////////////////////////////////////////////////////////////
// Kernel exception handler Just rethrows exceptions and terminates
// "gracefully".
//////////////////////////////////////////////////////////////////////////////

static auto exception_handler = [](sycl::exception_list e_list) {
  for (std::exception_ptr const &e : e_list) {
    try {
      std::rethrow_exception(e);
    } catch (std::exception const &e) {
#if _DEBUG
      std::cout << "Failure" << std::endl;
#endif
      std::terminate();
    }
  }
};

//////////////////////////////////////////////////////////////////////////////
// prepare test pattern buffers
//////////////////////////////////////////////////////////////////////////////
void parse_table_file(JoinKey *table, char *filename, int xdim, int ydim) {
  std::cout << "PARSE_TABLE_FILE" << std::endl;
  std::cout << filename << " xdim: " << xdim << ", ydim: " << ydim << std::endl;
  std::fstream fileStream;
  fileStream.open(filename);
  uint32_t holder;
  for (int i = 0; i < ydim; i++) {
    for (int j = 0; j < xdim+1; j++) {
      fileStream >> holder;
      if (j != 0) {
        table[i * xdim + j - 1] = holder;
      }
    }
  }
  fileStream.close();
  return;
}

int main(int argc, char *argv[]) {
  // Stuff you just need for SYCL, leave this here
  #if FPGA_SIMULATOR
    auto selector = sycl::ext::intel::fpga_simulator_selector_v;
  #elif FPGA_HARDWARE
    auto selector = sycl::ext::intel::fpga_selector_v;
  #elif FPGA_EMULATOR
    auto selector = sycl::ext::intel::fpga_emulator_selector_v;
  #else
    auto selector = default_selector_v;
  #endif

  queue q(selector, exception_handler);
  {
    // make sure the device supports USM device allocations
    auto device = q.get_device();

    std::cout << "Running on device: "
              << device.get_info<info::device::name>().c_str() << std::endl;
  }

  ///////////////////////////
  // Setup Hash Functions
  ///////////////////////////
  struct HashFunc {
    static HashVal hash0(JoinKey key) {
      HashVal hash = key % (_BKTS_ * _PPLN_);
      return hash;
    }

    static HashVal hash1(JoinKey key) {
      HashVal hash = (key / _BKTS_) % (_BKTS_ * _PPLN_);
      return hash;
    }

    static HashVal djb2(JoinKey key) {
      JoinKey hash = 5381;
      hash = ((hash << 5) + hash) + key;
      hash = hash % (_BKTS_ * _PPLN_);
      return (HashVal) hash;
    }

    static HashVal magic_int_hash(JoinKey key) {
      JoinKey key_t = key;
      key = ((key >> 16) ^ key) * 0x45d9f3b;
      key = ((key >> 16) ^ key) * 0x45d9f3b;
      key = (key >> 16) ^ key;
      key = key % (_BKTS_ * _PPLN_);
      return (HashVal) key;
    }

    static HashVal hash32shift(JoinKey key) {
      JoinKey key_t = key;
      int skey;
      skey = ~((int)key) + (((int)key) << 15);
      key = ((JoinKey)skey) ^ (((JoinKey)skey) >> 12);
      skey = ((int)key) + (((int)key) << 2);
      skey = ((JoinKey)skey) ^ (((JoinKey)skey) >> 4);
      skey = skey * 2057;
      key = ((JoinKey)skey) ^ (((JoinKey)skey) >> 16);
      key = key % (_BKTS_ * _PPLN_);
      return (HashVal) key;
    }

    static HashVal jenkins32hash(JoinKey key) {
      JoinKey key_t = key;
      key = (key+0x7ed55d16) + (key<<12);
      key = (key^0xc761c23c) ^ (key>>19);
      key = (key+0x165667b1) + (key<<5);
      key = (key+0xd3a2646c) ^ (key<<9);
      key = (key+0xfd7046c5) + (key<<3);
      key = (key^0xb55a4f09) ^ (key>>16);
      key = key % (_BKTS_ * _PPLN_);
      return (HashVal) key;
    }

    static HashVal hash32shiftmult(JoinKey key) {
      JoinKey key_t = key;
      int skey;
      JoinKey c2=0x27d4eb2d;
      key = (key ^ 61) ^ (key >> 16);
      skey = ((int)key) + (((int)key) << 3);
      key = ((JoinKey)skey) ^ (((JoinKey)skey) >> 4);
      key = key * c2;
      key = key ^ (key >> 15);
      key = key % (_BKTS_ * _PPLN_);
      return (HashVal) key;
    }
  };

  //////////////////////
  // Set up test input
  //////////////////////
  if (argc != 11) {
    std::cout << "ARGS ERROR" << std::endl;
    std::cout << "./main <t1_xdim> <t1_ydim> <t2_xdim> <t2_ydim> <t1_filename> <t2_filename> <join_column_value> <data_source_select> <hash_method_select> <timing_only>" << std::endl;
    return 1;
  }
  int t1_xdim = strtol(argv[1], nullptr, 0);
  int t1_ydim = strtol(argv[2], nullptr, 0);
  int t2_xdim = strtol(argv[3], nullptr, 0);
  int t2_ydim = strtol(argv[4], nullptr, 0);
  int join_column_value = atoi(argv[7]); 
  int data_src_sel = strtol(argv[8], nullptr, 0);
  HashSel hash_sel[1];
  hash_sel[0] = strtol(argv[9], nullptr, 0);
  int timing_only = strtol(argv[10], nullptr, 0);

  JoinKey t1_data_host[t1_xdim*t1_ydim];
  JoinKey t2_data_host[t2_xdim*t2_ydim];

  JoinKey c1_data_host[t1_ydim];
  JoinKey c2_data_host[t2_ydim];

  int t1_join_column_index = 0;
  int t2_join_column_index = 0;

  printf("Params:\nBKTS: %d\nSLTS: %d\nPPLN: %d\n", _BKTS_, _SLTS_, _PPLN_);
  printf("t1_xdim: %d\nt1_ydim: %d (max %d)\nt2_xdim: %d\nt2_ydim: %d\n", t1_xdim, t1_ydim, (_PPLN_*_BKTS_*_SLTS_), t2_xdim, t2_ydim);
  printf("join_column_value: %d\ndata_source_select: %d\nhash_method_select: %d\ntiming_only: %s\n\n", join_column_value, data_src_sel, hash_sel[0], (timing_only)?"true":"false");
  fflush(stdout);

  if (data_src_sel == 0) {
    std::cout<<"\nJoin on the column named "<<argv[7]<<std::endl<<"First row is the column names\n";
    parse_table_file(t1_data_host, argv[5], t1_xdim, t1_ydim);
    parse_table_file(t2_data_host, argv[6], t2_xdim, t2_ydim);
    // Locate the data
    for (int i = 0; i < t1_xdim; i++)
      if (t1_data_host[i] == (JoinKey) join_column_value)
        t1_join_column_index = i;

    for (int i = 0; i < t2_xdim; i++)
      if (t2_data_host[i] == (JoinKey) join_column_value)
        t2_join_column_index = i;
  } 
  if (data_src_sel > 0) {// generate dummy data on device for minimized latency, generate same data here for comparison
    int t1_join_column_index = 0;
    int t2_join_column_index = 0;
    srand(0);
    t1_data_host[0] = 0;
    t2_data_host[0] = 0;
    for (int i = 1; i < t1_ydim; i++)
      t1_data_host[i*t1_xdim] = (JoinKey) i;
    if (data_src_sel == 1) { // Random uniform
      for (int i = 1; i < t2_ydim; i++)
        t2_data_host[i*t2_xdim] = (JoinKey) rand() % t1_ydim;
    } else if (data_src_sel == 2) { // Skew left
      double mu = (double) t1_ydim/4;
      double sigma = (double) t1_ydim/4;
      std::default_random_engine generator;
      std::normal_distribution<double> distribution(mu,sigma);
      for (int i = 1; i < t2_ydim; i++)
        t2_data_host[i*t2_xdim] = (JoinKey) int((double)t1_ydim - abs(t1_ydim - (std::max(0.0, mu-(sigma*2)) + abs(distribution(generator) - std::max(0.0, mu-(sigma*2))))));
    } else if (data_src_sel == 3) { // strided
      for (int i = 1; i < t2_ydim; i++)
        t2_data_host[i*t2_xdim] = (JoinKey) rand()*(t1_ydim/_SLTS_) % t1_ydim;
    } else { // i
      for (int i = 1; i < t2_ydim; i++)
        t2_data_host[i*t2_xdim] = (JoinKey) i % t1_ydim;
    }
  }

  // Copy only the column for transmision to the kernel
  for (int i = 0; i < t1_ydim; i++)
    c1_data_host[i] = t1_data_host[i*t1_xdim+t1_join_column_index];
  for (int i = 0; i < t2_ydim; i++)
    c2_data_host[i] = t2_data_host[i*t2_xdim+t2_join_column_index];
  //////////////////////
  // Set up and prepare device memory
  //////////////////////

  // Create space on device
  JoinKey *c1_data_device = malloc_device<JoinKey>(t1_ydim, q);
  JoinKey *c2_data_device = malloc_device<JoinKey>(t2_ydim, q);
  HashSel *hash_sel_device = malloc_device<HashSel>(1, q);
  
  MatchedTuple *MatchedOutput_device = malloc_device<MatchedTuple>(t1_ydim+t2_ydim, q);
  HashedTuple *UnmappedOutput_device = malloc_device<HashedTuple>(t1_ydim, q);
  HashEntry *HashedOutput_device0 = malloc_device<HashEntry>(_BKTS_*_SLTS_, q);
  #if (_PPLN_ > 1)
  HashEntry *HashedOutput_device1 = malloc_device<HashEntry>(_BKTS_*_SLTS_, q);
  #endif
  #if (_PPLN_ > 2)
  HashEntry *HashedOutput_device2 = malloc_device<HashEntry>(_BKTS_*_SLTS_, q);
  #endif
  #if (_PPLN_ > 3)
  HashEntry *HashedOutput_device3 = malloc_device<HashEntry>(_BKTS_*_SLTS_, q);
  #endif
  ClockCounter *ClockCounter_device0 = malloc_device<ClockCounter>(2, q);
  #if (_PPLN_ > 1)
  ClockCounter *ClockCounter_device1 = malloc_device<ClockCounter>(2, q);
  #endif
  #if (_PPLN_ > 2)
  ClockCounter *ClockCounter_device2 = malloc_device<ClockCounter>(2, q);
  #endif
  #if (_PPLN_ > 3)
  ClockCounter *ClockCounter_device3 = malloc_device<ClockCounter>(2, q);
  #endif

  MatchedTuple MatchedOutput_host[t1_ydim+t2_ydim];
  HashedTuple UnmappedOutput_host[t1_ydim];
  HashEntry HashedOutput_host0[_BKTS_*_SLTS_] = {};
  #if (_PPLN_ > 1)
  HashEntry HashedOutput_host1[_BKTS_*_SLTS_] = {};
  #endif
  #if (_PPLN_ > 2)
  HashEntry HashedOutput_host2[_BKTS_*_SLTS_] = {};
  #endif
  #if (_PPLN_ > 3)
  HashEntry HashedOutput_host3[_BKTS_*_SLTS_] = {};
  #endif
  ClockCounter ClockCounter_host0[2] = {};
  #if (_PPLN_ > 1)
  ClockCounter ClockCounter_host1[2] = {};
  #endif
  #if (_PPLN_ > 2)
  ClockCounter ClockCounter_host2[2] = {};
  #endif
  #if (_PPLN_ > 3)
  ClockCounter ClockCounter_host3[2] = {};
  #endif

  // Zero out buffers
  for (int i = 0; i < t1_ydim+t2_ydim; i++) MatchedOutput_host[i].valid = 0;
  for (int i = 0; i < t1_ydim; i++) UnmappedOutput_host[i].valid = 0;
  
  // Copy data from host to device
  q.memcpy(c1_data_device, c1_data_host, t1_ydim * sizeof(JoinKey));
  q.memcpy(c2_data_device, c2_data_host, t2_ydim * sizeof(JoinKey));
  q.memcpy(hash_sel_device, hash_sel, sizeof(HashSel));
  q.wait();
  
  // For everything we do, we run it as a "task". This task can exit or run indefinately

  // Stream in the data
  try {
    // Source Stage: inject testInput
    q.submit([&](handler &h) {
      h.single_task<class testInputSrcTask>([=]() {
        // Tuples are bundled because DRAM reads need more concurrency
        TuplesBundle<BundleDepth> iBundle;
        // Build phase
        [[intel::initiation_interval(1)]] for (int i = 1; i < t1_ydim; i+=BundleDepth) {
          #pragma unroll
          for (int j = 0; j < BundleDepth; j++) {
            iBundle.tuples[j].key = (JoinKey) c1_data_device[i+j];
            iBundle.tuples[j].val = (JoinVal) (i+j);
            iBundle.tuples[j].eof = (bool) ((i+j+1) == t1_ydim);
            if ((i+j) < t1_ydim) {
              iBundle.tuples[j].valid = (bool) 1;
            } else {
              iBundle.tuples[j].valid = (bool) 0;
            }
          }
          iBundle.hashSelect = (HashSel) hash_sel_device[0];
          InputPipe0::write(iBundle);
        }
        
        // Probe phase
        [[intel::initiation_interval(1)]] for (int i = 1; i < t2_ydim; i+=BundleDepth) {
          #pragma unroll
          for (int j = 0; j < BundleDepth; j++) {
            iBundle.tuples[j].key = (JoinKey) c2_data_device[i+j];
            iBundle.tuples[j].val = (JoinVal) (i+j);
            iBundle.tuples[j].eof = (bool) ((i+j+1) == t2_ydim);
            if ((i+j) < t2_ydim) {
              iBundle.tuples[j].valid = (bool) 1;
            } else {
              iBundle.tuples[j].valid = (bool) 0;
            }
          }
          iBundle.hashSelect = (HashSel) hash_sel_device[0];
          InputPipe0::write(iBundle);
        }
      });
    });

    // Hash values
    q.submit([&](handler &h) { h.single_task<class HashTask0>([=]() {
        hashStage<HashFunc, InputPipe0, HashedTuplePipe0, BundleDepth>();});});

    // Fork values
    q.submit([&](handler &h) { h.single_task<class ForkTask0>([=]() {
        forkStage<HashFunc, HashedTuplePipe0, ForkedTuplePipe0, ForkedTuplePipe1, ForkedTuplePipe2, ForkedTuplePipe3>();});});

    // Store hashed values in hash table
    q.submit([&](handler &h) { h.single_task<class HashTableTask0>([=]() {
        hashTableStage<ForkedTuplePipe0, UnmappableTuplePipe0, MatchedTuplePipe0, HashedTablePipe0, HashTableTimingPipe0>();});});

    // Stupid hacky timing method this truly sucks but we need to count clocks
    q.submit([&](handler &h) { h.single_task<class TimerTask0>([=]() {
        timerStage<HashTableTimingPipe0, TimeReportingPipe0>();});});

    #if (_PPLN_ > 1)
    // Store hashed values in hash table
    q.submit([&](handler &h) { h.single_task<class HashTableTask1>([=]() {
        hashTableStage<ForkedTuplePipe1, UnmappableTuplePipe1, MatchedTuplePipe1, HashedTablePipe1, HashTableTimingPipe1>();});});

    // Stupid hacky timing method this truly sucks but we need to count clocks
    q.submit([&](handler &h) { h.single_task<class TimerTask1>([=]() {
        timerStage<HashTableTimingPipe1, TimeReportingPipe1>();});});
    #endif

    #if (_PPLN_ > 2)
    // Store hashed values in hash table
    q.submit([&](handler &h) { h.single_task<class HashTableTask2>([=]() {
        hashTableStage<ForkedTuplePipe2, UnmappableTuplePipe2, MatchedTuplePipe2, HashedTablePipe2, HashTableTimingPipe2>();});});

    // Stupid hacky timing method this truly sucks but we need to count clocks
    q.submit([&](handler &h) { h.single_task<class TimerTask2>([=]() {
        timerStage<HashTableTimingPipe2, TimeReportingPipe2>();});});
    #endif

    #if (_PPLN_ > 3)
    // Store hashed values in hash table
    q.submit([&](handler &h) { h.single_task<class HashTableTask3>([=]() {
        hashTableStage<ForkedTuplePipe3, UnmappableTuplePipe3, MatchedTuplePipe3, HashedTablePipe3, HashTableTimingPipe3>();});});

    // Stupid hacky timing method this truly sucks but we need to count clocks
    q.submit([&](handler &h) { h.single_task<class TimerTask3>([=]() {
        timerStage<HashTableTimingPipe3, TimeReportingPipe3>();});});
    #endif

    #if 0
    // Funnel values
    q.submit([&](handler &h) { h.single_task<class FunnelTask0>([=]() {
        funnelStage<MatchedTuplePipe0, MatchedTuplePipe1, MatchedTuplePipe2, MatchedTuplePipe3, FunnelledMatchedTuplePipe0, MatchedTuple>();});});
    
    // Funnel values
    q.submit([&](handler &h) { h.single_task<class FunnelTask1>([=]() {
        funnelStage<UnmappableTuplePipe0, UnmappableTuplePipe1, UnmappableTuplePipe2, UnmappableTuplePipe3, FunnelledUnmappableTuplePipe0, HashedTuple>();});});
    // write to dram to communicate back to host
    auto ev0 = q.submit([&](handler &h) { h.single_task<class SuccessfulReadoutTask>([=]() {
        // Zero out
        for (int i = 0; i < t1_ydim+t2_ydim; i++) {
          MatchedOutput_device[i].valid = 0;
          MatchedOutput_device[i].key = 0;
          MatchedOutput_device[i].val0 = 0;
          MatchedOutput_device[i].val1 = 0;
        }
        int eof = 0;
        int i = 0;
        while (eof < _PPLN_) {
          bool rValid;
          MatchedTuple temp = FunnelledMatchedTuplePipe0::read(rValid);
          if (rValid) {
            if (temp.valid) {
              MatchedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
        }
      });
    });
    // write to dram to communicate back to host
    auto ev1 = q.submit([&](handler &h) { h.single_task<class UnmappableReadoutTask>([=]() {
        // Zero out
        for (int i = 0; i < t1_ydim; i++) {
          UnmappedOutput_device[i].valid = 0;
          UnmappedOutput_device[i].key = 0;
          UnmappedOutput_device[i].val = 0;
        }
        int eof = 0;
        int i = 0;
        while (eof < _PPLN_) {
          HashedTuple temp = FunnelledUnmappableTuplePipe0::read();
          if (temp.valid) {
            UnmappedOutput_device[i] = temp;
            i++;
          }
          eof += (temp.eof) ? 1 : 0;
        }
      });
    });
    #else
    auto ev0 = q.submit([&](handler &h) { h.single_task<class SuccessfulReadoutTask>([=]() {
        // Zero out
        for (int i = 0; i < t1_ydim+t2_ydim; i++) {
          MatchedOutput_device[i].valid = 0;
          MatchedOutput_device[i].key = 0;
          MatchedOutput_device[i].val0 = 0;
          MatchedOutput_device[i].val1 = 0;
        }
        int eof = 0;
        int i = 0;
        while (eof < _PPLN_) {
          bool rValid;
          MatchedTuple temp = MatchedTuplePipe0::read(rValid);
          if (rValid) {
            if (temp.valid) {
              MatchedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
          #if (_PPLN_ > 1)
          temp = MatchedTuplePipe1::read(rValid);
          if (rValid) {
            if (temp.valid) {
              MatchedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
          #endif
          #if (_PPLN_ > 2)
          temp = MatchedTuplePipe2::read(rValid);
          if (rValid) {
            if (temp.valid) {
              MatchedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
          #endif
          #if (_PPLN_ > 3)
          temp = MatchedTuplePipe3::read(rValid);
          if (rValid) {
            if (temp.valid) {
              MatchedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
          #endif
        }
      });
    });

    auto  ev1 = q.submit([&](handler &h) { h.single_task<class UnmappableReadoutTask>([=]() {
        // Zero out
        for (int i = 0; i < t1_ydim; i++) {
          UnmappedOutput_device[i].valid = 0;
          UnmappedOutput_device[i].key = 0;
          UnmappedOutput_device[i].val = 0;
        }
        int eof = 0;
        int i = 0;
        while (eof < _PPLN_) {
          bool rValid;
          HashedTuple temp = UnmappableTuplePipe0::read(rValid);
          if (rValid) {
            if (temp.valid) {
              UnmappedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
          #if (_PPLN_ > 1)
          temp = UnmappableTuplePipe1::read(rValid);
          if (rValid) {
            if (temp.valid) {
              UnmappedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
          #endif
          #if (_PPLN_ > 2)
          temp = UnmappableTuplePipe2::read(rValid);
          if (rValid) {
            if (temp.valid) {
              UnmappedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
          #endif
          #if (_PPLN_ > 3)
          temp = UnmappableTuplePipe3::read(rValid);
          if (rValid) {
            if (temp.valid) {
              UnmappedOutput_device[i] = temp;
              i++;
            }
            eof += (temp.eof) ? 1 : 0;
          }
          #endif
        }
      });
    });
    #endif

    // write to dram to communicate back to host
    auto ev2_0 = q.submit([&](handler &h) { h.single_task<class HashTableReadoutTask0>([=]() {
        for (int b = 0; b < _BKTS_; b++) {
          for (int s = 0; s < _SLTS_; s++ ) {
            HashedOutput_device0[b*_SLTS_+s]=HashedTablePipe0::read();
    }}});});
    #if (_PPLN_ > 1)
    // write to dram to communicate back to host
    auto ev2_1 = q.submit([&](handler &h) { h.single_task<class HashTableReadoutTask1>([=]() {
        for (int b = 0; b < _BKTS_; b++) {
          for (int s = 0; s < _SLTS_; s++ ) {
            HashedOutput_device1[b*_SLTS_+s]=HashedTablePipe1::read();
    }}});});
    #endif
    #if (_PPLN_ > 2)
    // write to dram to communicate back to host
    auto ev2_2 = q.submit([&](handler &h) { h.single_task<class HashTableReadoutTask2>([=]() {
        for (int b = 0; b < _BKTS_; b++) {
          for (int s = 0; s < _SLTS_; s++ ) {
            HashedOutput_device2[b*_SLTS_+s]=HashedTablePipe2::read();
    }}});});
    #endif
    #if (_PPLN_ > 3)
    // write to dram to communicate back to host
    auto ev2_3 = q.submit([&](handler &h) { h.single_task<class HashTableReadoutTask3>([=]() {
        for (int b = 0; b < _BKTS_; b++) {
          for (int s = 0; s < _SLTS_; s++ ) {
            HashedOutput_device3[b*_SLTS_+s]=HashedTablePipe3::read();
    }}});});
    #endif

    // write to dram to communicate back to host
    auto ev3_0 = q.submit([&](handler &h) { h.single_task<class TimingReadoutTask0>([=]() {
        for (int i = 0; i < 2; i++) {
          ClockCounter_device0[i]=TimeReportingPipe0::read();
    }});});
    #if (_PPLN_ > 1)
    // write to dram to communicate back to host
    auto ev3_1 = q.submit([&](handler &h) { h.single_task<class TimingReadoutTask1>([=]() {
        for (int i = 0; i < 2; i++) {
          ClockCounter_device1[i]=TimeReportingPipe1::read();
    }});});
    #endif
    #if (_PPLN_ > 2)
    // write to dram to communicate back to host
    auto ev3_2 = q.submit([&](handler &h) { h.single_task<class TimingReadoutTask2>([=]() {
        for (int i = 0; i < 2; i++) {
          ClockCounter_device2[i]=TimeReportingPipe2::read();
    }});});
    #endif
    #if (_PPLN_ > 3)
    // write to dram to communicate back to host
    auto ev3_3 = q.submit([&](handler &h) { h.single_task<class TimingReadoutTask3>([=]() {
        for (int i = 0; i < 2; i++) {
          ClockCounter_device3[i]=TimeReportingPipe3::read();
    }});});
    #endif


    if (timing_only == 0) {
      ev0.wait(); // read mappable
      ev1.wait(); // read unmappable
      ev2_0.wait(); // read hash table
      #if (_PPLN_ > 1)
      ev2_1.wait(); // read hash table
      #endif
      #if (_PPLN_ > 2)
      ev2_2.wait(); // read hash table
      #endif
      #if (_PPLN_ > 3)
      ev2_3.wait(); // read hash table
      #endif
    }
    ev3_0.wait(); // read timing
    #if (_PPLN_ > 1)
    ev3_1.wait(); // read timing
    #endif
    #if (_PPLN_ > 2)
    ev3_2.wait(); // read timing
    #endif
    #if (_PPLN_ > 3)
    ev3_3.wait(); // read timing
    #endif

  } catch (std::exception const &e) {
    std::cerr << "Exception!\n";
    std::terminate();
  }

  // Copy values back to host
  if (timing_only == 0) {
    auto ev0 = q.memcpy(MatchedOutput_host, MatchedOutput_device, (t1_ydim+t2_ydim) * sizeof(MatchedTuple));
    ev0.wait();
    auto ev1 = q.memcpy(UnmappedOutput_host, UnmappedOutput_device, (t1_ydim) * sizeof(HashedTuple));
    ev1.wait();
    auto ev2_0 = q.memcpy(HashedOutput_host0, HashedOutput_device0, (_BKTS_*_SLTS_) * sizeof(HashEntry));
    ev2_0.wait();
    #if (_PPLN_ > 1)
    auto ev2_1 = q.memcpy(HashedOutput_host1, HashedOutput_device1, (_BKTS_*_SLTS_) * sizeof(HashEntry));
    ev2_1.wait();
    #endif
    #if (_PPLN_ > 2)
    auto ev2_2 = q.memcpy(HashedOutput_host2, HashedOutput_device2, (_BKTS_*_SLTS_) * sizeof(HashEntry));
    ev2_2.wait();
    #endif
    #if (_PPLN_ > 3)
    auto ev2_3 = q.memcpy(HashedOutput_host3, HashedOutput_device3, (_BKTS_*_SLTS_) * sizeof(HashEntry));
    ev2_3.wait();
    #endif
  }
  auto ev3_0 = q.memcpy(ClockCounter_host0, ClockCounter_device0, 2 * sizeof(ClockCounter));
  ev3_0.wait();
  #if (_PPLN_ > 1)
  auto ev3_1 = q.memcpy(ClockCounter_host1, ClockCounter_device1, 2 * sizeof(ClockCounter));
  ev3_1.wait();
  #endif
  #if (_PPLN_ > 2)
  auto ev3_2 = q.memcpy(ClockCounter_host2, ClockCounter_device2, 2 * sizeof(ClockCounter));
  ev3_2.wait();
  #endif
  #if (_PPLN_ > 3)
  auto ev3_3 = q.memcpy(ClockCounter_host3, ClockCounter_device3, 2 * sizeof(ClockCounter));
  ev3_3.wait();
  #endif

  // for some unknown reason, execution has been seg-faulting here. Added try-catches to make sure we get our performance nums, also reordered to make sure we get the important information first
  // Print out timing info.
  printf("\n\nPipeline 0\nPhase       | Interval (clocks)\n");
  printf("Build Phase | %lld\n", ClockCounter_host0[0]); 
  printf("Probe Phase | %lld\n", ClockCounter_host0[1]);
  fflush(stdout);

  #if (_PPLN_ > 1)
  printf("\n\nPipeline 1\nPhase       | Interval (clocks)\n");
  printf("Build Phase | %lld\n", ClockCounter_host1[0]); 
  printf("Probe Phase | %lld\n", ClockCounter_host1[1]);
  fflush(stdout);

  #endif
  #if (_PPLN_ > 2)
  printf("\n\nPipeline 2\nPhase       | Interval (clocks)\n");
  printf("Build Phase | %lld\n", ClockCounter_host2[0]); 
  printf("Probe Phase | %lld\n", ClockCounter_host2[1]);
  fflush(stdout);

  #endif
  #if (_PPLN_ > 3)
  printf("\n\nPipeline 3\nPhase       | Interval (clocks)\n");
  printf("Build Phase | %lld\n", ClockCounter_host3[0]); 
  printf("Probe Phase | %lld\n", ClockCounter_host3[1]);
  fflush(stdout);

  #endif

  if (timing_only == 0) {
    // Print out all matched values
    printf("\n\nMatched Outputs\n");
    printf("    | Key  |  V1  |  V2\n");
    for(int i = 0; i < t1_ydim+t2_ydim; i++) {
      if (MatchedOutput_host[i].valid) {
        printf("%3u | %4u | %4u | %4u\n", i, MatchedOutput_host[i].key, MatchedOutput_host[i].val0, MatchedOutput_host[i].val1);
      }
    }
  
    // Print out all hashed values
    printf("\n\nHash Table0\n");
    printf("    | ");
    for (int s = 0; s < _SLTS_; s++)
      printf("SL %1u | ", s);
    printf("\n");
    // Stream out hash table for debugging reasons
    for (int b = 0; b < _BKTS_; b++) {
      printf("%3u | ",b);
      for (int s = 0; s < _SLTS_; s++)
        printf("%4u | ", HashedOutput_device0[b*_SLTS_+s].key);
      printf("\n");
    }
    #if (_PPLN_ > 1)
      // Print out all hashed values
      printf("\n\nHash Table1\n");
      printf("    | ");
      for (int s = 0; s < _SLTS_; s++)
        printf("SL %1u | ", s);
      printf("\n");
      // Stream out hash table for debugging reasons
      for (int b = 0; b < _BKTS_; b++) {
        printf("%3u | ",b);
        for (int s = 0; s < _SLTS_; s++)
          printf("%4u | ", HashedOutput_device1[b*_SLTS_+s].key);
        printf("\n");
      }
    #endif
    #if (_PPLN_ > 2)
      // Print out all hashed values
      printf("\n\nHash Table2\n");
      printf("    | ");
      for (int s = 0; s < _SLTS_; s++)
        printf("SL %1u | ", s);
      printf("\n");
      // Stream out hash table for debugging reasons
      for (int b = 0; b < _BKTS_; b++) {
        printf("%3u | ",b);
        for (int s = 0; s < _SLTS_; s++)
          printf("%4u | ", HashedOutput_device2[b*_SLTS_+s].key);
        printf("\n");
      }
    #endif
    #if (_PPLN_ > 3)
      // Print out all hashed values
      printf("\n\nHash Table3\n");
      printf("    | ");
      for (int s = 0; s < _SLTS_; s++)
        printf("SL %1u | ", s);
      printf("\n");
      // Stream out hash table for debugging reasons
      for (int b = 0; b < _BKTS_; b++) {
        printf("%3u | ",b);
        for (int s = 0; s < _SLTS_; s++)
          printf("%4u | ", HashedOutput_device3[b*_SLTS_+s].key);
        printf("\n");
      }
    #endif

    // Print out input tables
    printf("\n\nTable 1\n");
    printf("Val | ");
    for (int c = 0; c < t1_xdim; c++)
      printf("Col%1u | ", c);
    printf("\n");
    for (int r = 0; r < t1_ydim; r++) {
      printf("%3u | ",r);
      for (int c = 0; c < t1_xdim; c++)
        printf("%4u | ", t1_data_host[r*t1_xdim+c]);
      printf("\n");
    }

    printf("\n\nTable 2\n");
    printf("Val | ");
    for (int c = 0; c < t2_xdim; c++)
      printf("Col%1u | ", c);
    printf("\n");
    for (int r = 0; r < t2_ydim; r++) {
      printf("%3u | ",r);
      for (int c = 0; c < t2_xdim; c++)
        printf("%4u | ", t2_data_host[r*t2_xdim+c]);
      printf("\n");
    }

    // print key histogram
    printf("\n\n\nTable 2 Histogram (bounded to what aligns with table 1\n");
    for (int a = 0; a < t1_ydim; a++) {
      printf("%3d |",c1_data_host[a]);
      for (int b = 0; b < t2_ydim; b++) {
        if (c2_data_host[b]==c1_data_host[a]) {
          printf("*");
        }
      }
      printf("\n");
    }
    
    // Print out all unmatched values
    printf("\n\nUnmappable Values\n");
    printf("    | Key  | Val\n");
    for(int i = 0; i < t1_ydim; i++) {
      if (UnmappedOutput_host[i].valid) {
        printf("%3u | %4u | %4u\n", i, UnmappedOutput_host[i].key, UnmappedOutput_host[i].val);
      }
    }
  }
  
  return 0;
}
