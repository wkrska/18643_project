#ifndef STAGES_H
#define STAGES_H

#include <cstdio>
#include <cstdlib>
#include <sycl/sycl.hpp>

using namespace sycl;

#include "core.h"
#include "pipes.h"
#include "types.h"
#include "debug.h"

// All streaming stages follows a common construction based on a main
// ii=1 loops, in many case infinite. An iteration should start every
// cycle, with the sequential loop body pipelined by the compiler to
// overlap many iterations.  The tasks have no arguments.  Only input
// and output are done through pipes.  Because pipes are types,
// template is used to create the semblance of "pipe arguments".

template <class HashFunc, typename TTuplesInPipe, typename TTuplesOutPipe, int TBundleDepth>
void hashStage() {
  while (1) {
    // input valid
    BOOL iValid = false;

    // input bundle
    [[intel::fpga_register]] TuplesBundle<TBundleDepth> iBundle;
    // Output tuple
    [[intel::fpga_register]] HashedTuple oTuple;

    iBundle = TTuplesInPipe::read(iValid);
    
    // only do the following if you read a value
    if (iValid) {
      [[intel::initiation_interval(1)]] for (int i = 0; i < TBundleDepth; i++) {
        if (iBundle.tuples[i].valid) {
          // Hash Functions are defined in main
          oTuple.valid = iBundle.tuples[i].valid;
          oTuple.key = iBundle.tuples[i].key;
          oTuple.val = iBundle.tuples[i].val;
          oTuple.eof = iBundle.tuples[i].eof;

          switch (iBundle.hashSelect) {
            case 0:
              oTuple.h0 = HashFunc::hash0(iBundle.tuples[i].key);
              oTuple.h1 = HashFunc::hash1(iBundle.tuples[i].key);
              break;
            case 1:
              oTuple.h0 = HashFunc::djb2(iBundle.tuples[i].key);
              oTuple.h1 = HashFunc::magic_int_hash(iBundle.tuples[i].key);
              break;
            case 2:
              oTuple.h0 = HashFunc::hash32shift(iBundle.tuples[i].key);
              oTuple.h1 = HashFunc::jenkins32hash(iBundle.tuples[i].key);
              break;
            case 3:
              oTuple.h0 = HashFunc::jenkins32hash(iBundle.tuples[i].key);
              oTuple.h1 = HashFunc::hash32shiftmult(iBundle.tuples[i].key);
              break;
            default:
              oTuple.h0 = HashFunc::hash0(iBundle.tuples[i].key);
              oTuple.h1 = HashFunc::hash1(iBundle.tuples[i].key);
              break;
          }

          TTuplesOutPipe::write(oTuple);
        }
      }
    }
  }
}

template <class HashFunc, typename TTuplesInPipe, 
          typename TTuplesOutPipe0, typename TTuplesOutPipe1, 
          typename TTuplesOutPipe2, typename TTuplesOutPipe3>
