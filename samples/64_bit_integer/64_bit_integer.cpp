/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *    (C) COPYRIGHT 2013 ARM Limited
 *        ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include "common.h"
#include "image.h"

#include <CL/cl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstddef>
#include <cmath>

using namespace std;

/**
 * \brief  Long data type (64-bit integer) OpenCL example.
 * \details An example to calculate, for an image:
 *          - the sum of the squares of the pixels values
 *          - sum of the pixels values.
 *          Makes use of the long data type and 64-bit atomics.
 *          The main calculation code is in an OpenCL kernel which is executed on a GPU device.
 * \return The exit code of the application, non-zero if a problem occurred.
 */
int main(void)
{
    string filename = "assets/input.bmp";

    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    const int numberOfMemoryObjects = 3;
    /* Index values for the memory objects. */
    const unsigned int imagePixelsIndex = 0;
    const unsigned int squareIndex = 1;
    const  unsigned int sumIndex = 2;
    cl_mem memoryObjects[3] = {0, 0, 0};
    cl_int errorNumber;


    if (!createContext(&context))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create an OpenCL context. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    if (!createCommandQueue(context, &commandQueue, &device))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create the OpenCL command queue. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Checking 64-bit integer atomics extension support. */
    if (!isExtensionSupported (device, "cl_khr_int64_base_atomics"))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "cl_khr_int64_base_atomics is not supported on this device. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    if (!createProgram(context, device, "assets/64_bit_integer.cl", &program))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL program." << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    kernel = clCreateKernel(program, "long_vectors", &errorNumber);
    if (!checkSuccess(errorNumber))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL kernel. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Load 24-bits per pixel RGB data from a bitmap. */
    cl_int width;
    cl_int height;
    unsigned char* loadedRGBData = NULL;
    if (!loadFromBitmap(filename, &width, &height, &loadedRGBData))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed loading bitmap. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Buffer for the image pixels. */
    size_t bufferSizeChar = width * height * sizeof(unsigned char);
    /* Buffer for the accumulators*/
    size_t bufferSizeLong = sizeof(cl_ulong);

    /*
     * Ask the OpenCL implementation to allocate buffers for the data.
     * We ask the OpenCL implementation to allocate memory rather than allocating
     * it on the CPU to avoid having to copy the data later.
     * The read/write flags relate to accesses to the memory from within the kernel.
     */
    bool createMemoryObjectsSuccess = true;

    memoryObjects[imagePixelsIndex] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSizeChar, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);

    memoryObjects[squareIndex] = clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_ALLOC_HOST_PTR, bufferSizeLong, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);

    memoryObjects[sumIndex] = clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_ALLOC_HOST_PTR, bufferSizeLong, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);

    if (!createMemoryObjectsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL buffer. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Map the input memory objects to a host side pointers. */
    bool mapMemoryObjectsSuccess = true;
    cl_uchar* inputImagePixels = (cl_uchar*)clEnqueueMapBuffer(commandQueue, memoryObjects[imagePixelsIndex], CL_TRUE, CL_MAP_WRITE, 0, bufferSizeChar, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);
    cl_ulong* inputSquareOfPixels = (cl_ulong*)clEnqueueMapBuffer(commandQueue, memoryObjects[squareIndex], CL_TRUE, CL_MAP_WRITE, 0, bufferSizeLong, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);
    cl_ulong* inputSumOfPixels = (cl_ulong*)clEnqueueMapBuffer(commandQueue, memoryObjects[sumIndex], CL_TRUE, CL_MAP_WRITE, 0, bufferSizeLong, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);

    if (!mapMemoryObjectsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Mapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /*
     * Convert 24-bits per pixel RGB into 8-bits per pixel luminance data
     * and fill the array for the kernel.
     */
    RGBToLuminance(loadedRGBData, inputImagePixels, width, height);
    delete [] loadedRGBData;

    /* Ensure the accumulators are initialized to zero. */
    *inputSquareOfPixels = 0;
    *inputSumOfPixels = 0;

    /* Unmap the memory so we can pass it to the kernel. */
    bool unmapMemoryObjectsSuccess = true;
    unmapMemoryObjectsSuccess &= checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[imagePixelsIndex], inputImagePixels, 0, NULL, NULL));
    unmapMemoryObjectsSuccess &= checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[squareIndex], inputSquareOfPixels, 0, NULL, NULL));
    unmapMemoryObjectsSuccess &= checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[sumIndex], inputSumOfPixels, 0, NULL, NULL));

    if (!unmapMemoryObjectsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Set the kernel arguments */
    bool setKernelArgumentsSuccess = true;
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, imagePixelsIndex, sizeof(cl_mem), &memoryObjects[imagePixelsIndex]));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, squareIndex, sizeof(cl_mem), &memoryObjects[squareIndex]));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, sumIndex, sizeof(cl_mem), &memoryObjects[sumIndex]));

    if (!setKernelArgumentsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed setting OpenCL kernel arguments. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* An event to associate with the Kernel. Allows us to retrieve profiling information later. */
    cl_event event = 0;

    /*
      * Each instance of the kernel operates on a 8 * 1 portion of the image.
     * Therefore, the global work size must be 1.
     */
    size_t globalWorksize[1] = {(width * height) / 8};
    int work_dim = 1;
    /* Enqueue the kernel */
    if (!checkSuccess(clEnqueueNDRangeKernel(commandQueue, kernel, work_dim, NULL, globalWorksize, NULL, 0, NULL, &event)))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed enqueuing the kernel. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Wait for kernel execution completion. */
    if (!checkSuccess(clFinish(commandQueue)))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed waiting for kernel execution to finish. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Print the profiling information for the event. */
    printProfilingInfo(event);
    /* Release the event object. */
    if (!checkSuccess(clReleaseEvent(event)))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Failed releasing the event object. " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Get pointers to the output data. */
    mapMemoryObjectsSuccess = true;
    cl_ulong* squareOfPixels = (cl_ulong*)clEnqueueMapBuffer(commandQueue, memoryObjects[squareIndex], CL_TRUE, CL_MAP_READ, 0, bufferSizeLong, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);

    cl_ulong* sumOfPixels = (cl_ulong*)clEnqueueMapBuffer(commandQueue, memoryObjects[sumIndex], CL_TRUE, CL_MAP_READ, 0, bufferSizeLong, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);
    if (!mapMemoryObjectsSuccess)
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Failed to map buffer. " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* [Output the results] */
    cout << "Square of the pixel values = " <<  *squareOfPixels << "\n";
    cout << "Sum of the pixel values = " <<  *sumOfPixels << endl;
    /* [Output the results] */

    /* Unmap the memory object as we are finished using them from the CPU side. */
    unmapMemoryObjectsSuccess = true;
    unmapMemoryObjectsSuccess &= checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[squareIndex], squareOfPixels, 0, NULL, NULL));
    unmapMemoryObjectsSuccess &= checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[sumIndex], sumOfPixels, 0, NULL, NULL));

    if (!unmapMemoryObjectsSuccess)
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Release OpenCL objects. */
    cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
}
