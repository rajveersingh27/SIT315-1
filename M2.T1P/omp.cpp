#include <iostream>
#include <time.h>
#include <chrono>
#include <cstdlib>
#include <omp.h>
#include <fstream>

using namespace std::chrono;
using namespace std;

const int SIZE = 300;
const int THREADS = 5;
int **matricesOne, **matricesTwo, **resultant;

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

void MultiplyMatrix(int **matricesOne, int **matricesTwo, int **matricyThree)
{
#pragma omp for
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            matricyThree[i][j] = 0;
            for (int k = 0; k < SIZE; k++)
            {
                matricyThree[i][j] += (matricesOne[i][k] * matricesTwo[k][j]);
            }
        }
    }
}

int main()
{
    omp_set_num_threads(THREADS);
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

    auto start = high_resolution_clock::now();
#pragma omp parallel num_threads(THREADS) default(none) shared(matricesOne, matricesTwo, resultant)
    {
        MultiplyMatrix(matricesOne, matricesTwo, resultant);
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    printf("Time in milliseconds %lld\n", duration.count());
    return 0;
}
//g++ -std=c++17 -Xpreprocessor -fopenmp omp.cpp -o omp -lomp
//./omp.cpp