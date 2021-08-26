#include <iostream>
#include <time.h>
#include <chrono>
#include <cstdlib>

using namespace std::chrono;
using namespace std;

const int SIZE = 300;
const int NUM_THREADS = 4;

int **matricesOne = new int *[SIZE];
int **matricesTwo = new int *[SIZE];
int **resultant = new int *[SIZE];

int randThread1 = 0;
int randThread2 = 0;
int multiThread = 0;

void manualAllocateMatricy()
{
    matricesOne = (int **)malloc(SIZE * sizeof(int *));
    for (int i = 0; i < SIZE; i++)
    {
        matricesOne[i] = (int *)malloc(SIZE * sizeof(int));
    }
    matricesTwo = (int **)malloc(SIZE * sizeof(int *));
    for (int i = 0; i < SIZE; i++)
    {
        matricesTwo[i] = (int *)malloc(SIZE * sizeof(int));
    }
    resultant = (int **)malloc(SIZE * sizeof(int *));
    for (int i = 0; i < SIZE; i++)
    {
        resultant[i] = (int *)malloc(SIZE * sizeof(int));
    }
}

void setMatrix(int **matricy, int maxNum)
{
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            matricy[i][j] = 0;
            for (int k = 0; k < SIZE; k++)
            {
                matricy[i][j] += rand() % maxNum;
            }
        }
    }
}

void *MultiplyMatrix(void *arg)
{
    int thread = multiThread++;
    int partition = SIZE / NUM_THREADS;

    for (int i = thread * partition; i < (thread + 1) * partition; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            int sum = 0;
            for (int k = 0; k < SIZE; k++)
            {
                sum = sum + (matricesOne[i][k] * matricesTwo[k][j]);
            }
            resultant[i][j] = sum;
        }
    }

    return NULL;
}

int main()
{
    manualAllocateMatricy();
    setMatrix(matricesOne, 2);
    setMatrix(matricesTwo, 2);
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            for (int k = 0; k < SIZE; k++)
            {
                resultant[i][j] += 0;
            }
        }
    }

    pthread_t threads[NUM_THREADS];
    auto start = high_resolution_clock::now();
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, MultiplyMatrix, NULL);
    }
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    printf("Time in milliseconds %lld\n", duration.count());
    return 0;
}

//g++ -std=c++17 -g pthread.cpp -o pthread
//./pthread