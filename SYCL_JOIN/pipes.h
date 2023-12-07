#ifndef PIPES_H
#define PIPES_H

#define DFLT_PIPE_DEPTH (32)

#include <sycl/sycl.hpp>
using namespace sycl;

#include "types.h"

template <int depth> struct TuplesBundle {
  HashSel hashSelect;
  Tuple tuples[depth];
};

// Pipe(s) from host to hash stage
using InputPipe0 =  ext::intel::pipe<class pipe0_0, TuplesBundle<BundleDepth>, DFLT_PIPE_DEPTH>;

// Pipe(s) from hash to hashtable stage
using HashedTuplePipe0 = ext::intel::pipe<class pipe1_0, HashedTuple, DFLT_PIPE_DEPTH>;

// Pipe(s) for successfully matched tuples from hashtable stage back to host
using MatchedTuplePipe0 = ext::intel::pipe<class pipe2_0, MatchedTuple, DFLT_PIPE_DEPTH>;

// Pipe(s) for unsuccessfully stored tuples from hashtable stage back to host
using UnmappableTuplePipe0 = ext::intel::pipe<class pipe3_0, HashedTuple, DFLT_PIPE_DEPTH>;

// Pipe(s) from hashtable to host
using HashedTablePipe0 = ext::intel::pipe<class pipe4_0, HashEntry, DFLT_PIPE_DEPTH>;

// Pipe(s) for timing reading and writing to the hash table
using HashTableTimingPipe0 = ext::intel::pipe<class pipe5_0, ClockCounter, DFLT_PIPE_DEPTH>;
using TimeReportingPipe0 = ext::intel::pipe<class pipe5_1, ClockCounter, DFLT_PIPE_DEPTH>;

#endif