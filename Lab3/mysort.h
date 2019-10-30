#include <iostream>
#include <vector>

// includes
#define STDARRAYSIZE 10000
// structs
typedef struct 
{
    int left;
    int right;
    int *arr;
}sortData_t;


// function declarations
void global_init();
void global_cleanup();
void taskHandler(int arr[], int left, int right);
void merge(int arr[], int left, int middle, int right);
void mergeSort(int arr[], int left, int right);
