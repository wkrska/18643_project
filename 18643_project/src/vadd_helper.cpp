#include "vadd_helper.h"

/****************************************************************
 * Copyright (c) 2017~2022, 18-643 Course Staff, CMU
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.

 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.

 * The views and conclusions contained in the software and
 * documentation are those of the authors and should not be
 * interpreted as representing official policies, either expressed or
 * implied, of the FreeBSD Project.
 ****************************************************************/

/*
 * The following code is repackaged from Vitis's default vector add
 * example.  It is hopefully easier to understand.
 */

// Allocate memory on device and map pointers into the host
void vadd_allocate_mem (cl_object &cl_obj, krnl_object &krnl_obj, int **ptr_a, int **ptr_b, int **ptr_result, int size_in_bytes) {
    cl_int err;

    // These commands will allocate memory on the Device. The cl::Buffer objects can
    // be used to reference the memory locations on the device.
    OCL_CHECK(err, krnl_obj.buffers.emplace_back(cl_obj.context, CL_MEM_READ_ONLY, size_in_bytes, nullptr, &err));
    OCL_CHECK(err, krnl_obj.buffers.emplace_back(cl_obj.context, CL_MEM_READ_ONLY, size_in_bytes, nullptr, &err));
    OCL_CHECK(err, krnl_obj.buffers.emplace_back(cl_obj.context, CL_MEM_WRITE_ONLY, size_in_bytes, nullptr, &err));

    cl::Buffer *buffer_a = &krnl_obj.buffers[0];
    cl::Buffer *buffer_b = &krnl_obj.buffers[1];
    cl::Buffer *buffer_result = &krnl_obj.buffers[2];

    //We then need to map our OpenCL buffers to get the pointers
    OCL_CHECK(err, (*ptr_a) = (int*)cl_obj.q.enqueueMapBuffer (*buffer_a , CL_TRUE , CL_MAP_WRITE , 0, size_in_bytes, NULL, NULL, &err));
    OCL_CHECK(err, (*ptr_b) = (int*)cl_obj.q.enqueueMapBuffer (*buffer_b , CL_TRUE , CL_MAP_WRITE , 0, size_in_bytes, NULL, NULL, &err));
    OCL_CHECK(err, (*ptr_result) = (int*)cl_obj.q.enqueueMapBuffer (*buffer_result , CL_TRUE , CL_MAP_READ , 0, size_in_bytes, NULL, NULL, &err));
}

// Unmap device memory when done
void vadd_deallocate_mem (cl_object &cl_obj, krnl_object &krnl_obj, int *ptr_a, int *ptr_b, int *ptr_result) {
    cl_int err;

    cl::Buffer *buffer_a = &krnl_obj.buffers[0];
    cl::Buffer *buffer_b = &krnl_obj.buffers[1];
    cl::Buffer *buffer_result = &krnl_obj.buffers[2];

    OCL_CHECK(err, err = cl_obj.q.enqueueUnmapMemObject(*buffer_a , ptr_a));
    OCL_CHECK(err, err = cl_obj.q.enqueueUnmapMemObject(*buffer_b , ptr_b));
    OCL_CHECK(err, err = cl_obj.q.enqueueUnmapMemObject(*buffer_result , ptr_result));
    OCL_CHECK(err, err = cl_obj.q.finish());
}

// Set kernel arguments and execute it
void vadd_run_kernel(cl_object &cl_obj, krnl_object &krnl_obj, int dim) {
    cl_int err;

    // get i/o buffers from kernel object
    cl::Buffer *buffer_a = &krnl_obj.buffers[0];
    cl::Buffer *buffer_b = &krnl_obj.buffers[1];
    cl::Buffer *buffer_result = &krnl_obj.buffers[2];

    std::cout << "Running kernel " << krnl_obj.name << "..." << std::endl;

    //set the kernel Arguments
    int narg=0;
    OCL_CHECK(err, err = krnl_obj.krnl.setArg(narg++, *buffer_a));
    OCL_CHECK(err, err = krnl_obj.krnl.setArg(narg++, *buffer_b));
    OCL_CHECK(err, err = krnl_obj.krnl.setArg(narg++, *buffer_result));
    OCL_CHECK(err, err = krnl_obj.krnl.setArg(narg++, dim));

    // Data will be migrated to kernel space
    OCL_CHECK(err, err = cl_obj.q.enqueueMigrateMemObjects({*buffer_a, *buffer_b},0/* 0 means from host*/));

    // Launch the Kernel; this is nonblocking.
    OCL_CHECK(err, err = cl_obj.q.enqueueTask(krnl_obj.krnl));

    // The result of the previous kernel execution will need to be retrieved in
    // order to view the results. This call will transfer the data from FPGA to
    // source_results vector
    OCL_CHECK(err, cl_obj.q.enqueueMigrateMemObjects({*buffer_result}, CL_MIGRATE_MEM_OBJECT_HOST));

    // wait for all tasks to finish
    OCL_CHECK(err, cl_obj.q.finish());
}

static const std::string vadd_error_message =
    "Error: Result mismatch:\n"
    "i = %d CPU result = %d Device result = %d\n";

// Verification functions
int vadd_check(int *ptr_a, int *ptr_b, int *ptr_result, int dim) {
    std::cout << "Verifying vadd result..." << std::endl;

    //Verify the result
    int match = 0;
    for (int i = 0; i < dim; i++) {
        int host_result = ptr_a[i] + ptr_b[i];
        if (ptr_result[i] != host_result) {
            printf(vadd_error_message.c_str(), i, host_result, ptr_result[i]);
            match = 1;
            break;
        }
    }

    return match;
}
