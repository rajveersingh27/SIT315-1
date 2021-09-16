#include <iostream>
#include <time.h>
#include <chrono>
#include <cstdlib>
#include <mpi.h>

using namespace std::chrono;
using namespace std;

int SIZE = 500;
int numtasks, rank, name_len, world_size = 2;

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

    // Matrix multiplication
    for (int h = 0; h < npp; h++)
    {
        for (int i = 0; i < SIZE; i++)
        {
            node_result[convert_to_array(h, i, npp)] = 0;
            for (int j = 0; j < SIZE; j++)
            {
                node_result[convert_to_array(h, i, npp)] += matricesOne[convert_to_array(j, i, SIZE)] * matricyTwo_segment[convert_to_array(h, j, npp)];
            }
        }
    }

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

// mpicxx ./matrixMPI.cpp -o mpi.o
// mpirun -np 4 -hostfile ./cluster ./mpi.o