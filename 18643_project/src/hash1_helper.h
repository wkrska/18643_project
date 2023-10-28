#pragma once

#include "utils.h"

// Allocate memory on device and map pointers into the host 
void exp2_allocate_mem(cl_object &cl_obj, krnl_object &krnl_obj, int **ptr_out, int size_in_bytes);

// Unmap device memory when done
void exp2_deallocate_mem(cl_object &cl_obj, krnl_object &krnl_obj, int *ptr_out);

// Set kernel arguments and execute it 
void exp2_run_kernel(cl_object &cl_obj, krnl_object &krnl_obj);
