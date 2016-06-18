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
 * \brief Simple template OpenCL sample.
 * \details The basic code to run a kernel with no arguments.
 * \return The exit code of the application, non-zero if a problem occurred.
 */
int main(void)
{
    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    const int numMemoryObjects = 1;
    cl_mem memoryObjects[numMemoryObjects] = {0};
    cl_int errorNumber;


    /* Set up OpenCL environment: create context, command queue, program and kernel. */

    /* [Create Context] */
    if (!createContext(&context))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numMemoryObjects);
        cerr << "Failed to create an OpenCL context. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Create Context] */

    /* [Create Command Queue] */
    if (!createCommandQueue(context, &commandQueue, &device))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numMemoryObjects);
        cerr << "Failed to create the OpenCL command queue. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Create Command Queue] */

    /* [Create Program] */
    if (!createProgram(context, device, "assets/template.cl", &program))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numMemoryObjects);
        cerr << "Failed to create OpenCL program." << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Create Program] */

    /* [Create kernel] */
    kernel = clCreateKernel(program, "template", &errorNumber);
    if (!checkSuccess(errorNumber))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numMemoryObjects);
        cerr << "Failed to create OpenCL kernel. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Create kernel] */


    /*
     * Add code here to set up memory/data, for example:
     * - Create memory buffers.
     * - Initialise the input data.
     * - Set up kernel arguments.
     */


    /* Execute the kernel instances. */

    /* [Set the kernel work size] */
    const int workDimensions = 1;
    size_t globalWorkSize[workDimensions] = {1};
    /* [Set the kernel work size] */

    /* [Enqueue the kernel] */
    /* An event to associate with the kernel. Allows us to retrieve profiling information later. */
    cl_event event = 0;

    if (!checkSuccess(clEnqueueNDRangeKernel(commandQueue, kernel, workDimensions, NULL, globalWorkSize, NULL, 0, NULL, &event)))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numMemoryObjects);
        cerr << "Failed enqueuing the kernel. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Enqueue the kernel] */

    /* [Wait for kernel execution completion] */
    if (!checkSuccess(clFinish(commandQueue)))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numMemoryObjects);
        cerr << "Failed waiting for kernel execution to finish. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Wait for kernel execution completion] */


    /* After execution. */

    /* [Print the profiling information for the event] */
    printProfilingInfo(event);
    /* Release the event object. */
    if (!checkSuccess(clReleaseEvent(event)))
    {
        cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numMemoryObjects);
        cerr << "Failed releasing the event object. " << __FILE__ << ":"<< __LINE__ << endl;
        return 1;
    }
    /* [Print the profiling information for the event] */


    /* Add code here to retrieve results of the kernel execution. */


    /* [Release OpenCL objects] */
    cleanUpOpenCL(context, commandQueue, program, kernel, memoryObjects, numMemoryObjects);
    /* [Release OpenCL objects] */

    return 0;
}
