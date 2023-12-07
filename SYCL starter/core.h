/*=====================================================



IGNORE THIS FOR NOW, JUST TAKE INSPO IF YOU NEED TO



=====================================================*/










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

// The following is used by a hack to work around a compiler issue
// until next release. 1D array declared in structs has to be declared
// as 2D to map to registers.
#define ONE (1)
#define ZERO (0)

// Takes in TNumTuple KV pairs and returns up to TNumTuple KV
// pairs. Input pairs with common keys are combined into one by
// summing their values.  Absorbed positions are marked invalid.
template <int TNumTuple, int TNumGroup>
void collectTuples(
    // input
    Tuple in[TNumTuple],

    // output
    Tuple uniq[TNumTuple]) {

  // initialize default
#pragma unroll
  for (UIDX tup = 0; tup < TNumTuple; tup++) {
    uniq[tup].valid = in[tup].valid;
    uniq[tup].key = in[tup].key;
  }

  // N^2 / 2 comparisons to collect common key pairs
#pragma unroll
  for (UIDX i = 0; i < TNumTuple; i++) {
    uniq[i].val = in[i].val;
#pragma unroll
    for (UIDX j = i + 1; j < TNumTuple; j++) {
      if (in[i].valid && in[j].valid && (in[i].key == in[j].key)) {
        uniq[j].valid = false;    // invalid absorbed position
        AGG_2ARG(uniq[i].val,in[j].val);
        //uniq[i].val += in[j].val; // absorb value
      }
    }
  }
}

// Translate TNumTuples to their allocated table locations.  Allocate
// new location when seeing a key for the first time. This implements
// a systolic array with O(G) delay but fixed FMAX independent of
// G.

// There is a compiler quirkiness having to do with adding banking
// directives to "struct with arrays" and/or "array of structs". The
// table array has to be declared flat, outside of a struct/class. To
// be revisited in the future.

// The macros below is a clumsy way of making things a little easier
// to use since we can't make a proper class.
#define MAPPER_DECLARE(P, N, G, O)                                             \
  [[intel::fpga_register]] AggKey __camKey[(P)][(G)] = {};                     \
  [[intel::fpga_register]] BOOL __camValid[(P)][(G)] = {};                     \
  CoreKeyMapper<(P), (N), (G), (O)> __mapper;

#define MAPPER_MAP(flush, it, ii, im, oi, om, p)                               \
  __mapper.map((flush), (it), (ii), (im), (oi), (om), __camKey, __camValid,    \
               (p));

template <int TNumPipeline, int TNumTuple, int TNumGroup, int TOffset>
struct CoreKeyMapper {
  //
  // CAM map table array to be implemented as registers
  //

  // map key to index; allocate on the fly; deep pipelining streaming
  // CAM lookup
  void map(
      // input
      BOOL flush,
      Tuple uniqIn[TNumTuple],    // cannot have duplicate keys
      AggTblIdx idxIn[TNumTuple], // from upstream translated linear index for
                                  // valid tuples
      BOOL mappedIn[TNumTuple],

      // output
      AggTblIdx
          idxOut[TNumTuple], // output translated linear index for valid tuples
      BOOL mappedOut[TNumTuple],

      // STATE
      AggKey camKey[TNumPipeline][TNumGroup],
      BOOL camValid[TNumPipeline][TNumGroup],

      // STATE idx
      UIDX ppln) {
#pragma unroll
    for (UIDX tup = 0; tup < TNumTuple; tup++) {
      // in cascade mode, transfer already mapped indices
      idxOut[tup] = idxIn[tup];
      mappedOut[tup] = mappedIn[tup];
    }

    // CAM populated from 0 to G-1
#pragma unroll
    for (UIDX grp = 0; grp < TNumGroup; grp++) {
      // this unrolls into very deep cascaded stages; CAM lookup is
      // serialized and pipelined across by the lookup stream.
      if (camValid[ppln][grp]) {
        // on a valid location, try mapping
#pragma unroll
        for (UIDX tup = 0; tup < TNumTuple; tup++) {
          if (uniqIn[tup].valid && camValid[ppln][grp] &&
              (uniqIn[tup].key == camKey[ppln][grp])) {
            mappedOut[tup] = true;
            idxOut[tup] = grp + TOffset * TNumGroup;
          }
        }
      } else {
#pragma unroll
        for (UIDX tup = 0; tup < TNumTuple; tup++) {
          if (uniqIn[tup].valid && !mappedOut[tup]) {
            // if still not mapped after exhausting previous keys;
            // allocate a new one
            camValid[ppln][grp] = true;
            camKey[ppln][grp] = uniqIn[tup].key;
            mappedOut[tup] = true;
            idxOut[tup] = grp + TOffset * TNumGroup;
            break; // go to next location for remaining unmapped
                   // tuples
          }
        }
      }
      if (flush) {
        camValid[ppln][grp] = false;
        // camKey[ppln][grp] = uniqIn[tup].key;
      }
    }
    return;
  }
};

