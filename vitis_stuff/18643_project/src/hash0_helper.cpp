#include "exp1_helper.h"
#include <sys/time.h>

// Allocate memory on device and map pointers into the host
void exp1_allocate_mem (cl_object &cl_obj, krnl_object &krnl_obj, int **ptr_in, int **ptr_out, int size_in_bytes) {
    cl_int err;

    // These commands will allocate memory on the Device. The cl::Buffer objects can
    // be used to reference the memory locations on the device.
    OCL_CHECK(err, krnl_obj.buffers.emplace_back(cl_obj.context, CL_MEM_READ_ONLY, size_in_bytes, nullptr, &err)); // in
    OCL_CHECK(err, krnl_obj.buffers.emplace_back(cl_obj.context, CL_MEM_READ_WRITE, 1 * sizeof(int), nullptr, &err)); // out

    cl::Buffer *buffer_in = &krnl_obj.buffers[0];
    cl::Buffer *buffer_out = &krnl_obj.buffers[1];

    //We then need to map our OpenCL buffers to get the pointers
    OCL_CHECK(err, (*ptr_in) = (int*)cl_obj.q.enqueueMapBuffer (*buffer_in , CL_TRUE , CL_MAP_WRITE , 0, size_in_bytes, NULL, NULL, &err));
    OCL_CHECK(err, (*ptr_out) = (int*)cl_obj.q.enqueueMapBuffer (*buffer_out , CL_TRUE , CL_MAP_READ , 0, 1 * sizeof(int), NULL, NULL, &err));
}

// Unmap device memory when done
void exp1_deallocate_mem (cl_object &cl_obj, krnl_object &krnl_obj, int *ptr_in, int *ptr_out) {
    cl_int err;

    cl::Buffer *buffer_in = &krnl_obj.buffers[0];
    cl::Buffer *buffer_out = &krnl_obj.buffers[1];

    OCL_CHECK(err, err = cl_obj.q.enqueueUnmapMemObject(*buffer_in , ptr_in));
    OCL_CHECK(err, err = cl_obj.q.enqueueUnmapMemObject(*buffer_out, ptr_out));
    OCL_CHECK(err, err = cl_obj.q.finish());
}

// Set kernel arguments and execute it
void exp1_run_kernel(cl_object &cl_obj, krnl_object &krnl_obj) {
    cl_int err;

    // Copied directly from vadd example
    cl::Buffer *buffer_in = &krnl_obj.buffers[0];
    cl::Buffer *buffer_out = &krnl_obj.buffers[1];

    std::cout << "Running kernel " << krnl_obj.name << "..." << std::endl;

    // Copied directly from vadd example
    int narg=0;
    OCL_CHECK(err, err = krnl_obj.krnl.setArg(narg++, *buffer_in));
    OCL_CHECK(err, err = krnl_obj.krnl.setArg(narg++, *buffer_out));

    std::cout << "Args Set" << std::endl;

    /* Measure time from start of data loading to end of result downloading, and for just the time of execution
        (Mostly copied from lab 0 starter code)
    */
    struct timeval start_time, end_time;

    // Queue migrate data to kernel
    OCL_CHECK(err, err = cl_obj.q.enqueueMigrateMemObjects({*buffer_in},0/* 0 means from host*/));
    OCL_CHECK(err, cl_obj.q.finish());

    std::cout << "Data loaded" << std::endl;
    std::cout << "Start captured" << std::endl;

    // Get "compute" runtime start time
    gettimeofday(&start_time, NULL);

    // Queue start of kernel, wait until finished
    OCL_CHECK(err, err = cl_obj.q.enqueueTask(krnl_obj.krnl));
    OCL_CHECK(err, cl_obj.q.finish());

    // Get "compute" runtime end time
    gettimeofday(&end_time, NULL);
    std::cout << "Execution Finished" << std::endl;
    std::cout << "End captured" << std::endl;

    OCL_CHECK(err, cl_obj.q.enqueueMigrateMemObjects({*buffer_out}, CL_MIGRATE_MEM_OBJECT_HOST));
    OCL_CHECK(err, cl_obj.q.finish());

    // Print runtimes
    //double timeusec = (end_time.tv_sec - start_time.tv_sec) * 1e6 +
    //                (end_time.tv_usec - start_time.tv_usec);

    //std::cout << "Runtime: " << timeusec << std::endl;

    double timeusec = (end_time.tv_sec - start_time.tv_sec) * 1e6 +
                    (end_time.tv_usec - start_time.tv_usec);
    double total_bytes = 4096 * 4096 * 4;
    double bytes_per_gigabyte = 1073741824;
    double total_gigabytes = total_bytes / bytes_per_gigabyte;
    double timesec = timeusec * 1e-6;
    double gigabytes_per_second = total_gigabytes / timesec;
    double bytes_per_second = total_bytes / timesec;

    std::cout << "Runtime (usec): " << timeusec << std::endl;
    std::cout << "GB per second: " << gigabytes_per_second << std::endl;
    std::cout << "bytes per second: " << bytes_per_second << std::endl;
}
