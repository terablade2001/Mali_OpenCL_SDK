/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *    (C) COPYRIGHT 2013 ARM Limited
 *        ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include <iostream>

using namespace std;

/**
 * \brief Basic integer array addition implemented in C/C++.
 * \details A sample which shows how to add two integer arrays and store the result in a third array.
 *          No OpenCL code is used in this sample, only standard C/C++. The code executes only on the CPU.
 * \return The exit code of the application, non-zero if a problem occurred.
 */
int main(void)
{
    /* [Setup memory] */
    /* Number of elements in the arrays of input and output data. */
    int arraySize = 1000000;

    /* Arrays to hold the input and output data. */
    int* inputA = new int[arraySize];
    int* inputB = new int[arraySize];
    int* output = new int[arraySize];
    /* [Setup memory] */

    /* Fill the arrays with data. */
    for (int i = 0; i < arraySize; i++)
    {
        inputA[i] = i;
        inputB[i] = i;
    }

    /* [C/C++ Implementation] */
    for (int i = 0; i < arraySize; i++)
    {
        output[i] = inputA[i] + inputB[i];
    }
    /* [C/C++ Implementation] */

    /* Uncomment the following block to print results. */
    /*
    for (int i = 0; i < arraySize; i++)
    {
        cout << "i = " << i << ", output = " <<  output[i] << "\n";
    }
    */

    delete[] inputA;
    delete[] inputB;
    delete[] output;
}
