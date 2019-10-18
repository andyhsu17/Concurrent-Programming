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
#include <set>
#include <cmath>
#include <mutex> 
#include "locksbarriers.h"

pthread_barrier_t bar;
pthread_t* threads;
sortData_t * dataStruct;
volatile int NUMTHREADS = 5;
struct timespec start, end;
pthread_mutex_t gmutex;
bucketData_t * bucketData;
int numarray[10000];
int numbuckets = 10;
std::vector<std::multiset<int> > bucket;
std::mutex globalMutex;
std::atomic<int> part;
std::atomic <int> arrayElements;
bool sense = false, barpthread = false, tas = false, ttas = false, ticket = false, lockpthread = false;
std::string outputfile = "out.txt";
Lock lk;
Barrier mybar;
void handle_concurrency_primitive(bool position)
{
    if(!position)       
    {        
        if(tas) lk.tasLock();
        else if(ttas) lk.ttasLock();
        else if(ticket) lk.ticketLock();
        else if(lockpthread) pthread_mutex_lock(&gmutex);
    }
    if(position)
    {
        if(tas) lk.unlock();
        else if(ttas) lk.unlock();
        else if(ticket) lk.unlockTicket();
        else if(lockpthread) pthread_mutex_unlock(&gmutex);
    }
}
void handle_concurrency_primitive1()
{
        if(sense) mybar.wait();
        else if(barpthread) pthread_barrier_wait(&bar);
}

void global_init()
{
    part = 0;
	threads = new pthread_t[NUMTHREADS];
	dataStruct = new sortData_t[NUMTHREADS];
    bucket.resize(NUMTHREADS);
	pthread_barrier_init(&bar, NULL, NUMTHREADS);   // number of threads that must reach the barrier before being returned is NUMTHREADS
}
void global_cleanup()
{
	delete[] threads;
    delete[] dataStruct;
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
    thread_local bool position = 0;
    if(!left)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
    }
    handle_concurrency_primitive1();
    mergeSort(numarray, left, right);
    handle_concurrency_primitive1();
    if(!left)
    {
        clock_gettime(CLOCK_MONOTONIC, &end);
    }
}

void bucket_sort(int start,int n)
{
  int max = 0;
  thread_local bool position = 0;
  for(int i = 0; i <= n; i++) 
  {
      if(numarray[i] > max) max = numarray[i];
  }
  int divider = ceil(float(max+1) / NUMTHREADS);

  for (int i = start; i <= n; i++)
  {
    handle_concurrency_primitive(position);
    position = 1;
    int x = floor(numarray[i] / divider);
    bucket[x].insert(numarray[i]);
    handle_concurrency_primitive(position);
    position = 0;  }
}


void* bucket_sort(void* arg)
{
  int tid = part++;
  if(tid == 0)
        clock_gettime(CLOCK_MONOTONIC,&start);
  int temp = arrayElements / NUMTHREADS;
  int left = tid * temp;
  int right = (tid + 1) * temp - 1;
  if(tid == NUMTHREADS - 1 && arrayElements % NUMTHREADS != 0) 
  {
      int remainder = arrayElements % NUMTHREADS;
      right += remainder;
  }
  bucket_sort(left,right);       
  handle_concurrency_primitive1();
  if(tid == NUMTHREADS - 1)     
        clock_gettime(CLOCK_MONOTONIC,&end);       
}

int main(int argc, char* argv[])
{
    int ret;
    int  c;
    int i = 0;
    int temp;
    int j = 0;
    bool fj;
    if(!strcmp(argv[1], "--name"))
    {
        std::cout << "Andrew Hsu" << std::endl;
        return 0;
    }
    std::string sourcefile = argv[1];
    std::string outfile;
    for(int l = 0; l < argc; l++)
    {
        if(!strcmp(argv[l], "-o"))
        {
            outfile = atoi(argv[l + 1]); // standard numthreads is 5
        }
        if(!strcmp(argv[l], "-t"))
        {
            NUMTHREADS = atoi(argv[l + 1]); // standard numthreads is 5
        }
        if(argv[l][2] == *(char*)"a" && argv[l][3] == *(char*)"l" && argv[l][4] == *(char*)"g")
        {
            const char *algorithm = &argv[l][6];
            if(!strcmp(algorithm, "fj")) fj = true;
            if(!strcmp(algorithm, "bucket")) fj = false;
            std::cout << "algorithm is: " << algorithm << std::endl;
        }
        if(!strcmp(argv[l], "--bar=sense"))
        {
            sense = true;
        }
        if(!strcmp(argv[l], "--bar=pthread"))
        {
            barpthread = true;
        }
        if(!strcmp(argv[l], "--lock=tas"))
        {
            tas = true;
        }
        if(!strcmp(argv[l], "--lock=ttas"))
        {
            ttas = true;
        }
        if(!strcmp(argv[l], "--lock=ticket"))
        {
            ticket = true;
        }
        if(!strcmp(argv[l], "--lock=pthread"))
        {
            lockpthread = true;
        }
        if(!strcmp(argv[l], "-o"))
        {
            outputfile = argv[l + 1];
        } 
    }
    if(barpthread == sense == tas == ttas == ticket == lockpthread) 
    {
        std::cout << "Must choose one barrier or one lock" << std::endl; 
        return -1;
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
        std::cout << "performing bucketsort" << std::endl << std::endl;
        global_init();                  // initialize barriers and thread array

        for(int i = 0; i < NUMTHREADS; i++)
        {
            ret = pthread_create(&threads[i], NULL, &bucket_sort, (void*)NULL);  // create first thread
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
        unsigned long long elapsed_ns;                  // timing calculations
        elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
        printf("Elapsed (ns): %llu\n",elapsed_ns);
        double elapsed_s = ((double)elapsed_ns)/1000000000.0;
        printf("Elapsed (s): %lf\n",elapsed_s);

        std::vector<int> newvec;
        std::cout << "number of threads is: " << bucket.size() << std::endl;
        for(i = 0; i < bucket.size(); i++)
        {
            for(std::multiset<int>::iterator itr = bucket[i].begin(); itr != bucket[i].end(); itr++)
            {
                newvec.push_back(*itr);
            }
        }
        sort(newvec.begin(), newvec.end());
        // for(int i = 0; i < newvec.size(); i++)
        // {
        //     std::cout << newvec[i] << std::endl;
        // }
        // print contents of bucket

        // write to output file
        std::ofstream outfile(outputfile);
        if(outfile.is_open())
        {
            for(int j = 0; j < arrayElements; j++)
            {
                outfile << newvec[j] << std::endl;
            }
            outfile << "total time is: " << elapsed_ns << " ns"<< std::endl;
        }
        outfile.close();
        global_cleanup();
    }

    return 0;
}

/*

scp ./* hsu@128.138.189.219:~
ssh hsu@128.138.189.219

*/