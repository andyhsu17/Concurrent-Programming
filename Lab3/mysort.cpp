#include <iostream>
#include <fstream>
#include <time.h>
#include <omp.h>
#include <string.h>
#include "mysort.h"

struct timespec start, end;
int numarray[STDARRAYSIZE];
sortData_t * dataStruct;
volatile int arrayElements;

void merge(int arr[], int left, int middle, int right)
{
    int i, j, k;                        // initializations
    int size1 = middle - left + 1; 
    int size2 =  right - middle; 
  
    int Leftarray[size1], Rightarray[size2]; 
  
    for (i = 0; i < size1; i++)                 // copy array into 2 temporary arrays 
        Leftarray[i] = arr[left + i]; 
    for (j = 0; j < size2; j++) 
        Rightarray[j] = arr[middle + 1 + j]; 
  
    i = 0;      // index of first array
    j = 0; // index of second array  
    k = left;  // index of merged array

    while (i < size1 && j < size2) 
    { 
        if (Leftarray[i] <= Rightarray[j])          // if left is less than right, store the left one first
        { 
            arr[k] = Leftarray[i]; 
            i++; 
        } 
        else
        { 
            arr[k] = Rightarray[j];                 // otherwise put the right first
            j++; 
        } 
        k++; 
    }
    while (i < size1) 
    { 
        arr[k] = Leftarray[i];                  // merge whatever is left
        i++; 
        k++; 
    } 
    while (j < size2) 
    { 
        arr[k] = Rightarray[j];                 // put everything remaining
        j++; 
        k++; 
    } 
}

void mergeSort(int arr[], int left, int right)
{
        if (left < right) 
        { 
            int middle = (left + right) / 2;      // middle index of initial array 
            mergeSort(arr, left, middle);           // sort left first
            mergeSort(arr, middle + 1, right);      // sort right
            merge(arr, left, middle, right);        //merge the two
        }    
}
void taskHandler(int arr[], int left, int right)
{
    if(!left)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
    }
    mergeSort(numarray, left, right);
    if(!left)
    {
        clock_gettime(CLOCK_MONOTONIC, &end);
    }
}

int main(int argc, char * argv[])
{
    int ret;
    int i = 0;
    int j = 0;
    int c;
    int temp;
    int left, right;
    int NUMTHREADS;
    std::fstream myfile(argv[1]);       // file input numbers into int array
    std::vector <std::vector<int>> threadData; 
    std::string output = "out.txt";

    if(!strcmp(argv[1], "--name"))
    {
        std::cout << "Andrew Hsu" << std::endl;
        return 0;
    }

    std::string sourcefile = argv[1];

    for(int l = 0; l < argc; l++)
    {
        if(!strcmp(argv[l], "-o"))
        {
            output = argv[l + 1]; 
        }
        
    }

    if(myfile.is_open())
    {
        while(myfile >> c)
        {
            numarray[i++] = c;
        }
    }
    myfile.close();
    arrayElements = i;

    #pragma omp parallel        // just allows us to get the number of threads. otherwise this will always be 1
    {
        NUMTHREADS = omp_get_num_threads();    
    }
    temp = arrayElements / NUMTHREADS;    // number of elements in each thread

    for(int i = 0; i < NUMTHREADS; i++)     // load left and right array values inside vector of vectors
    {
        left = j;
        right = j + temp - 1;
        if(i == NUMTHREADS - 1 && arrayElements % NUMTHREADS != 0) // case where number of elements does not divide evenly among threads
        {
            int remainder = arrayElements % NUMTHREADS;
            right += remainder;
        }
        std::vector <int> v;        // load left and right index vector into main vector
        v.push_back(left);
        v.push_back(right);
        threadData.push_back(v);
        j += temp;
    }

    #pragma omp parallel    // omp parallelization
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)     // have omp handle threads for for oop
        {
            taskHandler(numarray, threadData[i][0], threadData[i][1]); // call as many tasks as there are threads  
        }
    }
    mergeSort(numarray, 0, arrayElements - 1);      // sort the results of the parallelized array
    unsigned long long elapsed_ns;                  // timing calculations
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed (s): %lf\n",elapsed_s);

// now write to output file
    std::ofstream outfile(output);
    if(outfile.is_open())
    {
        for(int j = 0; j < arrayElements; j++)
        {
            outfile << numarray[j] << std::endl;
        }
    }
    outfile.close();
    return 0;
}


/*
scp ./* hsu@128.138.189.219:~
ssh hsu@128.138.189.219
*/