// SETUP TEST CONDITION
#if 0
#define TESTLEN (1 << 16) // number TESTLEN tuples
#else
#define TESTLEN (1 << 5)
#endif


#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sycl/sycl.hpp>
using namespace sycl;

#include "core.h"
#include "pipes.h"
#include "stages.h"
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

TuplePadded16 testInput_host[TESTLEN];
TuplePadded16 testOutput_host[TESTLEN];
int main(/*int argc, char** argv*/) {

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

  //////////////////////
  // Set up test input
  //////////////////////

  for (UINT i = 0; i < TESTLEN; i++) {
    testInput_host[i].valid = true;
    testInput_host[i].val = i;
    testInput_host[i].key = i << 5;
  }

  //////////////////////
  // Set up and prepare device memory
  //////////////////////

  TuplePadded16 *testInput_device = malloc_device<TuplePadded16>(TESTLEN, q);
  TuplePadded16 *testOutput_device = malloc_device<TuplePadded16>(TESTLEN, q);

  q.memcpy(testInput_device, testInput_host, TESTLEN * sizeof(TuplePadded16));
  q.wait();

  // For everything we do, we run it as a "task". This task can exit or run indefinately

  // Stream in the data
  try {
    // Source Stage: inject testInput
    q.submit([&](handler &h) {
      h.single_task<class testInputSrcTask>([=]() {
        [[intel::initiation_interval(1)]] for (UINT i = 0; i < TESTLEN/TUPLEN; i++) {
          TuplesBundle<TUPLEN> bundle;
          for (UINT j = 0; j < TUPLEN; j++) {
            TuplePadded16 tuple16 = testInput_device[i * TUPLEN + j];
            bundle.tuples[j].valid = true;
            bundle.tuples[j].key = tuple16.key;
            bundle.tuples[j].val = tuple16.val;
          }
          bundle.eof = (i == TESTLEN - 1);

          // "pipe" is a type, not a variable.
          
          TestInputPipe0::write(bundle);
        }
      });
    });

    // increment tuples as an example, return to host
    q.submit([&](handler &h) {
      h.single_task<class IncrementTask0>([=]() {
        incrementStage<TestInputPipe0, ReadoutTuplePipe, TUPLEN>();
      });
    });

    // write to dram to communicate back to host
    auto ev = q.submit([&](handler &h) {
      h.single_task<class ReadoutTask>([=]() {
        [[intel::initiation_interval(1)]] for (UINT i = 0; i < TESTLEN; i++) {
          bool goodRead = false;
          Tuple tuple;
          while (!goodRead)
            tuple = ReadoutTuplePipe::read(goodRead);
          TuplePadded16 tuple16;
          tuple16.valid = tuple.valid;
          tuple16.key = tuple.key;
          tuple16.val = tuple.val;
          testOutput_device[i] = tuple16;
        }
      });
    });

    ev.wait();

  } catch (std::exception const &e) {
    std::cerr << "Exception!\n";
    std::terminate();
  }

  // Copy values back to host
  auto ev = q.memcpy(testOutput_host, testOutput_device,
                     TESTLEN * sizeof(TuplePadded16));
  ev.wait();

  // Print out values (val should be one more than the key)
  for (int i = 0; i < TESTLEN; i++) {
    printf("(%d:%d) key=%u incr=%u\n", i, testOutput_host[i].valid, testOutput_host[i].key >> 5, testOutput_host[i].val);
  }
  fflush(stdout);

  return 0;
}