void forkStage() {
  // based on the hashed value, send to one of N pipes (determined at build time)
  [[intel::fpga_register]] HashedTuple iTuple;
  [[intel::fpga_register]] HashedTuple eofTuple;
  eofTuple.eof = true;
  eofTuple.valid = false;

  BOOL temp0;
  BOOL temp1;
  BOOL temp2;
  BOOL temp3;
  
  while (1) {
    iTuple = TTuplesInPipe::read();
    #if 0
    HashVal dest = HashFunc::jenkins32hash(iTuple.h0);
    switch (dest % _PPLN_) {
    #else 
    switch (iTuple.h0 % _PPLN_) {
    #endif
      case (0) :
        iTuple.h0 = iTuple.h0 % _BKTS_;
        iTuple.h1 = iTuple.h1 % _BKTS_;
        // temp0 = false;
        // while(!temp0) {
          TTuplesOutPipe0::write(iTuple);//, temp0);
        if (iTuple.eof) {
          #if (_PPLN_ > 1)
          TTuplesOutPipe1::write(eofTuple);
          #endif
          #if (_PPLN_ > 2)
          TTuplesOutPipe2::write(eofTuple);
          #endif
          #if (_PPLN_ > 3)
          TTuplesOutPipe3::write(eofTuple);
          #endif
        }
        break;
      #if (_PPLN_ > 1)
      case (1) :
        iTuple.h0 = iTuple.h0 % _BKTS_;
        iTuple.h1 = iTuple.h1 % _BKTS_;
        // temp1 = false;
        // while(!temp1) {
          TTuplesOutPipe1::write(iTuple);//, temp1);
        if (iTuple.eof) {
          TTuplesOutPipe0::write(eofTuple);
          #if (_PPLN_ > 2)
          TTuplesOutPipe2::write(eofTuple);
          #endif
          #if (_PPLN_ > 3)
          TTuplesOutPipe3::write(eofTuple);
          #endif
        }
        break;
      #endif
      #if (_PPLN_ > 2)
      case (2) :
        iTuple.h0 = iTuple.h0 % _BKTS_;
        iTuple.h1 = iTuple.h1 % _BKTS_;
        // temp2 = false;
        // while(!temp2) {
          TTuplesOutPipe2::write(iTuple);//, temp2);
        if (iTuple.eof) {
          TTuplesOutPipe0::write(eofTuple);
          TTuplesOutPipe1::write(eofTuple);
          #if (_PPLN_ > 3)
          TTuplesOutPipe3::write(eofTuple);
          #endif
        }
        break;
      #endif
      #if (_PPLN_ > 3)
      case (3) :
        iTuple.h0 = iTuple.h0 % _BKTS_;
        iTuple.h1 = iTuple.h1 % _BKTS_;
        // temp3 = false;
        // while(!temp3) {
          TTuplesOutPipe3::write(iTuple);//, temp3);
        if (iTuple.eof) {
          TTuplesOutPipe0::write(eofTuple);
          TTuplesOutPipe1::write(eofTuple);
          TTuplesOutPipe2::write(eofTuple);
        }
        break;
      #endif
      default :
        TTuplesOutPipe0::write(iTuple);
        break;
    }
  }
}

template <typename TTuplesInPipe, typename TUnmappedTuplesOutPipe, typename TMatchedTuplesOutPipe, typename THashedTuplesOutPipe, typename TTriggerOutPipe>
void hashTableStage() {
  // HASH TABLE DECLARATION
  [[intel::fpga_memory("BLOCK_RAM"), /*intel::doublepump,, intel::bankwidth(_SLTS_*sizeof(HashEntry)), intel::numbanks(_BKTS_)*/ intel::simple_dual_port]] 
  HashEntry hash_table[_BKTS_][_SLTS_];

  // Buffer for read row(s)
  // [[intel::fpga_register]] HashEntry hash_buckets[_SLTS_+2][_SLTS_];
  /*[[intel::fpga_register]]*/ HashBucket hash_buckets[_SLTS_+2];

  // Zero table
  #pragma unroll
  for (int b = 0; b < _BKTS_; b++) {
    #pragma unroll
    for (int s = 0; s < _SLTS_; s++) {
      HashEntry empty = {};
      hash_table[b][s] = empty;
    }
  }

  // Control vars
  const int BUILD = 0;
  const int PROBE = 1;
  const int EXIT = 2;
  int phase = BUILD;

  // For timing purposes... really stupid code
  ClockCounter event = 1;
  BOOL firstItr = true;
  
  // Build phase loop, exits on EOF
  [[intel::disable_loop_pipelining]] while (phase == BUILD) {
    // Read tuple from pipeline (blocking)
    HashedTuple iTuple = TTuplesInPipe::read();
    if (firstItr) {
      TTriggerOutPipe::write(event);
      firstItr = false;
    }
    if (iTuple.valid) {
      // Format for hash-table
      HashEntry iTupleFormat;
      
      // Load hash0 bucket, check for min 
      bucket_copy(hash_table[iTuple.h0], hash_buckets[0].entries, _SLTS_);
      bucket_copy(hash_table[iTuple.h1], hash_buckets[1].entries, _SLTS_);

      BOOL h0_avail = false;
      BOOL h1_avail = false;
      int free_slot0 = INT_MIN;
      int free_slot1 = INT_MIN;
      bucket_scan(hash_buckets[0].entries, free_slot0, h0_avail, _SLTS_);
      if (!h0_avail) {
        bucket_scan(hash_buckets[1].entries, free_slot1, h1_avail, _SLTS_);
      }
      
      // If empty slot found for h0 bucket
      if (h0_avail) {
        iTupleFormat.full = 1;
        iTupleFormat.key = iTuple.key;
        iTupleFormat.val = iTuple.val;
        iTupleFormat.tag = iTuple.h1;
        hash_table[iTuple.h0][free_slot0] = iTupleFormat;
      } else if (h1_avail) { // If not empty slot for h0 bucket but empty slot found for h1 bucket
        iTupleFormat.full = 1;
        iTupleFormat.key = iTuple.key;
        iTupleFormat.val = iTuple.val;
        iTupleFormat.tag = iTuple.h0;
        hash_table[iTuple.h1][free_slot1] = iTupleFormat;
      } else { // no empty slots found, search all C*2 options (no special optimizations). Pipelines
        int s = 0; // itr var
        BOOL eviction = 0; // success code for eviction candidate

        int free_slot = INT_MIN; // free slot to place current value 
        int repl_slot = INT_MIN; // free slot to place evicted value
        
        ////////////////////////////////////////////////////////////

        // Search tags of h0
        // Perform first load outside of loop
        bucket_copy(hash_table[hash_buckets[0].entries[0].tag], hash_buckets[0+2].entries, _SLTS_); 
        /*[[intel::initiation_interval(1)]]*/ while (s < (_SLTS_-1) && !eviction) {
          // Check if availability from prev itr
          bucket_scan(hash_buckets[s + 2].entries, repl_slot, eviction, _SLTS_);
          if (eviction)
            free_slot = s;

          s++;
          // Load new 
          bucket_copy(hash_table[hash_buckets[0].entries[s].tag], hash_buckets[s + 2].entries, _SLTS_); 
        }

        if (!eviction) {
          // Check if availability from last itr
          bucket_scan(hash_buckets[_SLTS_-1].entries, repl_slot, eviction, _SLTS_);
        }

        if (eviction) {
          // If there is space in tags of h0, perform replacement
          // Evicted element (Move data from (copy of) candidate slot to its alternate spot)
          hash_table[hash_buckets[0].entries[free_slot].tag][repl_slot] = hash_buckets[0].entries[free_slot];

          // Write value tp has table
          iTupleFormat.full = 1;
          iTupleFormat.key = iTuple.key;
          iTupleFormat.val = iTuple.val;
          iTupleFormat.tag = iTuple.h1;
          hash_table[iTuple.h0][free_slot] = iTupleFormat;
        }

        ////////////////////////////////////////////////////////////

        if (!eviction) {
          s = 0;
          // Search tags of h1
          // Perform first load outside of loop
          bucket_copy(hash_table[hash_buckets[1].entries[0].tag], hash_buckets[0+2].entries, _SLTS_); 
          /*[[intel::initiation_interval(1)]]*/ while (s < (_SLTS_-1) && !eviction) {
            // Check if availability from prev itr.entries
            bucket_scan(hash_buckets[s + 2].entries, repl_slot, eviction, _SLTS_);
            if (eviction)
              free_slot = s;
            s++;
            // Load new 
            bucket_copy(hash_table[hash_buckets[1].entries[s].tag], hash_buckets[s + 2].entries, _SLTS_); 
          }

          if (!eviction) {
            // Check if availability from last itr
            bucket_scan(hash_buckets[_SLTS_-1].entries, repl_slot, eviction, _SLTS_);
          }

          if (eviction) {
            // If there is space in tags of h1, perform replacement
            // Evicted element (Move data from (copy of) candidate slot to its alternate spot)
            hash_table[hash_buckets[1].entries[free_slot].tag][repl_slot] = hash_buckets[1].entries[free_slot];

            // Write value tp has table
            iTupleFormat.full = 1;
            iTupleFormat.key = iTuple.key;
            iTupleFormat.val = iTuple.val;
            iTupleFormat.tag = iTuple.h0;
            hash_table[iTuple.h0][free_slot] = iTupleFormat;
          }
        }

        ////////////////////////////////////////////////////////////

        // Failure case, if unable to evict any elements, write failed tuple to unmappalbe pipe
        if (!eviction) {
          TUnmappedTuplesOutPipe::write(iTuple);
        }
      }
    }
    
    // End of itr, check if EOF
    if (iTuple.eof) {
      phase = PROBE;
    }
  }
  // Write EOF for failure pipe
  HashedTuple tTuple;
  tTuple.valid = false;
  tTuple.eof = true;
  TUnmappedTuplesOutPipe::write(tTuple);
  
  // Trigger timing
  TTriggerOutPipe::write(event);
  firstItr = true;


  // PROBE PHASE
  /*[[intel::initiation_interval(3)]]*/ while (phase == PROBE) {
    
    HashedTuple iTuple = TTuplesInPipe::read();
    if (firstItr) {
      TTriggerOutPipe::write(event);
      firstItr = false;
    }
    MatchedTuple oTuple;

    if (iTuple.valid) {
      // Load hash0 bucket, check for min 
      bucket_copy(hash_table[iTuple.h0], hash_buckets[0].entries, _SLTS_);
      bucket_copy(hash_table[iTuple.h1], hash_buckets[1].entries, _SLTS_);

      // Check if exists in either
      BOOL h0_avail = false;
      BOOL h1_avail = false;
      int free_slot0 = INT_MIN;
      int free_slot1 = INT_MIN;
      compare(hash_buckets[0].entries,free_slot0, iTuple.key, h0_avail, _SLTS_);
      compare(hash_buckets[1].entries,free_slot1, iTuple.key, h1_avail, _SLTS_);

      // Send out pair if exists
      oTuple.valid = true;
      oTuple.key = iTuple.key;
      oTuple.val1 = iTuple.val;
      oTuple.eof = iTuple.eof;
      if (h0_avail) {
        oTuple.val0 = hash_buckets[0].entries[free_slot0].val;
        TMatchedTuplesOutPipe::write(oTuple);
      } else if (h1_avail) {
        oTuple.val0 = hash_buckets[1].entries[free_slot1].val;
        TMatchedTuplesOutPipe::write(oTuple);
      }
    }

    // If EOF, send out last value
    if (iTuple.eof) {
      oTuple.valid = false;
      oTuple.eof = true;
      TMatchedTuplesOutPipe::write(oTuple);
      phase = EXIT;
    }
  }

  // Trigger wrapup
  TTriggerOutPipe::write(event);

  // Stream out hash table for debugging reasons
  for (int b = 0; b < _BKTS_; b++)
    for (int s = 0; s < _SLTS_; s++)
      THashedTuplesOutPipe::write(hash_table[b][s]);

}

template <typename TTuplesInPipe0, typename TTuplesInPipe1, 
          typename TTuplesInPipe2, typename TTuplesInPipe3,
          typename TTuplesOutPipe, typename DataType>
void funnelStage() {
  BOOL iValid0 = false;
  [[intel::fpga_register]] DataType iTuple0;
  #if (_PPLN_ > 1)
  BOOL iValid1 = false;
  [[intel::fpga_register]] DataType iTuple1;
  #endif
  #if (_PPLN_ > 2)
  BOOL iValid2 = false;
  [[intel::fpga_register]] DataType iTuple2;
  #endif
  #if (_PPLN_ > 3)
  BOOL iValid3 = false;
  [[intel::fpga_register]] DataType iTuple3;
  #endif
  BOOL wValid0;
  BOOL wValid1;
  BOOL wValid2;
  BOOL wValid3;
  
  while (1) {
    iTuple0 = TTuplesInPipe0::read(iValid0);
    if (iValid0) {
      // wValid0 = false;
      // while (!wValid0) {
        TTuplesOutPipe::write(iTuple0);//, wValid0);
      // }
    }
    #if (_PPLN_ > 1)
    iTuple1 = TTuplesInPipe1::read(iValid1);
    if (iValid1) {
      // wValid1 = false;
      // while (!wValid1) {
        TTuplesOutPipe::write(iTuple1);//, wValid1);
      // }
    }
    #endif
    #if (_PPLN_ > 2)
    iTuple2 = TTuplesInPipe2::read(iValid2);
    if (iValid2) {
      // wValid2 = false;
      // while (!wValid2) {
        TTuplesOutPipe::write(iTuple2);//, wValid2);
      // }
    }
    #endif
    #if (_PPLN_ > 3)
    iTuple3 = TTuplesInPipe3::read(iValid3);
    if (iValid3) {
      // wValid3 = false;
      // while (!wValid3) {
        TTuplesOutPipe::write(iTuple3);//, wValid3);
      // }
    }
    #endif
  }
}

template <typename TTriggerInPipe, typename TTimeOutPipe>
void timerStage() {
  // Time build stage
  // blocking read
  TTriggerInPipe::read();
  BOOL iValid = false;
  ClockCounter cc = 0;
  [[intel::initiation_interval(1)]] while (!iValid) {
    cc++;
    // non-blocking read
    TTriggerInPipe::read(iValid);
  }
  TTimeOutPipe::write(cc);

  // Time probe stage
  // blocking read
  TTriggerInPipe::read();
  iValid = false;
  cc = 0;
  [[intel::initiation_interval(1)]] while (!iValid) {
    cc++;
    // non-blocking read
    TTriggerInPipe::read(iValid);
  }
  TTimeOutPipe::write(cc);
}

#endif
