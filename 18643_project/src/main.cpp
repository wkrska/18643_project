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

#include "vadd_helper.h"
#define VADD_DIM 4096

#include "mmm_helper.h"
#define MMM_DIM 128

#define EXP_DIM 4096
#define EXP2_DIM 4096
#define MATMUL_DIM 640
//#define EXP2_DIM 128

#include "exp1_helper.h"
#include "exp2_helper.h"
#include "exp3_helper.h"


int main(int argc, char* argv[]) {

	// Hard coding xclbin filenames, ignoring command line arguments
  std::string xclbinFilename[7] = {
    "binary_container_vadd.xclbin",
    "binary_container_mmm.xclbin",
    "binary_container_exp1a.xclbin",
    "binary_container_exp1b.xclbin",
    "binary_container_exp2a.xclbin",
    "binary_container_exp2b.xclbin",
    "binary_container_exp3.xclbin"
  };

  cl_object cl_obj;

  initialize_device(cl_obj);

#if 0
    {
        // Read vadd
        read_xclbin(xclbinFilename[0], cl_obj.bins);

        krnl_object vadd_obj;
        vadd_obj.index = 0;
        vadd_obj.name = "krnl_vadd";

        int *ptr_a, *ptr_b, *ptr_result;

        program_kernel(cl_obj, vadd_obj);
        vadd_allocate_mem(cl_obj, vadd_obj, &ptr_a, &ptr_b, &ptr_result, VADD_DIM * sizeof(int));
        initialize_memory_int(ptr_a, VADD_DIM);
        initialize_memory_int(ptr_b, VADD_DIM);
        vadd_run_kernel(cl_obj, vadd_obj, VADD_DIM);
        int match = vadd_check(ptr_a, ptr_b, ptr_result, VADD_DIM);
        std::cout << "VADD TEST " << (match ? "FAILED" : "PASSED") << "\n" << std::endl;
        vadd_deallocate_mem(cl_obj, vadd_obj, ptr_a, ptr_b, ptr_result);
    }
#endif

#if 1
    {
        // Read mmm
        read_xclbin(xclbinFilename[1], cl_obj.bins);

        krnl_object mmm_obj;
        mmm_obj.index = 0;
        mmm_obj.name = "krnl_mmm";

        float *ptr_a, *ptr_b, *ptr_result;

        //int mmm_size = (atoi(argv[argc-1])) ? MMM_DIM : atoi(argv[argc-1]);
        int mmm_size = MATMUL_DIM;
        std::cout << "Size = " << mmm_size << std::endl;

        program_kernel(cl_obj, mmm_obj);
        mmm_allocate_mem(cl_obj, mmm_obj, &ptr_a, &ptr_b, &ptr_result, mmm_size * mmm_size * sizeof(float));
        initialize_memory_fp(ptr_a, mmm_size * mmm_size);
        initialize_memory_fp(ptr_b, mmm_size * mmm_size);
        mmm_run_kernel(cl_obj, mmm_obj, mmm_size);
        int match = mmm_check(ptr_a, ptr_b, ptr_result, mmm_size);
        std::cout << "MMM TEST " << (match ? "FAILED" : "PASSED") << "\n" << std::endl;
        mmm_deallocate_mem(cl_obj, mmm_obj, ptr_a, ptr_b, ptr_result);
    }
#endif

#if 1
    // Exp 1a
    {
        read_xclbin(xclbinFilename[2], cl_obj.bins);

        krnl_object exp1a_obj;
        exp1a_obj.index = 1;
        exp1a_obj.name = "krnl_exp1a";

        int *ptr_in, *ptr_out;

        program_kernel(cl_obj, exp1a_obj);
        exp1_allocate_mem(cl_obj, exp1a_obj, &ptr_in, &ptr_out, EXP_DIM * EXP_DIM * sizeof(int));
        initialize_memory_int(ptr_in, EXP_DIM * EXP_DIM);
        initialize_memory_int(ptr_out, 1);
        exp1_run_kernel(cl_obj, exp1a_obj);
        exp1_deallocate_mem(cl_obj, exp1a_obj, ptr_in, ptr_out);
        std::cout << "exp1a completed\n\n";
    }
#endif

#if 1

    // Exp 1b
    {
        read_xclbin(xclbinFilename[3], cl_obj.bins);

        krnl_object exp1b_obj;
        exp1b_obj.index = 2; // 3;
        exp1b_obj.name = "krnl_exp1b";

        int *ptr_in, *ptr_out;

        program_kernel(cl_obj, exp1b_obj);
        exp1_allocate_mem(cl_obj, exp1b_obj, &ptr_in, &ptr_out, EXP_DIM * EXP_DIM * sizeof(int));
        initialize_memory_int(ptr_in, EXP_DIM * EXP_DIM);
        initialize_memory_int(ptr_out, 1);
        exp1_run_kernel(cl_obj, exp1b_obj);
        exp1_deallocate_mem(cl_obj, exp1b_obj, ptr_in, ptr_out);
        std::cout << "exp1b completed\n\n";
    }
#endif

#if 1
    // Exp 2a
    {
        read_xclbin(xclbinFilename[4], cl_obj.bins);

        krnl_object exp2a_obj;
        exp2a_obj.index = 3;
        exp2a_obj.name = "krnl_exp2a";

        int *ptr_result;

        program_kernel(cl_obj, exp2a_obj);
        exp2_allocate_mem(cl_obj, exp2a_obj, &ptr_result, EXP2_DIM * EXP2_DIM * sizeof(int));
        exp2_run_kernel(cl_obj, exp2a_obj);
        exp2_deallocate_mem(cl_obj, exp2a_obj, ptr_result);
    }
#endif

#if 1
    // Exp 2b
    {
        read_xclbin(xclbinFilename[5], cl_obj.bins);

        krnl_object exp2b_obj;
        exp2b_obj.index = 4;
        exp2b_obj.name = "krnl_exp2b";

        int *ptr_result;

        program_kernel(cl_obj, exp2b_obj);
        exp2_allocate_mem(cl_obj, exp2b_obj, &ptr_result, EXP2_DIM * EXP2_DIM * sizeof(int));
        exp2_run_kernel(cl_obj, exp2b_obj);
        exp2_deallocate_mem(cl_obj, exp2b_obj, ptr_result);
    }
#endif

#if 1
    // Exp 3
    {
        read_xclbin(xclbinFilename[6], cl_obj.bins);

        krnl_object exp3_obj;
        exp3_obj.index = 5;
        exp3_obj.name = "krnl_exp3";

        int *ptr_in, *ptr_out;

        program_kernel(cl_obj, exp3_obj);
        exp3_allocate_mem(cl_obj, exp3_obj, &ptr_in, &ptr_out, EXP_DIM * EXP_DIM * sizeof(int));
        initialize_memory_int(ptr_in, EXP_DIM * EXP_DIM);
        initialize_memory_int(ptr_out, 1); 
        exp3_run_kernel(cl_obj, exp3_obj);
        exp3_deallocate_mem(cl_obj, exp3_obj, ptr_in, ptr_out);
    }
#endif
}
