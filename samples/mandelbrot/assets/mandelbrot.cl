/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *    (C) COPYRIGHT 2013 ARM Limited
 *        ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/*
 * The number of iterations before we assume a point is in the Mandelbrot set.
 * Increasing this value has no effect since as we are returning the output as unsigned chars,
 * 255 is the maximum value that can be returned.
 */
#define MAX_ITER 255

/**
 * \brief Create a float4 containing the x positions of x and the 3 adjacent pixels.
 * \param[in] x First pixel position to use.
 * \return An vector of 4 floats containing x, x + 1, x + 2, and x + 3.
 */
float4 createStartX(int x)
{
    return (float4)(x, x + 1, x + 2, x + 3);
}

 /**
 * \brief Mandelbrot kernel function.
 * \details Evaluates whether a set of four adjacent pixels are part of the Mandelbrot
 *          set. The output is an array where each value represents how many iterations
 *          were required to be able to decide whether the point in imaginary space
 *          is part of the Mandelbrot set or not. A value of MAX_ITER represents a point
 *          that is part of the Mandelbrot set. All other values represent points that are
 *          not part of the Mandelbrot set.
 *          A value is in the Mandelbrot set if the results of the calculations remain bounded.
 * \param[out] output Output data buffer. Must be width * height * sizeof(cl_uchar) in size.
 * \param[in] width Width of the data required.
 * \param[in] height Height of the data required.
 */
__kernel void mandelbrot(__global uchar* restrict output,
                         const int width,
                         const int height)
{
    /* [Kernel size] */
    /*
     * Each kernel calculates 4 output pixels in the same row (hence the '* 4').
     * x is in the range [0, width] in steps of 4.
     * y is in the range [0, height].
     */
    int x = get_global_id(0) * 4;
    int y = get_global_id(1);
    /* [Kernel size] */

    /* [Create inputs] */
    /*
     * Calculate the coordinates of the four pixels in the real and imaginary space.
     * Scale the coordinates by the height and width such that the same image is produced for all values, just at different resolutions.
     * The real coordinate is scaled to the range [0, 2.5] (using '/ width * 2.5f') and then shifted to be in the range [-2, 0.5] (using '-2').
     * The imaginary coordinate is scaled to the range [0, 2] (using '/ ((float) height) * 2' and then shifted to be in the range [-1, 1] (using '-1').
     * The resulting ranges ([-2, 0.5], [-1, 1]) are the limits of the interesting parts of the Mandelbrot set.
     * Four pixels (adjacent in x (real) dimension) are calulated per kernel instance. Therefore, we calculate real coordinates for 4 pixels.
     */
    float4 initialReal = -2 + (createStartX(x) / (float)width * 2.5f);
    float4 initialImaginary = -1 + (y / (float)height * 2);

    float4 real = initialReal;
    float4 imaginary = initialImaginary;
    /* [Create inputs] */

    /* This will store the number of iterations for each of the four pixels */
    int4 iterationsPerPixel = (int4)(0, 0, 0, 0);
    /* Total number of iterations for this kernel instance. */
    int iterations = 0;
    int4 mask;

    /* [Calculation] */
    /*
     * Loop until we can say that all pixels are not part
     * of the Mandelbrot set or the maximum number of
     * iterations has been reached.
     */
    do
    {
        iterations++;
        if (iterations > MAX_ITER)
        {
            break;
        }

        /* Backup the real value as its used in both calculations but changed by the first. */
        float4 oldReal = real;

        /* Evaluate the equation. */
        real = real * real - imaginary * imaginary + initialReal;
        imaginary = 2 * oldReal * imaginary + initialImaginary;

        /*
         * Calculate which indices to update.
         * Mathematically, if the result of the calculation becomes greater that 2, it will continue to infinity.
         * Therefore, if the result becomes greater than 2 it cannot be part of the Mandelbrot set and we stop adding to it's iteration count.
         * To get the absolute value of the calculation we have squared and added the components, therefore we must test it against
         * 4 (2 squared).
         */
        float4 absoluteValue = real * real + imaginary * imaginary;
        /* For vector input isless(x, y) returns -1 per component if x < y. */
        mask = isless(absoluteValue, (float4) 4.0f);
        /* Increase the iterations per pixel (if they are less than 4). */
        iterationsPerPixel -= mask;
    } while(any(mask));
    /* [Calculation] */

    /* [Store] */
    /*
     * Write the result to the output buffer.
     * Convert the output to unsigned char to make it easier to write to a bitmap.
     */
    vstore4(convert_uchar4(iterationsPerPixel), 0, output + x + y * width);
    /* [Store] */
}
