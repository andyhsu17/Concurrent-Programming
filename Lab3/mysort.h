#include <iostream>
#include <vector>

// includes
#define STDARRAYSIZE 10000
// structs



// function declarations
void global_init();
void global_cleanup();
void taskHandler(int arr[], int left, int right);
void merge(int arr[], int left, int middle, int right);
void mergeSort(int arr[], int left, int right);
