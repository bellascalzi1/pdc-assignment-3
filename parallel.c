#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CL_TARGET_OPENCL_VERSION 200

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MAX_SOURCE_SIZE (0x100000)

void check_err(cl_int err, char* err_output) {
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error %i: %s\n", (int)err, err_output);
        exit(1);
    }
}

int *generateRandomArray(size_t size) {

    int *arr = (int *)malloc(sizeof(int) * size);

    for(size_t i=0; i<size; i++) {
        // Make verifying results easier for me :( max array size is probs gonna be 1mil
        arr[i] = rand() % 1000000;
    }

    return arr;

}

void printArray(int *arr, size_t size) {
    printf("Printing Array...\n");
    for(size_t i=0; i<size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void checkResult(int *arr, size_t size) {

    for(size_t i=0; i<size-1; i++) {
        if(arr[i] > arr[i+1]) {
            printf("ERRORORORORORORO %d is greater than %d\n", arr[i], arr[i+1]);
        }
    }
}

int main(int argc, char *argv[]) {

    if(argc != 3) {
        printf("Give me the array size and no work items, you idiot!\n");
        exit(EXIT_FAILURE);
    }

    const size_t arraySize = (size_t) atoi(argv[1]);
    const size_t noWorkItemsPerWorkgroup = (size_t) atoi(argv[2]);

    // Seed the RNG with the current time
    srand(time(NULL));

    int *arrPtr = generateRandomArray(arraySize);

    // printf("Array before sort is:\n");
    // printArray(arrPtr, arraySize);

    // Load the kernel source code into the array source_str
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("kernel.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    // variable used for OpenCL error handling
    cl_int err;

    // Get the platform ID
    cl_platform_id platform_id;
    cl_uint ret_num_platforms;
    err = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    check_err(err, "clGetPlatformIDs");
    // printf("%i Platforms found\n", ret_num_platforms);

    // Get the first GPU device associated with the platform
    cl_device_id device_id;
    cl_uint ret_num_devices;
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
    check_err(err, "clGetPlatformIDs");
    // printf("%i Devices found\n", ret_num_devices);

    // Get maximum work group size
    size_t max_work_group_size;
    err = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);
    check_err(err, "clGetDeviceInfo");
    // printf("Max workgroup size is %i\n", (int)max_work_group_size);

    // Create an OpenCL context
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    check_err(err, "clCreateContext");

    // Create Queue for laptop
    cl_command_queue_properties props[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_id, props, &err);

    // Create queue for cluster
    // cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
    check_err(err, "clCreateCommandQueue");

    // Create memory buffer on devide for array to be sorted
    cl_mem arrayDevice = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, arraySize * sizeof(int), arrPtr, &err);
    check_err(err, "clCreateBuffer arrayDevice");

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &err);
    check_err(err, "clCreateProgramWithSource");

    // Build the program
    err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    check_err(err, "clBuildProgram");

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "oddEvenSort", &err);
    check_err(err, "clCreateKernel");

    // Set the arguments of the kernel
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&arrayDevice);
    check_err(err, "clSetKernelArg arrayDevice");
    err = clSetKernelArg(kernel, 1, sizeof(int), (void *)&arraySize);
    check_err(err, "clSetKernelArg n");

    // Since each work item is doing both an odd step and an even step, the total number of work items needed is arraySize / 2
    size_t noWorkItemsNeeded = (arraySize) / 2;

    // Launch Kernel linked to an event
    size_t local_work_size = noWorkItemsPerWorkgroup;
    size_t num_work_groups = (noWorkItemsNeeded + local_work_size - 1) / local_work_size; // divide rounding up

    size_t global_work_size = num_work_groups * local_work_size;

    // printf("Array Size: %ld\nNumber Work Items per Workgroup: %ld\nMax work group size: %ld\nNumber work groups: %ld\nGlobal work size: %ld\n", arraySize, noWorkItemsPerWorkgroup, local_work_size, num_work_groups, global_work_size);

    cl_event event;
    cl_ulong time_start;
    cl_ulong time_end;
    double total_time = 0.0;

    // Array is guaranteed to be sorted in no more than arraySize steps.
    for(size_t i=0; i<arraySize; i++) {
        // We are enqueuing multiple kernels here as a way of synchronising between workgroups. If we didn't do this, the sort wouldn't work.
        err = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &noWorkItemsNeeded, &local_work_size, 0, NULL, &event);
        check_err(err, "clEnqueueNDRangeKernel");

        // Idea for reusing one event for timing: https://stackoverflow.com/questions/23550912/measuring-execution-time-of-opencl-kernels
        err = clWaitForEvents(1, &event);
        check_err(err, "clWaitForEvents");

        err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
        check_err(err, "clGetEventProfilingInfo start_time");

        err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
        check_err(err, "clGetEventProfilingInfo end_time");

        total_time += (time_end - time_start) / 1000000.0;
    }

    // Print final time
    printf("%0.3f ", total_time);

    // Wait for all the commands on the queue to finish
    clFinish(command_queue);
    
    // Get the result from the device
    int *resultArray = (int*)malloc(sizeof(int)*arraySize);
    err = clEnqueueReadBuffer(command_queue, arrayDevice, CL_TRUE, 0, arraySize * sizeof(int), resultArray, 0, NULL, NULL);
    check_err(err, "clEnqueueReadBuffer c");

    // Display the result to the screen
    // printArray(resultArray, arraySize);

    // Check that array is actually sorted
    checkResult(resultArray, arraySize);

    // Clean up
    err = clFlush(command_queue);
    err = clFinish(command_queue);
    err = clReleaseKernel(kernel);
    err = clReleaseProgram(program);
    err = clReleaseMemObject(arrayDevice);
    err = clReleaseCommandQueue(command_queue);
    err = clReleaseContext(context);
    free(arrPtr);

    return EXIT_SUCCESS;
}