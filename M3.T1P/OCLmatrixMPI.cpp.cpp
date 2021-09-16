#include <iostream>
#include <time.h>
#include <chrono>
#include <cstdlib>
#include <mpi.h>
#include <CL/cl.h>

#define CL_TARGET_OPENCL_VERSION 220
#define PRINT 1

using namespace std::chrono;
using namespace std;

int SIZE = 500;
int numtasks, rank, name_len, world_size = 2;

int *v;
int *w;
int *a;
cl_mem buff1;
cl_mem buff2;
cl_mem buff3;
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
void init(int *&A, int size);

inline int conv_1d(int column, int row, int row_len);
int init_mpi(int argc, char **argv);
void log_mat(int *mat, int i, int j);

int *matricesOne = (int *)malloc(SIZE * SIZE * sizeof(int *));
int *matricesTwo = (int *)malloc(SIZE * SIZE * sizeof(int *));
int *resultant = (int *)malloc(SIZE * SIZE * sizeof(int *));

int mpi_init(int argc, char** argv)
{
    int rank = 0;
    char name[MPI_MAX_PROCESSOR_NAME];
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(name, &name_len);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    return rank;
}

inline int convert_to_array(int column, int row, int row_len)
{
    return row * row_len + column;
}

void setMatrix(int *matricy, long long len)
{
    for (int i = 0; i < len; i++)
    {
        matricy[i] += rand() % 100;
    }
}

void print(int **matricy)
{
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            printf("%d", matricy[i][j]);
        }
        cout << endl;
    }
}

int main(int argc, char** argv)
{   
    auto start = high_resolution_clock::now();
    int rank = mpi_init(argc, argv);
    
    setMatrix(matricesOne, SIZE * SIZE);
    setMatrix(matricesTwo, SIZE * SIZE);
    
    // number of elements per process
    int npp = SIZE / world_size;

    // matricyTwo segment multiplied by matricyOne
    int *matricyTwo_segment = (int *)malloc(SIZE * npp * sizeof(int *));

    // holds result
    int *node_result = (int *)malloc(SIZE * npp * sizeof(int *));

    // Distributing data
    MPI_Datatype seg;
    MPI_Datatype coll;
    MPI_Bcast(matricesOne, SIZE * SIZE, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Type_vector(SIZE, npp, SIZE, MPI_INT, &seg);
    MPI_Type_create_resized(seg, 0, sizeof(int), &seg);
    MPI_Type_commit(&seg);
    MPI_Type_vector(npp, npp, npp, MPI_INT, &coll);
    MPI_Type_commit(&coll);

    // Scatter Data
    int *m1 = (int *)malloc(world_size * sizeof(int));
    int *m2 = (int *)malloc(world_size * sizeof(int));

    for (int i = 0; i < world_size; i++)
    {
        m2[i] = 1;
        m1[i] = i * npp;
    }

    MPI_Scatterv(matricesTwo, m2, m1, seg, matricyTwo_segment, SIZE, coll, 0, MPI_COMM_WORLD);

    // Open CL Matrix multiplication
    size_t global[2] = {(size_t)SIZE, (size_t)SIZE};
    setup_openCL_device_context_queue_kernel((char *)"./execute.cl", (char *)"execute");
    setup_kernel_memory();
    copy_kernel_args();
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clEnqueueReadBuffer(queue, buff3, CL_TRUE, 0, SIZE * SIZE * sizeof(int), &a[0], 0, NULL, NULL);

    MPI_Datatype send;
    MPI_Type_vector(SIZE, npp, npp, MPI_INT, &send);
    MPI_Type_commit(&send);

    MPI_Datatype recv;
    MPI_Type_vector(SIZE, npp, SIZE, MPI_INT, &recv);
    MPI_Type_create_resized(recv, 0, sizeof(int), &recv);
    MPI_Type_commit(&recv);

    int *m1_recv = (int *)malloc(world_size * sizeof(int));
    int *m2_recv = (int *)malloc(world_size * sizeof(int));

    for (int i = 0; i < world_size; i++)
    {
        m2_recv[i] = 1;
        m1_recv[i] = i * npp;
    }

    MPI_Gatherv(node_result, 1, send, resultant, m2_recv, m1_recv, recv, 0, MPI_COMM_WORLD);
    MPI_Finalize();

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    if (rank == 0)
    {
        printf("%lu\n", duration.count());
    }
    return 0;
}

void free_memory()
{
    clReleaseMemObject(buff1);
    clReleaseMemObject(buff2);
    clReleaseMemObject(buff3);

    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    free(v);
    free(w);
    free(a);
}

void copy_kernel_args()
{
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&SIZE);
    clSetKernelArg(kernel, 1, sizeof(int), (void *)&SIZE);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&buff1);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&buff2);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&buff3);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory()
{
    buff1 =
        clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * SIZE * sizeof(int), NULL, NULL);
    buff2 =
        clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * SIZE * sizeof(int), NULL, NULL);
    buff3 =
        clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * SIZE * sizeof(int), NULL, NULL);

    clEnqueueWriteBuffer(queue, buff1, CL_TRUE, 0, SIZE * SIZE * sizeof(int), &v[0], 0,
                         NULL, NULL);
    clEnqueueWriteBuffer(queue, buff2, CL_TRUE, 0, SIZE * SIZE * sizeof(int), &w[0], 0,
                         NULL, NULL);
    clEnqueueWriteBuffer(queue, buff3, CL_TRUE, 0, SIZE * SIZE * sizeof(int), &a[0], 0,
                         NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename,
                                              char *kernelname)
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

    program = clCreateProgramWithSource(ctx, 1, (const char **)&program_buffer,
                                        &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {

        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 0, NULL,
                              &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, log_size + 1,
                              program_log, NULL);
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

    /* Identify a platform */
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0)
    {
        perror("Couldn't identify a platform");
        exit(1);
    }

    // Access a device
    // GPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
    if (err == CL_DEVICE_NOT_FOUND)
    {
        // CPU
        printf("GPU not found\n");
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    }
    if (err < 0)
    {
        perror("Couldn't access any devices");
        exit(1);
    }

    return dev;
}

// mpicxx ./OCLmatrixMPI.cpp -o mpiOCL.o -l OpenCL
// mpirun -np 4 -hostfile ./cluster ./mpiOCL.o