#ifndef STAGES_H
#define STAGES_H

#include <sycl/sycl.hpp>
using namespace sycl;

#include "core.h"
#include "pipes.h"
#include "types.h"

// All streaming stages follows a common construction based on a main
// ii=1 loops, in many case infinite. An iteration should start every
// cycle, with the sequential loop body pipelined by the compiler to
// overlap many iterations.  The tasks have no arguments.  Only input
// and output are done through pipes.  Because pipes are types,
// template is used to create the semblance of "pipe arguments".

template <typename TTuplesInPipe, typename TTuplesOutPipe, int len>
void incrementStage() {

  // Most of the code that used to be here I cut out, but I think some of it is important to see! 
  // Take a look into `core.h`, try and get a grasp on what is going on in there
  // Specifically, look at what the following macros are doing

  // // accumulation table in TNumTuple banks, duplicated for each tuple lane
  // AGG_DECLARE(TNumPipeline, TNumTuple, TNumGroup);
  // AGG_ZERO_TABLE();
  // AGG_UPDATE(iBundle.tuples[ppln], iBundle.idx[ppln], ppln);

  [[intel::disable_loop_pipelining]] while (1) {
    // Can use this for setup time, take as longggg as you want

    // recognize this from class?
    bool done = false;
    bool done_ = false;
    bool done__ = false;

    // main loop, iterate until end-of-frame arrives
    [[intel::initiation_interval(1)]] while (!done) {
      // input valid
      BOOL iValid = false;
      // input bundle
      [[intel::fpga_register]] TuplesBundle<len>
          iBundle;
      // Output bundle
      [[intel::fpga_register]] TuplesBundle<len>
          oBundle;

      // go round 2 more times without fetching new bundle to finish
      // deferred updates
      done = done_;
      done_ = done__;

      /* There are two ways to perform reads: blocking and non-blocking.
        The only syntactical difference is whehter you pass in a boolean value to it (like below)
        When passing the bool, it will assign whether it successfully read a value from the pipe
        If you include no args, it just won't continue until there is a value to read
      */
      if (!done__) {
        iBundle = TTuplesInPipe::read(iValid);
      }
      
      // only do the following if you read a value
      if (iValid) {
        #pragma unroll
        for (UIDX i = 0; i < len; i++) {
          oBundle.tuples[i].valid = iBundle.tuples[i].valid;
          oBundle.tuples[i].key = iBundle.tuples[i].key; // increment by one
          oBundle.tuples[i].val = iBundle.tuples[i].val + 1; // increment by one
          // there use to be a really read heavy thing here (AGG_UPDATE) but we don't need that rn, good to keep in mind
        }

        oBundle.eof = iBundle.eof;

        if (iBundle.eof) {
          done__ = true;
        }

        // writeout
        for (UIDX i = 0; i < len; i++) {
          TTuplesOutPipe::write(oBundle.tuples[i]);
        }
      }
    }
  }
}
#endif
