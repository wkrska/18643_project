#ifndef PIPES_H
#define PIPES_H

#define DFLT_PIPE_DEPTH (16)

#include <sycl/sycl.hpp>
using namespace sycl;

#include "types.h"

template <int len> struct TuplesBundle {
  Tuple tuples[len];
  BOOL eof;
};

// pipes from input source (in main.cpp) to uniquify stage
using TestInputPipe0 =  ext::intel::pipe<class pipe0_0, TuplesBundle<TUPLEN>, DFLT_PIPE_DEPTH>;

using ReadoutTuplePipe = ext::intel::pipe<class pipe_out, Tuple, TUPLEN*10>;

#endif
