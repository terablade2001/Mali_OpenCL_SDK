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

using namespace std;

/**
 * \brief Basic integer array addition implemented in OpenCL.
 * \details A sample which shows how to add two integer arrays and store the result in a third array.
 *          The main calculation code is in an OpenCL kernel which is executed on a GPU device.
 * \return The exit code of the application, non-zero if a problem occurred.
 */
int main(void)
{
    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    int numberOfMemoryObjects = 3;
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

    if (!createProgram(context, device, "assets/hello_world_opencl.cl", &program))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL program." << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    kernel = clCreateKernel(program, "hello_world_opencl", &errorNumber);
    if (!checkSuccess(errorNumber))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL kernel. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }

    /* [Setup memory] */
    /* Number of elements in the arrays of input and output data. */
    cl_int arraySize = 1000000;

    /* The buffers are the size of the arrays. */
    size_t bufferSize = arraySize * sizeof(cl_int);

    /*
     * Ask the OpenCL implementation to allocate buffers for the data.
     * We ask the OpenCL implemenation to allocate memory rather than allocating
     * it on the CPU to avoid having to copy the data later.
     * The read/write flags relate to accesses to the memory from within the kernel.
     */
    bool createMemoryObjectsSuccess = true;

    memoryObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSize, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);

    memoryObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSize, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);

    memoryObjects[2] = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, bufferSize, NULL, &errorNumber);
    createMemoryObjectsSuccess &= checkSuccess(errorNumber);

    if (!createMemoryObjectsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed to create OpenCL buffer. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Setup memory] */

    /* [Map the buffers to pointers] */
    /* Map the memory buffers created by the OpenCL implementation to pointers so we can access them on the CPU. */
    bool mapMemoryObjectsSuccess = true;

    cl_int* inputA = (cl_int*)clEnqueueMapBuffer(commandQueue, memoryObjects[0], CL_TRUE, CL_MAP_WRITE, 0, bufferSize, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);

    cl_int* inputB = (cl_int*)clEnqueueMapBuffer(commandQueue, memoryObjects[1], CL_TRUE, CL_MAP_WRITE, 0, bufferSize, 0, NULL, NULL, &errorNumber);
    mapMemoryObjectsSuccess &= checkSuccess(errorNumber);

    if (!mapMemoryObjectsSuccess)
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Failed to map buffer. " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }
    /* [Map the buffers to pointers] */

    /* [Initialize the input data] */
    for (int i = 0; i < arraySize; i++)
    {
       inputA[i] = i;
       inputB[i] = i;
    }
    /* [Initialize the input data] */

    /* [Un-map the buffers] */
    /*
     * Unmap the memory objects as we have finished using them from the CPU side.
     * We unmap the memory because otherwise:
     * - reads and writes to that memory from inside a kernel on the OpenCL side are undefined.
     * - the OpenCL implementation cannot free the memory when it is finished.
     */
    if (!checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[0], inputA, 0, NULL, NULL)))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    if (!checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[1], inputB, 0, NULL, NULL)))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }
    /* [Un-map the buffers] */

    /* [Set the kernel arguments] */
    bool setKernelArgumentsSuccess = true;
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 0, sizeof(cl_mem), &memoryObjects[0]));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 1, sizeof(cl_mem), &memoryObjects[1]));
    setKernelArgumentsSuccess &= checkSuccess(clSetKernelArg(kernel, 2, sizeof(cl_mem), &memoryObjects[2]));

    if (!setKernelArgumentsSuccess)
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed setting OpenCL kernel arguments. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Set the kernel arguments] */

    /* An event to associate with the Kernel. Allows us to retrieve profiling information later. */
    cl_event event = 0;

    /* [Global work size] */
    /*
     * Each instance of our OpenCL kernel operates on a single element of each array so the number of
     * instances needed is the number of elements in the array.
     */
    size_t globalWorksize[1] = {arraySize};
    /* Enqueue the kernel */
    if (!checkSuccess(clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, globalWorksize, NULL, 0, NULL, &event)))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
        cerr << "Failed enqueuing the kernel. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Global work size] */

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

    /* Get a pointer to the output data. */
    cl_int* output = (cl_int*)clEnqueueMapBuffer(commandQueue, memoryObjects[2], CL_TRUE, CL_MAP_READ, 0, bufferSize, 0, NULL, NULL, &errorNumber);
    if (!checkSuccess(errorNumber))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Failed to map buffer. " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* [Output the results] */
    /* Uncomment the following block to print results. */
    /*
    for (int i = 0; i < arraySize; i++)
    {
        cout << "i = " << i << ", output = " <<  output[i] << "\n";
    }
    */
    /* [Output the results] */

    /* Unmap the memory object as we are finished using them from the CPU side. */
    if (!checkSuccess(clEnqueueUnmapMemObject(commandQueue, memoryObjects[2], output, 0, NULL, NULL)))
    {
       cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
       cerr << "Unmapping memory objects failed " << __FILE__ << ":"<< __LINE__ << endl;
       return 1;
    }

    /* Release OpenCL objects. */
    cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numberOfMemoryObjects);
}