// Aggregate into linearly indexed table.  Unlike V0, this version
// uses N-copies of 1R1W parallel tables to accumulate separately for
// each tuple lane. These N tables made from BRAM are much
// cheaper/faster than using only 1 table (NR+NW) made from registers.

// There is a compiler quirkiness having to do with adding banking
// directives to "struct with arrays" and/or "array of structs". The
// table array has to be declared flat, outside of a struct/class. To
// be revisited in the future.

// The macros below is a clumsy way of making things a little easier
// to use since we can't make a proper class.
#define TWOPOWsmall(N)                                                         \
  (((N) <= 2)                                                                  \
       ? (N)                                                                   \
       : (((N) <= 4)                                                           \
              ? 4                                                              \
              : (((N) <= 8)                                                    \
                     ? 8                                                       \
                     : (((N) <= 16)                                            \
                            ? 16                                               \
                            : (((N) <= 32) ? 32                                \
                                           : (((N) <= 64) ? 64 : (N)))))))

#define AGG_DECLARE(P, N, G)                                                   \
  [[intel::fpga_memory("BLOCK_RAM"), intel::singlepump,                        \
    intel::simple_dual_port /*, intel::bankwidth(sizeof(TuplePadded16)),       \
                              intel::max_replicates(1),                        \
                              intel::numbanks((N)*TWOPOWsmall(P))*/            \
  ]] TuplePadded16 __table[(G)][TWOPOWsmall(P)][(N)];                          \
  [[intel::fpga_register]] TuplePadded16 __deferred[(P)][2][(N)] = {};         \
  [[intel::fpga_register]] AggTblIdx __deferredIdx[(P)][2][(N)] = {};          \
  [[intel::fpga_register]] CoreAggregator<(P), (N), (G)> __agg;

#define AGG_ZERO_TABLE() __agg.zero(__table)

#define AGG_UPDATE(t, i, p)                                                    \
  __agg.update((t), (i), __table, __deferred, __deferredIdx, (p))

#define AGG_READ(idx, tups) __agg.read(idx, tups, __table)

template <int TNumPipeline, int TNumTuple, int TNumGroup>
struct CoreAggregator {

  // states has to live outside unfortunately.

  // BRAM array can't be zero by "={}";
  void zero(
      // STATE
      TuplePadded16 table[TNumGroup][TWOPOWsmall(TNumPipeline)][TNumTuple]) {

    assert(
        !((TWOPOWsmall(TNumPipeline) - 1) &
          (TWOPOWsmall(TNumPipeline)))); // TWOPOWsmall only cover small values

    [[intel::disable_loop_pipelining]] for (UIDX grp = 0; grp < TNumGroup;
                                            grp++) {
#pragma unroll
      for (UIDX ppln = 0; ppln < TNumPipeline; ppln++) {
#pragma unroll
        for (UIDX tub = 0; tub < TNumTuple; tub++) {
          TuplePadded16 empty = {};
          table[grp][ppln][tub] = empty;
        }
      }
    }
  }

