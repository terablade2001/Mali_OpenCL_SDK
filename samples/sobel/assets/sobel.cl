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
 * \brief Sobel filter kernel function.
 * \param[in] inputImage Input image data in row-major format.
 * \param[in] width Width of the image passed in as inputImage.
 * \param[out] outputImageDX Output image of the calculated gradient in the X direction.
 * \param[out] outputImageDY Output image of the calculated gradient in the Y direction.
 */
__kernel void sobel(__global const uchar* restrict inputImage,
                    const int width,
                    __global char* restrict outputImageDX,
                    __global char* restrict outputImageDY)
{
    /* [Kernel size] */
    /*
     * Each kernel calculates 16 output pixels in the same row (hence the '* 16').
     * column is in the range [0, width] in steps of 16.
     * row is in the range [0, height].
     */
    const int column = get_global_id(0) * 16;
    const int row = get_global_id(1) * 1;

    /* Offset calculates the position in the linear data for the row and the column. */
    const int offset = row * width + column;
    /* [Kernel size] */

    /* [Load row] */
    /*
     * First row of input.
     * In a scalar Sobel calculation you would load 1 value for leftLoad, middleLoad and rightLoad.
     * In the vector case we load 16 values for each.
     * leftLoad, middleLoad and rightLoad load 16-bytes of data from the first row.
     * The data they load overlaps. e.g. for the first column and row, leftLoad is 0->15, middleLoad is 1->16 and rightLoad is 2->17.
     * So we're actually loading 18-bytes of data from the first row.
     */
    uchar16 leftLoad = vload16(0, inputImage + (offset + 0));
    uchar16 middleLoad = vload16(0, inputImage + (offset + 1));
    uchar16 rightLoad = vload16(0, inputImage + (offset + 2));
    /* [Load row] */

    /* [Convert data] */
    /*
     * Convert the data from unsigned chars to shorts (8-bit unsigned to 16-bit signed).
     * The calculations can overflow 8-bits so we require larger intermediate storage.
     * Additionally, the values can become negative so we need a signed type.
     */
    short16 leftData = convert_short16(leftLoad);
    short16 middleData = convert_short16(middleLoad);
    short16 rightData = convert_short16(rightLoad);
    /* [Convert data] */

    /* [Calculation] */
    /*
     * Calculate the results for the first row.
     * Looking at the Sobel masks above for the first line of input,
     * the dX calculation is the sum of 1 * leftData, 0 * middleData, and -1 * rightData.
     * The dY calculation is the sum of 1 * leftData, 2 * middleData, and 1 * rightData.
     * This is what is being calculated below, except we have removed the
     * unnecessary calculations (multiplications by 1 or 0) and we are calculating 16 values at once.
     * This pattern repeats for the other 2 rows of data.
     */
    short16 dx = rightData - leftData;
    short16 dy = rightData + leftData + middleData * (short)2;
    /* [Calculation] */

    /*
     * Second row of input.
     * By adding the 'width * 1' to the offset we get the next row of data at the same column position.
     * middleData is not loaded because it is not used in any of the calculations.
     */
    leftLoad = vload16(0, inputImage + (offset + width * 1 + 0));
    rightLoad = vload16(0, inputImage + (offset + width * 1 + 2));

    leftData = convert_short16(leftLoad);
    rightData = convert_short16(rightLoad);

    /*
     * Calculate the results for the second row.
     * The dX calculation is the sum of -2 * leftData, 0 * middleData, and -2 * rightData.
     * There is no dY calculation to do: sum of 0 * leftData, 0 * middleData, and 0 * rightData.
     */
    dx += (rightData - leftData) * (short)2;

    /* Third row of input. */
    leftLoad = vload16(0, inputImage + (offset + width * 2 + 0));
    middleLoad = vload16(0, inputImage + (offset + width * 2 + 1));
    rightLoad = vload16(0, inputImage + (offset + width * 2 + 2));

    leftData = convert_short16(leftLoad);
    middleData = convert_short16(middleLoad);
    rightData = convert_short16(rightLoad);

    /*
     * Calculate the results for the third row.
     * The dX calculation is the sum of -1 * leftData, 0 * middleData, and -1 * rightData.
     * The dY calculation is the sum of -1 * leftData, -2 * middleData, and -1 * rightData.
     */
    dx += rightData - leftData;
    dy -= rightData + leftData + middleData * (short)2;

    /* [Store] */
    /*
     * Store the results.
     * The range of outputs from our Sobel calculations is [-1020, 1020].
     * In order to output this as an 8-bit signed char we must divide it by 8 (or shift right 3 times).
     * This gives the range [-128, 128]. Depending on what type of output you require,
     * (signed/unsigned, seperate/combined gradients) it is possible to do more of the calculations on the GPU using OpenCL.
     * In this sample we're assuming that the application requires signed uncombined gradient outputs.
     */
    vstore16(convert_char16(dx >> 3), 0, outputImageDX + offset + width + 1);
    vstore16(convert_char16(dy >> 3), 0, outputImageDY + offset + width + 1);
    /* [Store] */
}
