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
 * \brief  Long data type (64-bit integer) kernel.
 * \details This kernel loads 8 pixel values to calculate the square of each pixel value. Then it accumulates the
 * square of pixels and the sum of pixels values in the respective accumulators.
 * \param[in] imagePixels Input array with image pixels.
 * \param[in] squareOfPixels Sum of the square of pixel values.
 * \param[out] sumOfPixels Sum of pixel values.
 */

/* [Enable atom_add extension] */
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable
/* [Enable atom_add extension] */

__kernel void long_vectors(__global uchar* restrict imagePixels,
                           __global ulong* restrict squareOfPixels,
                           __global ulong* restrict sumOfPixels)
{
    /*
     * Set i to be the ID of the kernel instance.
     * If the global work size (set by clEnqueueNDRangeKernel) is n,
     * then n kernels will be run and i will be in the range [0, n - 1].
     */
    int i = get_global_id(0);

    /* [Squares and sums]*/
    /* Load 8 pixels (char) and convert them to shorts to calculate the square.*/
    ushort8 pixelShort = convert_ushort8(vload8(i, imagePixels));
    /* Square of 255 < 2 ^ 16. */
    ushort8 newSquareShort = pixelShort * pixelShort;

    /*
     * Convert original pixel value and the square to longs to sum
     * all the vectors together and add the final values to the
     * respective accumulators.
     */
    ulong8 pixelLong = convert_ulong8(pixelShort);
    ulong8 newSquareLong = convert_ulong8(newSquareShort);

    /*
     * Use vector data type suffixes (.lo and .hi) to get smaller vector types,
     * until we obtain one single value.
     */
    ulong4 sumLongPixels1 = pixelLong.hi + pixelLong.lo;
    ulong2 sumLongPixels2 = sumLongPixels1.hi + sumLongPixels1.lo;
    ulong sumLongPixels3 = sumLongPixels2.hi + sumLongPixels2.lo;

    ulong4 sumLongSquares1 = newSquareLong.hi + newSquareLong.lo;
    ulong2 sumLongSquares2 = sumLongSquares1.hi + sumLongSquares1.lo;
    ulong sumLongSquares3 = sumLongSquares2.hi + sumLongSquares2.lo;
    /* [Squares and sums]*/

    /*
     * As all the kernels are accessing sumOfPixels
     * and squareOfPixels at the same time,
     * we use atom_add to ensure only one kernel
     * at a time can access the given variables.
     * This means that this operation is very expensive,
     * so we want to use it only when necessary.
     */
    /* [Atomic transaction] */
    atom_add(sumOfPixels, sumLongPixels3);
    atom_add(squareOfPixels, sumLongSquares3);
    /* [Atomic transaction] */
}
