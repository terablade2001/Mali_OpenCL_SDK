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
 * \brief Simple FIR filter OpenCL sample.
 * \details A sample which loads an image from assets/input.bmp and then passes it to the GPU.
 *          An OpenCL kernel applies FIR filtering on the data and
 *          the output image data is stored in output.bmp on the target.
 * \return The exit code of the application, non-zero if a problem occurred.
 */
int main(void)
{
    /* Name of the bitmap to load and run the FIR filter on. */
    string filename = "assets/input.bmp";

    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    const unsigned int numberOfMemoryObjects = 2;
    cl_mem memoryObjects[numberOfMemoryObjects] = {0, 0};
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

    if (!createProgram(context, device, "assets/fir_float.cl", &program))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL program." << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    kernel = clCreateKernel(program, "fir_float", &errorNumber);
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

    /* Convert 24-bits per pixel RGB into 8-bits per pixel luminance data. */
    unsigned char* inputLuminance = new unsigned char [width * height];
    RGBToLuminance(loadedRGBData, inputLuminance, width, height);
    delete [] loadedRGBData;

    /* All buffers are the size of the image data. */
    size_t bufferSize = width * height * sizeof(float);

    /* Create one input buffer for the original image data, and one output buffer for the output image data. */
    bool createMemoryObjectsSuccess = true;
    memoryObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSize, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);
    memoryObjects[1] = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSize, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);
    if (!createMemoryObjectsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL buffers. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* Map the input memory object to a host side pointer. */
    cl_float* inputImageData = (cl_float*)clEnqueueMapBuffer(commandQueue, memoryObjects[0], CL_TRUE, CL_MAP_WRITE, 0, bufferSize, 0, NULL, NULL, &errorNumber);
    if (!checkSuccess(errorNumber))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Mapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Converting luminance data into float. A real world application would use real floating point data.*/
    for(int i = 0; i < width * height; i++)
    {
        inputImageData[i] = (float)inputLuminance[i] / 255.0f;
    }

    delete [] inputLuminance;

    /* Unmap the memory so we can pass it to the kernel. */
    if (!checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[0], inputImageData, 0, NULL, NULL)))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Setup the kernel arguments.  */
    bool setKernelArgumentsSuccess = true;
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 0, sizeof(cl_mem), &memoryObjects[0]));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 1, sizeof(cl_mem), &memoryObjects[1]));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 2, sizeof(cl_int), &width));
    if (!setKernelArgumentsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed setting OpenCL kernel arguments. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* An event to associate with the kernel. Allows us to retrieve profiling information later. */
    cl_event event = 0;

    /* [Kernel size] */
    /*
     * Each instance of the kernel operates on a 4 * 1 portion of the image.
     * Therefore, the global work size must be width / 4 by height / 1 work items.
     */
    size_t globalWorksize[2] = {width / 4, height / 1};
    /* [Kernel size] */

    /* Enqueue the kernel */
    if (!checkSuccess(clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorksize, NULL, 0, NULL, &event)))
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

    /* Map the output memory to a host side pointer. */
    cl_float* output = (cl_float*)clEnqueueMapBuffer(commandQueue, memoryObjects[1], CL_TRUE, CL_MAP_READ, 0, bufferSize, 0, NULL, NULL, &errorNumber);
    if (!checkSuccess(errorNumber))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Mapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Convert the float output to unsigned char for saving to bitmap. */
    unsigned char *outputData= new unsigned char[width * height];
    for(int i = 0; i< width * height; i++)
    {
        outputData[i] = (unsigned char)(output[i] * 255.0f);
    }

    /* Unmap the output. */
    if (!checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[1], output, 0, NULL, NULL)))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Release OpenCL objects. */
    cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);

    /* Convert the output luminance array to RGB and save it out to a file. */
    unsigned char *rgbOut = new unsigned char[width * height * 3];
    luminanceToRGB(outputData, rgbOut, width, height);
    delete [] outputData;

    saveToBitmap("output.bmp", width, height, rgbOut);
    delete [] rgbOut;

    return 0;
}
