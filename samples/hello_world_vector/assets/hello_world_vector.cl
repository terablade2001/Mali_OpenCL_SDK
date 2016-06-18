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
 * \brief Vectorized Hello World kernel function.
 * \param[in] inputA First input array.
 * \param[in] inputB Second input array.
 * \param[out] output Output array.
 */
 /* [Vector Implementation] */
__kernel void hello_world_vector(__global int* restrict inputA,
                                 __global int* restrict inputB,
                                 __global int* restrict output)
{
    /*
     * We have reduced the global work size (n) by a factor of 4 compared to the hello_world_opencl sample.
     * Therefore, i will now be in the range [0, (n / 4) - 1].
     */
    int i = get_global_id(0);

    /*
     * Load 4 integers into 'a'.
     * The offset calculation is implicit from the size of the vector load.
     * For vloadN(i, p), the address of the first data loaded would be p + i * N.
     * Load from the data from the address: inputA + i * 4.
     */
    int4 a = vload4(i, inputA);
    /* Do the same for inputB */
    int4 b = vload4(i, inputB);

    /*
     * Do the vector addition.
     * Store the result at the address: output + i * 4.
     */
    vstore4(a + b, i, output);
}
/* [Vector Implementation] */
