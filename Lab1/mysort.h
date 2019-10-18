#include <iostream>
#include <vector>

// includes
#define STDARRAYSIZE 50
// structs
typedef struct 
{
    int left;
    int right;
    int *arr;
}sortData_t;

typedef struct
{
    std::vector <int> arr;

}bucketData_t;
// function declarations
void global_init();
void global_cleanup();
void merge(int arr[], int left, int middle, int right);
void mergeSort(std::vector <int> arr, int left, int right);
void bucketSort(int array[], int k);
void bucketSort(int array[], int n);