#include <stdio.h>
#include <time.h>
#include <iostream>
#include <stack>
#include <chrono>
#include <CL/cl.h>

using namespace std;
using namespace std::chrono;

#define PRINT 1

long SIZE = 1000;
int *input;
int *input_stack;

cl_mem buff_data;
cl_mem buff_stack;
cl_device_id device_id;
cl_context context;
cl_program program;
cl_kernel kernel;
cl_command_queue queue;
cl_event event = NULL;
int err;

cl_device_id create_device();
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
void setup_kernel_memory();
void copy_kernel_args();
void free_memory();

void print_array(int *arr, int size)
{
    for (int i = 0; i < 5; i++)
    {
        printf("%d  ", arr[i]);
    }

    printf("...");

    for (int i = size - 5; i < size; i++)
    {
        printf(" %d  ", arr[i]);
    }
    printf("\n");
}

void initializeArray(int *&arr, int *&stack, int size)
{
    arr = (int *)malloc(sizeof(int) * size);
    stack = (int *)malloc(sizeof(int) * size);

    for (int i = 0; i < size; i++)
    {
        arr[i] = rand() % 100;
    }
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        SIZE = atoi(argv[1]);
    }

    initializeArray(input, input_stack, SIZE);
    size_t gl[] = {(size_t)SIZE + 1};

    auto start = high_resolution_clock::now();

    setup_openCL_device_context_queue_kernel((char *)"./quick.cl", (char *)"quicksort");
    setup_kernel_memory();
    copy_kernel_args();
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, gl, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clEnqueueReadBuffer(queue, buff_data, CL_TRUE, 0, SIZE * sizeof(int), &input[0], 0, NULL, NULL);
    free_memory();

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
}

void free_memory()
{
    clReleaseMemObject(buff_data);
    clReleaseMemObject(buff_stack);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);
    free(input);
    free(input_stack);
}

void copy_kernel_args()
{
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buff_data);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&buff_stack);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory()
{
    buff_data = clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * sizeof(int), NULL, NULL);
    buff_stack = clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * sizeof(int), NULL, NULL);
    clEnqueueWriteBuffer(queue, buff_data, CL_TRUE, 0, SIZE * sizeof(int), &input[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, buff_stack, CL_TRUE, 0, SIZE * sizeof(int), &input_stack[0], 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
    device_id = create_device();
    cl_int err;
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);

    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);

    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };

    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)
{
    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    program_handle = fopen(filename, "r");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    program = clCreateProgramWithSource(ctx, 1, (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

cl_device_id create_device()
{

    cl_platform_id platform;
    cl_device_id dev;
    int err;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0)
    {
        perror("Couldn't identify a platform");
        exit(1);
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
    if (err == CL_DEVICE_NOT_FOUND)
    {
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    }
    if (err < 0)
    {
        perror("Couldn't access any devices");
        exit(1);
    }

    return dev;
}

// g++ -std=c++11 ./quicksort_ocl.cpp -o ocl.o -l OpenCL
// ./ocl.o

// g++ -std=c++11 ./quicksort_ocl.cpp -o ocl.o -l OpenCL
// ./ocl.o