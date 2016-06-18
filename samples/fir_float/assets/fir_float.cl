/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *    (C) COPYRIGHT 2013 ARM Limited
 *        ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/* FW_SCALE = 1 / 256. */
#define FW_SCALE 0.00390625f
#define FW_UL (30.0f * FW_SCALE)
#define FW_UM (5.0f * FW_SCALE)
#define FW_UR (6.0f * FW_SCALE)
#define FW_CL (19.0f * FW_SCALE)
#define FW_CM (30.0f * FW_SCALE)
#define FW_CR (9.0f * FW_SCALE)
#define FW_BL (15.0f * FW_SCALE)
#define FW_BM (5.0f * FW_SCALE)
#define FW_BR (40.0f * FW_SCALE)

/**
 * \brief FIR filter kernel function.
 * \param[in] input Input image data in row-major format.
 * \param[in] width Width of the image passed in as input.
 * \param[out] output Output image after FIR has been applied. Resulting image depends on the coefficients used.
 */
__kernel void fir_float(__global const float* restrict input,
                        __global float* restrict output,
                        const int width)
{
    /* [Kernel size] */
    /*
     * Each kernel calculates 4 output pixels in the same row (hence the '* 4').
     * column is in the range [0, width] in steps of 4.
     * row is in the range [0, height].
     */
    const int column = get_global_id(0) * 4;
    const int row = get_global_id(1);
    /* Offset calculates the position in the linear data for the row and the column. */
    const int offset = row * width + column;
    /* [Kernel size] */

    /* Accumulator array of 4 floats. */
    float4 accumulator = (float4)0.0f;

    /*
     * As mentioned before the kernel works on a 6x3 window.
     * e.g. If the data in the input array is as follows:
     * [0  1  2  3  4  5 ]
     * [6  7  8  9  10 11]
     * [12 13 14 15 16 17]
     *
     * We load overlapping data from the first row into 3 vectors:
     *
     * data0 = [0 1 2 3]
     * data1 = [1 2 3 4]
     * data2 = [2 3 4 5]
     *
     * Which means the first result of accumulator will be equal to:
     * accumulator.s0 = data0.s0 * FW_UL + data1.s0 * FW_UM + data2.s0 * FW_UR
     *
     * The same is done for the second and third row, which makes acc.s0 the sum of 0, 1, 2, 6, 7, 8, 12, 13 and 14
     * multiplied by the corresponding coefficients.
     * If this indices are compared with the input array, we can see the 3x3 window of the FIR filter.
     */

    /* [Load first row] */
    /*
     * Access the first row in the 6x3 window to apply FW_U coefficients.
     * data1 can be constructed from the other vectors without doing an additional load.
     */
    float4 data0 = vload4(0, input + offset);
    float4 data2 = vload4(0, input + offset + 2);
    float4 data1 = (float4)(data0.s12, data2.s12);
    /* [Load first row] */

    /* [Filter first row] */
    accumulator += data0 * FW_UL;
    accumulator += data1 * FW_UM;
    accumulator += data2 * FW_UR;
    /* [Filter first row] */

    /* [Load and filter second and third row] */
    /* Access the second row in the 6x3 window and repeat the process, but with FW_C coefficients. */
    data0 = vload4(0, input + offset + width);
    data2 = vload4(0, input + offset + width + 2);
    data1 = (float4)(data0.s12, data2.s12);

    accumulator += data0 * FW_CL;
    accumulator += data1 * FW_CM;
    accumulator += data2 * FW_CR;

    /* Access the third row in the 6x3 window and repeat the process, but with FW_B coefficients. */
    data0 = vload4(0, input + offset + width * 2);
    data2 = vload4(0, input + offset + width * 2 + 2);
    data1 = (float4)(data0.s12, data2.s12);

    accumulator += data0 * FW_BL;
    accumulator += data1 * FW_BM;
    accumulator += data2 * FW_BR;
    /* [Load and filter second and third row] */

    /* [Store] */
    /* Store the accumulator. */
    vstore4(accumulator, 0, output + offset);
    /* [Store] */
}