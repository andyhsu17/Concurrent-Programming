#include <iostream>
#include <fstream>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "pthreadbarrier.h"
#include "mysort.h"
#include <atomic>
#include <array>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

pthread_barrier_t bar;
pthread_t* threads;
sortData_t * dataStruct;
int NUMTHREADS = 5;
struct timespec start, end;

bucketData_t * bucketData;

void global_init()
{
	threads = new pthread_t[NUMTHREADS];
	dataStruct = new sortData_t[NUMTHREADS];
    bucketData = new bucketData_t[NUMTHREADS];
	pthread_barrier_init(&bar, NULL, NUMTHREADS);   // number of threads that must reach the barrier before being returned is NUMTHREADS
}
void global_cleanup()
{
	delete[] threads;
    delete[] dataStruct;
    delete[] bucketData;
	pthread_barrier_destroy(&bar);
}

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


void * taskHandler(void * args)
{
    int left = ((sortData_t *)args)->left;
    int right = ((sortData_t * )args)->right;
    int * numarray = ((sortData_t *)args)->arr;
    if(!left)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
    }
    pthread_barrier_wait(&bar);
    mergeSort(numarray, left, right);
    pthread_barrier_wait(&bar);
    if(!left)
    {
        clock_gettime(CLOCK_MONOTONIC, &end);
    }
}

//*************************************
// bucket sort
//*************************************
// void bucketSort(int array[], int n, int arraysize) // n number of buckets
// {
//     int len = arraysize;
//     int index = 0;
//     int Eperbucket = arraysize / n;
//     std::vector <int> buckets[n];
    
    
//     for(int i = 0; i < n; i++)      // sort each bucket
//     {
//         sort(buckets[i].begin(), buckets[i].end());
//     }

//     index = 0;
//     for(int i = 0; i < n; i++)      // merge the buckets into one array
//     {
//         while(!buckets[i].empty()) 
//         {
//             array[index++] = *(buckets[i].begin());
//             buckets[i].erase(buckets[i].begin());
//         }
//     }
//     mergeSort(array, 0, arraysize - 1);
  
// }


void bucketSort(std::vector <int> array) // n number of buckets
{
    sort(array.begin(), array.end());
}


void * bucketHandler(void * args)
{
    std::vector <int> * numarray = (std::vector <int> *)args;
    bucketSort( *numarray);
    
}

int main(int argc, char* argv[])
{
    int ret;
    int  c;
    int i = 0;
    int numarray[1000];
    int arrayElements;
    int temp;
    int j = 0;
    bool fj;
    if(!strcmp(argv[1], "--name"))
    {
        std::cout << "Andrew Hsu" << std::endl;
        return 0;
    }
    else if(argc < 3) 
    {
        std::cout << "please enter input file and algorithm choice" << std::endl;
        return -1;
    }
    else if((argc == 5) && !strcmp(argv[2], "-t"))
    {
        NUMTHREADS = atoi(argv[3]);
        if (NUMTHREADS > 150) 
        {
            std::cout << "Please enter number of threads less than 150" << std::endl;
            return -1;
        }
        if(argc == 5)
        {
            if(!strcmp("--alg=fj", argv[4])) fj = true; 
            else if(!strcmp("--alg=bucket", argv[4])) fj = false;
            else
            {
                std::cout << "please enter algorithm desired" << std::endl;
                return -1;
            }
        }
    }
    else if(argc == 3)
    {
        if(!strcmp("--alg=fj", argv[2])) fj = true;
        else if(!strcmp("--alg=bucket", argv[2])) fj = false;
        else
        {
            std::cout << "please enter algorithm desired" << std::endl;
            return -1;
        }
    }
   
    std::fstream myfile(argv[1]);       // file input numbers into int array
    if(myfile.is_open())
    {
        while(myfile >> c)
        {
            numarray[i++] = c;
        }
    }
    myfile.close();
    arrayElements = i;
    
    if(fj)      // forkjoin
    {
        std::cout << "performing fork join" << std::endl;
        temp = arrayElements / NUMTHREADS;    // number of elements in each thread
        global_init();                  // initialize barriers and thread array
        for(int i = 0; i < NUMTHREADS; i++)
        {
            dataStruct[i].left = j;                 // separate the initial array into left and right indexes
            dataStruct[i].right = j + temp - 1;
            dataStruct[i].arr = numarray;
            j += temp;
            ret = pthread_create(&threads[i], NULL, &taskHandler, &dataStruct[i]);  // create first thread
            if(ret)
            {
                std::cout << "ERROR; pthread_create: "<< ret << std::endl;
                exit(-1);
            }
        }
        for(size_t k = 0; k < NUMTHREADS; k++)
        {
            ret = pthread_join(threads[k],NULL);
            if(ret)
            {
                std::cout << "ERROR; pthread_join: " <<  ret << std::endl;
                exit(-1);
            }
        }
        mergeSort(numarray, 0, arrayElements - 1);      // sort the results of the parallelized array
        unsigned long long elapsed_ns;                  // timing calculations
        elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
        printf("Elapsed (ns): %llu\n",elapsed_ns);
        double elapsed_s = ((double)elapsed_ns)/1000000000.0;
        printf("Elapsed (s): %lf\n",elapsed_s);

// now write to output file
        std::ofstream outfile("out.txt");
        if(outfile.is_open())
        {
            for(int j = 0; j < arrayElements; j++)
            {
                outfile << numarray[j] << std::endl;
            }
        }
        outfile.close();
    }

    else // bucket sort
    {
        int len = arrayElements;
        int n = 5;          // standard 5 buckets
        int index = 0;
        int Eperbucket = arrayElements / n;
        std::vector <int> buckets[n];
         
        std::cout << "performing bucketsort" << std::endl;

        for(int j = 0; j < n; j++)          
        {     
            for(int i = 0; i < Eperbucket; i++)      // split elements into buckets
            {
                buckets[j].push_back(numarray[index++]);
            }
        }
        for(int i = 0; i < n; i++)
        {
            // bucketData[i].arr = buckets[i];                                        // initialize threads
            int ret = pthread_create(&threads[i], NULL, &bucketHandler, &buckets[i]);  
            if(ret)
            {
                std::cout << "ERROR; pthread_create: "<< ret << std::endl;
                exit(-1);
            }
        }

        for(size_t k = 0; k < n; k++)
        {
            ret = pthread_join(threads[k],NULL);
            if(ret)
            {
                std::cout << "ERROR; pthread_join: " <<  ret << std::endl;
                exit(-1);
            }
        }

        index = 0;
        for(int i = 0; i < n; i++)      // merge the buckets into one array
        {
            while(!buckets[i].empty()) 
            {
                numarray[index++] = *(buckets[i].begin());
                buckets[i].erase(buckets[i].begin());
            }
        }

        mergeSort(numarray, 0, arrayElements - 1);
        for(int i = 0; i < arrayElements; i++)      // print elements of array
        {
            std::cout << numarray[i] << std::endl;
        }

    }

    return 0;
}

/*

scp ./* hsu@128.138.189.219:~
ssh hsu@128.138.189.219

*/