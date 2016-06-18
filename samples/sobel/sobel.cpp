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
 * \brief Simple Sobel filter OpenCL sample.
 * \details A sample which loads a bitmap and then passes it to the GPU.
 *          An OpenCL kernel which does Sobel filtering is then run on the data.
 *          The gradients of the image in x and y directions are returned by the GPU
 *          and are combined on the CPU to form the filtered data.
 *          The input image is loaded from assets/input.bmp. The output gradients in X and Y,
 *          as well as the combined gradient image are stored in output-dX.bmp, output-dY.bmp
 *          and output.bmp respectively.
 * \return The exit code of the application, non-zero if a problem occurred.
 */
int main(void)
{
    /*
     * Name of the bitmap to load and run the sobel filter on.
     * It's width must be divisible by 16.
     */
    string filename = "assets/input.bmp";

    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    const unsigned int numberOfMemoryObjects = 3;
    cl_mem memoryObjects[numberOfMemoryObjects] = {0, 0, 0};
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

    if (!createProgram(context, device, "assets/sobel.cl", &program))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL program." << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    kernel = clCreateKernel(program, "sobel", &errorNumber);
    if (!checkSuccess(errorNumber))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL kernel. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Load 24-bits per pixel RGB data from a bitmap. */
    cl_int width;
    cl_int height;
    unsigned char* imageData = NULL;
    if (!loadFromBitmap(filename, &width, &height, &imageData))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed loading bitmap. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* All buffers are the size of the image data. */
    size_t bufferSize = width * height * sizeof(cl_uchar);

    bool createMemoryObjectsSuccess = true;
    /* Create one input buffer for the image data, and two output buffers for the gradients in X and Y respectively. */
    memoryObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSize, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);
    memoryObjects[1] = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSize, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);
    memoryObjects[2] = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSize, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);
    if (!createMemoryObjectsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL buffers. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Map the input luminance memory object to a host side pointer. */
    cl_uchar* luminance = (cl_uchar*)clEnqueueMapBuffer(commandQueue, memoryObjects[0], CL_TRUE, CL_MAP_WRITE, 0, bufferSize, 0, NULL, NULL, &errorNumber);
    if (!checkSuccess(errorNumber))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Mapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Convert 24-bits per pixel RGB into 8-bits per pixel luminance data. */
    RGBToLuminance(imageData, luminance, width, height);

    delete [] imageData;

    /* Unmap the memory so we can pass it to the kernel. */
    if (!checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[0], luminance, 0, NULL, NULL)))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    bool setKernelArgumentsSuccess = true;
    /* Setup the kernel arguments. */
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 0, sizeof(cl_mem), &memoryObjects[0]));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 1, sizeof(cl_int), &width));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 2, sizeof(cl_mem), &memoryObjects[1]));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 3, sizeof(cl_mem), &memoryObjects[2]));
    if (!setKernelArgumentsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed setting OpenCL kernel arguments. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* An event to associate with the Kernel. Allows us to retreive profiling information later. */
    cl_event event = 0;

    /* [Kernel size] */
    /*
     * Each instance of the kernel operates on a 16 * 1 portion of the image.
     * Therefore, the global work size must be width / 16 by height / 1 work items.
     */
    size_t globalWorksize[2] = {width / 16, height / 1};
    /* [Kernel size] */

    /* Enqueue the kernel */
    if (!checkSuccess(clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorksize, NULL, 0, NULL, &event)))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed enqueuing the kernel. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Wait for completion */
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

    /* Map the arrays holding the output gradients. */
    bool mapMemoryObjectsSuccess = true;
    cl_char* outputDx = (cl_char*)clEnqueueMapBuffer(commandQueue, memoryObjects[1], CL_TRUE, CL_MAP_READ, 0, bufferSize, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);
    cl_char* outputDy = (cl_char*)clEnqueueMapBuffer(commandQueue, memoryObjects[2], CL_TRUE, CL_MAP_READ, 0, bufferSize, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);
    if (!mapMemoryObjectsSuccess)
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Mapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /*
     * In order to better visualise the data, we take the absolute
     * value of the gradients and then combine them together.
     * This is work which could be done in the OpenCL kernel, it all depends
     * on what type of output you require.
     */

    /* To visualise the data we take the absolute values of the gradients. */
    unsigned char *absDX = new unsigned char[width * height];
    unsigned char *absDY = new unsigned char[width * height];
    for (int i = 0; i < width * height; i++)
    {
        absDX[i] = abs(outputDx[i]);
        absDY[i] = abs(outputDy[i]);
    }

    /* Unmap the memory. */
    bool unmapMemoryObjectsSuccess = true;
    unmapMemoryObjectsSuccess &= checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[1], outputDx, 0, NULL, NULL));
    unmapMemoryObjectsSuccess &= checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[2], outputDy, 0, NULL, NULL));
    if (!unmapMemoryObjectsSuccess)
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Release OpenCL objects. */
    cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);

    /* Convert the two output luminance arrays to RGB and save them out to files. */
    unsigned char* rgbOut = new unsigned char[width * height * 3];
    luminanceToRGB(absDX, rgbOut, width, height);
    saveToBitmap("output-dX.bmp", width, height, rgbOut);

    luminanceToRGB(absDY, rgbOut, width, height);
    saveToBitmap("output-dY.bmp", width, height, rgbOut);

    /* Calculate the total gradient of the image, convert it to RGB and store it out to a file. */
    unsigned char* totalOutput = new unsigned char[width * height];
    for (int index = 0; index < width * height; index++)
    {
        totalOutput[index] = sqrt(pow(absDX[index], 2) + pow(absDY[index], 2));
    }
    luminanceToRGB(totalOutput, rgbOut, width, height);
    saveToBitmap("output.bmp", width, height, rgbOut);

    delete [] absDX;
    delete [] absDY;
    delete [] rgbOut;
    delete [] totalOutput;

    return 0;
}
