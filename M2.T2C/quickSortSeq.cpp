#include <iostream>
#include <time.h>
#include <chrono>
#include <cstdlib>
using namespace std;
using namespace std::chrono;

const int SIZE = 1000000;

void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

void printArray(int *arr)
{
    for (int i = 0; i < SIZE; i++)
    {
        printf("%d, ", arr[i]);
    }
    cout << endl;
}

void initializeArray(int *arr)
{
    for (int i = 0; i < SIZE; i++)
    {
        arr[i] = (rand() % SIZE) + 1;
    }
}

int partition(int arr[], int low, int high)
{
    int pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++)
    {
        if (arr[j] <= pivot)
        {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void quickSort(int arr[], int low, int high)
{
    if (low < high)
    {
        int pivot = partition(arr, low, high);
        quickSort(arr, low, pivot - 1);
        quickSort(arr, pivot + 1, high);
    }
}

int main()
{
    int arr[SIZE];
    initializeArray(arr);
    //printArray(arr);
    auto start = high_resolution_clock::now();
    quickSort(arr, 0, SIZE - 1);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    //printArray(arr);
    printf("Time in milliseconds %lld\n", duration.count());
    // printArray(arr);
    return 0;
}

// g++ -std=c++17 -g quickSortSeq.cpp -o seq
// ./seq