// SETUP TEST CONDITION
#if 1 // Use default/makefile defined values
  #ifndef _BKTS_
    #define _BKTS_ 1024
  #endif
  #ifndef _SLTS_
    #define _SLTS_ 4
  #endif
  #define BundleDepth 8
#else // Use one time/user defined values
  #define _BKTS_ 16
  #define _SLTS_ 4
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
      HashVal hash = key % _BKTS_;
      return hash;
    }

    static HashVal hash1(JoinKey key) {
      HashVal hash = (key / _BKTS_) % _BKTS_;
      return hash;
    }

    static HashVal djb2(JoinKey key) {
      JoinKey hash = 5381;
      hash = ((hash << 5) + hash) + key;
      hash = hash % _BKTS_;
      return (HashVal) hash;
    }

    static HashVal magic_int_hash(JoinKey key) {
      JoinKey key_t = key;
      key = ((key >> 16) ^ key) * 0x45d9f3b;
      key = ((key >> 16) ^ key) * 0x45d9f3b;
      key = (key >> 16) ^ key;
      key = key % _BKTS_;
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
      key = key % _BKTS_;
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
      key = key % _BKTS_;
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
      key = key % _BKTS_;
      return (HashVal) key;
    }
  };

  //////////////////////
  // Set up test input
  //////////////////////
  if (argc != 10) {
    std::cout << "ARGS ERROR" << std::endl;
    std::cout << "./main <t1_xdim> <t1_ydim> <t2_xdim> <t2_ydim> <t1_filename> <t2_filename> <join_column_value> <data_source_select> <hash_method_select>" << std::endl;
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

  JoinKey t1_data_host[t1_xdim*t1_ydim];
  JoinKey t2_data_host[t2_xdim*t2_ydim];

  JoinKey c1_data_host[t1_ydim];
  JoinKey c2_data_host[t2_ydim];

  int t1_join_column_index = 0;
  int t2_join_column_index = 0;
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
  HashEntry *HashedOutput_device = malloc_device<HashEntry>(_BKTS_*_SLTS_, q);
  ClockCounter *ClockCounter_device = malloc_device<ClockCounter>(2, q);

  MatchedTuple MatchedOutput_host[t1_ydim+t2_ydim];
  HashedTuple UnmappedOutput_host[t1_ydim];
  HashEntry HashedOutput_host[_BKTS_*_SLTS_] = {};
  ClockCounter ClockCounter_host[2] = {};

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
    q.submit([&](handler &h) {
      h.single_task<class HashTask0>([=]() {
        hashStage<HashFunc, InputPipe0, HashedTuplePipe0, BundleDepth>();
      });
    });

    // Store hashed values in hash table
    q.submit([&](handler &h) {
      h.single_task<class HashTableTask0>([=]() {
        hashTableStage<HashedTuplePipe0, 
                       UnmappableTuplePipe0, 
                       MatchedTuplePipe0, 
                       HashedTablePipe0,
                       HashTableTimingPipe0>();
      });
    });

    // Stupid hacky timing method this truly sucks but we need to count clocks
    // gettimeofday(&TimerStage, NULL);
    q.submit([&](handler &h) {
      h.single_task<class TimerTask0>([=]() {
        timerStage<HashTableTimingPipe0, TimeReportingPipe0>();
      });
    });

    // write to dram to communicate back to host
    auto ev0 = q.submit([&](handler &h) {
      h.single_task<class SuccessfulReadoutTask>([=]() {
        // Zero out
        for (int i = 0; i < t1_ydim+t2_ydim; i++) {
          MatchedOutput_device[i].valid = 0;
          MatchedOutput_device[i].key = 0;
          MatchedOutput_device[i].val0 = 0;
          MatchedOutput_device[i].val1 = 0;
        }
        bool eof = false;
        int i = 0;
        while (!eof) {
          MatchedTuple temp = MatchedTuplePipe0::read();
          if (temp.valid) {
            MatchedOutput_device[i] = temp;
            i++;
          }
          eof = temp.eof;
        }
      });
    });

    // write to dram to communicate back to host
    auto ev1 = q.submit([&](handler &h) {
      h.single_task<class UnmappableReadoutTask>([=]() {
        // Zero out
        for (int i = 0; i < t1_ydim; i++) {
          UnmappedOutput_device[i].valid = 0;
          UnmappedOutput_device[i].key = 0;
          UnmappedOutput_device[i].val = 0;
        }
        bool eof = false;
        int i = 0;
        while (!eof) {
          HashedTuple temp = UnmappableTuplePipe0::read();
          if (temp.valid) {
            UnmappedOutput_device[i] = temp;
            i++;
          }
          eof = temp.eof;
        }
      });
    });

    // write to dram to communicate back to host
    auto ev2 = q.submit([&](handler &h) {
      h.single_task<class HashTableReadoutTask>([=]() {
        for (int b = 0; b < _BKTS_; b++) {
          for (int s = 0; s < _SLTS_; s++ ) {
            HashedOutput_device[b*_SLTS_+s]=HashedTablePipe0::read();
          }
        }
      });
    });

    // write to dram to communicate back to host
    auto ev3 = q.submit([&](handler &h) {
      h.single_task<class TimingReadoutTask>([=]() {
        for (int i = 0; i < 2; i++) {
          ClockCounter_device[i]=TimeReportingPipe0::read();
        }
      });
    });

    ev0.wait();
    ev1.wait();
    ev2.wait();
    ev3.wait();

  } catch (std::exception const &e) {
    std::cerr << "Exception!\n";
    std::terminate();
  }
  // gettimeofday(&EndDeviceDram, NULL);

  // Copy values back to host
  auto ev0 = q.memcpy(MatchedOutput_host, 
                      MatchedOutput_device,
                      (t1_ydim+t2_ydim) * sizeof(MatchedTuple));
  auto ev1 = q.memcpy(UnmappedOutput_host, 
                      UnmappedOutput_device,
                      (t1_ydim) * sizeof(HashedTuple));
  auto ev2 = q.memcpy(HashedOutput_host, 
                      HashedOutput_device,
                      (_BKTS_*_SLTS_) * sizeof(HashEntry));
  auto ev3 = q.memcpy(ClockCounter_host, 
                      ClockCounter_device,
                      2 * sizeof(ClockCounter));

  ev0.wait();
  ev1.wait();
  ev2.wait();
  ev3.wait();

  // for some unknown reason, execution has been seg-faulting here. Added try-catches to make sure we get our performance nums, also reordered to make sure we get the important information first
  // Print out timing info.
  printf("\n\nPhase       | Interval (clocks)\n");
  printf("Build Phase | %lld\n", ClockCounter_host[0]); 
  printf("Probe Phase | %lld\n", ClockCounter_host[1]);

  // Print out all matched values
  printf("\n\nMatched Outputs\n");
  printf("    | Key  |  V1  |  V2\n");
  for(int i = 0; i < t1_ydim+t2_ydim; i++) {
    if (MatchedOutput_host[i].valid) {
      printf("%3u | %4u | %4u | %4u\n", i, MatchedOutput_host[i].key, MatchedOutput_host[i].val0, MatchedOutput_host[i].val1);
    }
  }

  // Print out all hashed values
  printf("\n\nHash Table\n");
  printf("    | ");
  for (int s = 0; s < _SLTS_; s++)
    printf("SL %1u | ", s);
  printf("\n");
  // Stream ot hash table for debugging reasons
  for (int b = 0; b < _BKTS_; b++) {
    printf("%3u | ",b);
    for (int s = 0; s < _SLTS_; s++)
      printf("%4u | ", HashedOutput_device[b*_SLTS_+s].key);
    printf("\n");
  }

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
  // int buckets[(_BKTS_/4)] = {};
  // for (int r = 0; r < t2_ydim; r++)
  //   buckets[t2_data_host[r*t2_xdim+t2_join_column_index]/(t1_ydim/(_BKTS_/4))]++;
  // for (int i = 0; i < (_BKTS_/4); i++) {
  //   printf("[%4d,%4d): %3d | ", (i*t1_ydim)/(_BKTS_/4), ((i+1)*t1_ydim)/(_BKTS_/4)-1, buckets[i]);
  //   for (int p = 0; p < buckets[i]; p++)
  //     printf("*");
  //   printf("\n");
  // }
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
  
  return 0;
}
