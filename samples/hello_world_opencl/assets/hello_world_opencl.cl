/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *    (C) COPYRIGHT 2013 ARM Limited
 *        ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * \brief Hello World kernel function.
 * \param[in] inputA First input array.
 * \param[in] inputB Second input array.
 * \param[out] output Output array.
 */
/* [OpenCL Implementation] */
__kernel void hello_world_opencl(__global int* restrict inputA,
                                 __global int* restrict inputB,
                                 __global int* restrict output)
{
    /*
     * Set i to be the ID of the kernel instance.
     * If the global work size (set by clEnqueueNDRangeKernel) is n,
     * then n kernels will be run and i will be in the range [0, n - 1].
     */
    int i = get_global_id(0);

    /* Use i as an index into the three arrays. */
    output[i] = inputA[i] + inputB[i];
}
/* [OpenCL Implementation] */
