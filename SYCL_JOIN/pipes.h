#ifndef PIPES_H
#define PIPES_H

#define DFLT_PIPE_DEPTH (32)
#define BIG_PIPE_DEPTH (2000)

#include <sycl/sycl.hpp>
using namespace sycl;

#include "types.h"
#include "debug.h"

template <int depth> struct TuplesBundle {
  HashSel hashSelect;
  Tuple tuples[depth];
};

// Pipe(s) from host to hash stage
using InputPipe0 = ext::intel::pipe<class pipe0_0, TuplesBundle<BundleDepth>, DFLT_PIPE_DEPTH>;

// Pipe(s) from hash to fork stage
using HashedTuplePipe0 = ext::intel::pipe<class pipe1_0, HashedTuple, DFLT_PIPE_DEPTH>;

// Pipe(s) from fork to hashTable stage
using ForkedTuplePipe0 = ext::intel::pipe<class pipe7_0, HashedTuple, DFLT_PIPE_DEPTH>;
using ForkedTuplePipe1 = ext::intel::pipe<class pipe7_1, HashedTuple, BIG_PIPE_DEPTH>;
using ForkedTuplePipe2 = ext::intel::pipe<class pipe7_2, HashedTuple, DFLT_PIPE_DEPTH>;
using ForkedTuplePipe3 = ext::intel::pipe<class pipe7_3, HashedTuple, BIG_PIPE_DEPTH>;

// Pipe(s) for successfully matched tuples from hashtable stage back to funnel
using MatchedTuplePipe0 = ext::intel::pipe<class pipe2_0, MatchedTuple, DFLT_PIPE_DEPTH>;
using MatchedTuplePipe1 = ext::intel::pipe<class pipe2_1, MatchedTuple, DFLT_PIPE_DEPTH>;
using MatchedTuplePipe2 = ext::intel::pipe<class pipe2_2, MatchedTuple, DFLT_PIPE_DEPTH>;
using MatchedTuplePipe3 = ext::intel::pipe<class pipe2_3, MatchedTuple, DFLT_PIPE_DEPTH>;

// Pipe(s) for unsuccessfully stored tuples from hashtable stage back to funnel
using UnmappableTuplePipe0 = ext::intel::pipe<class pipe3_0, HashedTuple, DFLT_PIPE_DEPTH>;
using UnmappableTuplePipe1 = ext::intel::pipe<class pipe3_1, HashedTuple, DFLT_PIPE_DEPTH>;
using UnmappableTuplePipe2 = ext::intel::pipe<class pipe3_2, HashedTuple, DFLT_PIPE_DEPTH>;
using UnmappableTuplePipe3 = ext::intel::pipe<class pipe3_3, HashedTuple, DFLT_PIPE_DEPTH>;

// Pipe(s) from hashtable to host
using HashedTablePipe0 = ext::intel::pipe<class pipe4_0, HashEntry, DFLT_PIPE_DEPTH>;
using HashedTablePipe1 = ext::intel::pipe<class pipe4_1, HashEntry, DFLT_PIPE_DEPTH>;
using HashedTablePipe2 = ext::intel::pipe<class pipe4_2, HashEntry, DFLT_PIPE_DEPTH>;
using HashedTablePipe3 = ext::intel::pipe<class pipe4_3, HashEntry, DFLT_PIPE_DEPTH>;

// Pipe from funnel to host
using FunnelledMatchedTuplePipe0 = ext::intel::pipe<class pipe8_0, MatchedTuple, BIG_PIPE_DEPTH>;
using FunnelledUnmappableTuplePipe0 = ext::intel::pipe<class pipe8_1, HashedTuple, BIG_PIPE_DEPTH>;


// Pipe(s) for timing reading and writing to the hash table
using HashTableTimingPipe0 = ext::intel::pipe<class pipe5_0, ClockCounter, DFLT_PIPE_DEPTH>;
using HashTableTimingPipe1 = ext::intel::pipe<class pipe5_1, ClockCounter, DFLT_PIPE_DEPTH>;
using HashTableTimingPipe2 = ext::intel::pipe<class pipe5_2, ClockCounter, DFLT_PIPE_DEPTH>;
using HashTableTimingPipe3 = ext::intel::pipe<class pipe5_3, ClockCounter, DFLT_PIPE_DEPTH>;

using TimeReportingPipe0 = ext::intel::pipe<class pipe6_0, ClockCounter, DFLT_PIPE_DEPTH>;
using TimeReportingPipe1 = ext::intel::pipe<class pipe6_1, ClockCounter, DFLT_PIPE_DEPTH>;
using TimeReportingPipe2 = ext::intel::pipe<class pipe6_2, ClockCounter, DFLT_PIPE_DEPTH>;
using TimeReportingPipe3 = ext::intel::pipe<class pipe6_3, ClockCounter, DFLT_PIPE_DEPTH>;

#endif