  void update(
      // input
      Tuple in[TNumTuple],      // unique key tuples
      AggTblIdx idx[TNumTuple], // linear idx from mapper stage

      // STATE passed in by reference
      TuplePadded16 table[TNumGroup][TWOPOWsmall(TNumPipeline)][TNumTuple],
      TuplePadded16 deferred[TWOPOWsmall(TNumPipeline)][2][TNumTuple],
      AggTblIdx deferredIdx[TWOPOWsmall(TNumPipeline)][2][TNumTuple],

      // STATE idx
      UIDX ppln) {

    TuplePadded16 update[TNumTuple] = {};

    {
      BOOL frwd[TNumTuple] = {};
      AggVal frwdVal[TNumTuple];

      // find forwarding from memoized updates of not yet written
      // (works within lane)
#pragma unroll
      for (UIDX tup = 0; tup < TNumTuple; tup++) {
        if ((idx[tup] == deferredIdx[ppln][0][tup]) &&
            deferred[ppln][0][tup].valid) {
          // distance 1 RAW
          assert(in[tup].valid ? in[tup].key == deferred[ppln][0][tup].key
                                  : true);
          frwd[tup] = true;
          frwdVal[tup] = deferred[ppln][0][tup].val;
        } else if ((idx[tup] == deferredIdx[ppln][1][tup]) &&
                   deferred[ppln][1][tup].valid) {
          // distance 2 RAW
          assert(in[tup].valid ? in[tup].key == deferred[ppln][1][tup].key
                                  : true);
          frwd[tup] = true;
          frwdVal[tup] = deferred[ppln][1][tup].val;
        }
      }

      // compute new table entry value
#pragma unroll
      for (UIDX tup = 0; tup < TNumTuple; tup++) {
        TuplePadded16 temp = table[idx[tup]][ppln][tup];
        update[tup].valid = in[tup].valid;
        update[tup].key = in[tup].key; // overwrite okay
        AGG_3ARG(update[tup].val, in[tup].val, (frwd[tup] ? frwdVal[tup] : temp.val));
        // update[tup].val = in[tup].val + (frwd[tup] ? frwdVal[tup] : temp.val);
      }
    }

#pragma unroll
    for (UIDX tup = 0; tup < TNumTuple; tup++) {
      // write deferred updates from 2 iters ago
      if (deferred[ppln][1][tup].valid) {
        table[deferredIdx[ppln][1][tup]][ppln][tup] = deferred[ppln][1][tup];
      }
      // shift
      deferred[ppln][1][tup] = deferred[ppln][0][tup];
      deferredIdx[ppln][1][tup] = deferredIdx[ppln][0][tup];
    }

#pragma unroll
    for (UIDX tup = 0; tup < TNumTuple; tup++) {
      // record deferred updates to next round
      deferred[ppln][0][tup] = update[tup];
      deferredIdx[ppln][0][tup] = idx[tup];
    }
  }

  // Reading back when finished accumulating
  void read(
      // input
      AggTblIdx idx,

      // output
      Tuple tuples[TNumPipeline],

      // STATE
      TuplePadded16 table[TNumGroup][TWOPOWsmall(TNumPipeline)][TNumTuple]) {
// sum across banks to compute total
#pragma unroll
    for (UIDX ppln = 0; ppln < TNumPipeline; ppln++) {
#pragma unroll
      for (UIDX tup = 0; tup < TNumTuple; tup++) {
        TuplePadded16 temp = table[idx][ppln][tup];
        if (temp.valid) {
          tuples[ppln].valid = true;
          tuples[ppln].key = temp.key; // overwrite okay
          AGG_2ARG(tuples[ppln].val, temp.val);
          //tuples[ppln].val += temp.val;
        }
      }
    }
  }
};

#ifdef SHOW_DEPRECATED
#include "core_.h"
#endif

#endif
