#include <iostream>
#include <time.h>
#include <chrono>
#include <cstdlib>
#include <stack>
#include <mpi.h>
using namespace std;
using namespace std::chrono;

long SIZE = 1000000;
int num_processes, ranks;
int *data, *stacks, *combine;
#define HEAD 0

void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

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

void initializeArray(int *arr)
{
    for (int i = 0; i < SIZE; i++)
    {
        arr[i] = rand() % 100;
    }
}

void quicksort(int *array, int *stacks, int piece)
{
    if (piece <= 1)
    {
        return;
    }

    int min = 0;
    int high = piece - 1;
    int max = -1;

    stacks[++max] = min;
    stacks[++max] = high;

    while (max >= 0)
    {
        high = stacks[max--];
        min = stacks[max--];

        int pivot = array[high];
        int index = min;

        for (int i = min; i < high; i++)
        {
            if (array[i] <= pivot)
            {
                swap(&array[i], &array[index]);
                index++;
            }
        }
        swap(&array[index], &array[high]);
        if (index + 1 < high)
        {
            stacks[++max] = index + 1;
            stacks[++max] = high;
        }
        if (index - 1 > min)
        {
            stacks[++max] = min;
            stacks[++max] = index - 1;
        }
    }
}

int *merge(int *chunk, int chunk_Size, int *combine, int combine_Size)
{
    int *result = (int *)malloc((chunk_Size + combine_Size) * sizeof(int));
    int i = 0;
    int j = 0;
    int k;
    for (k = 0; k < chunk_Size + combine_Size; k++)
    {
        if (i >= chunk_Size)
        {
            result[k] = combine[j];
            j++;
        }
        else if (j >= combine_Size)
        {
            result[k] = chunk[i];
            i++;
        }
        else if (chunk[i] < combine[j])
        {
            result[k] = chunk[i];
            i++;
        }
        else
        {
            result[k] = combine[j];
            j++;
        }
    }
    return result;
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        SIZE = atoi(argv[1]);
    }

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &ranks);
    MPI_Status status;

    if (ranks == HEAD)
    {
        data = (int *)malloc(SIZE * sizeof(int));
        stacks = (int *)malloc(SIZE * sizeof(int));
        initializeArray(data);
    }

    auto start = high_resolution_clock::now();

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&SIZE, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int piece = SIZE / num_processes;
    int *chunk = (int *)malloc(piece * sizeof(int));
    int *chunk_stack = (int *)malloc(piece * sizeof(int));

    MPI_Scatter(data, piece, MPI_INT, chunk, piece, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(stacks, piece, MPI_INT, chunk_stack, piece, MPI_INT, 0, MPI_COMM_WORLD);

    int local_chunk_size = piece;

    quicksort(chunk, chunk_stack, piece);

    for (int step = 1; step < num_processes; step = 2 * step)
    {
        if (ranks % (2 * step) != 0)
        {
            MPI_Send(chunk, local_chunk_size, MPI_INT, ranks - step, 0, MPI_COMM_WORLD);
            break;
        }
        if (ranks + step < num_processes)
        {
            int merging_size;

            if (SIZE >= piece * (ranks + 2 * step))
            {
                merging_size = piece * step;
            }
            else
            {
                merging_size = SIZE - piece * (ranks + step);
            }

            combine = (int *)malloc(merging_size * sizeof(int));
            MPI_Recv(combine, merging_size, MPI_INT, ranks + step, 0, MPI_COMM_WORLD, &status);
            data = merge(chunk, local_chunk_size, combine, merging_size);
            chunk = data;
            local_chunk_size = local_chunk_size + merging_size;
        }
    }

    if (ranks == HEAD)
    {
        print_array(chunk, SIZE);
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        cout << "Time taken to quicksort: " << duration.count() << " milliseconds" << endl;
    }
    MPI_Finalize();
    return 0;
}
// mpicxx ./quicksort_mpi.cpp -o mpi
// mpirun -np 4 -hostfile ./cluster ./mpi