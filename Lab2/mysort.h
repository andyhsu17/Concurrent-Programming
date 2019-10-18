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
void bucket_sort(int start, int n);
void *bucket_sort(void* arg);
void handle_concurrency_primitive(bool position);
void handle_concurrency_primitive1();