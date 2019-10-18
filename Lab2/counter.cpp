#include <iostream>
#include <fstream>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "pthreadbarrier.h"
#include "counter.h"
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
#include <string>

//*************************************************************************
// andy finish testing all command line interfaces (barriers and locks)
// part 2: change mysort.cpp to use my own concurrency primitives
//*************************************************************************

struct timespec start, end;
pthread_t* threads;
volatile int NUMTHREADS = 5;
int NUMITERATIONS = 10;
Barrier bar;
pthread_barrier_t pthreadbar;
Lock lk;
pthread_mutex_t gmutex;
std::atomic<int> tid{0};

int cntr = 0;
std::atomic <int> threadcntr{0};
// status variables
bool sense = false, barpthread = false, tas = false, ttas = false, ticket = false, lockpthread = false;


/*
This function handles all user selection of different barriers or tickets. Just makes it cleaner
*/
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
        if(sense) bar.wait();
        else if(barpthread) pthread_barrier_wait(&pthreadbar);
        else if(tas) lk.unlock();
        else if(ttas) lk.unlock();
        else if(ticket) lk.unlockTicket();
        else if(lockpthread) pthread_mutex_unlock(&gmutex);
    }
}

void thread_main(int my_tid)
{
    thread_local bool position = 0;
    for(int i = 0; i < NUMITERATIONS; i++)
    {
        handle_concurrency_primitive(position);
        position = 1;
        if(i % NUMTHREADS == my_tid)
        {
            cntr++;
        }
        handle_concurrency_primitive(position);
        position = 0;
    }
}

void * handler(void* args)
{
    int tid = threadcntr++;
    if(tid == 0)
        clock_gettime(CLOCK_MONOTONIC,&start);
    thread_main(tid);
    if(tid == NUMTHREADS - 1)
        clock_gettime(CLOCK_MONOTONIC,&end);
    tid++;
}

int main(int argc, char* argv[])
{
    std::string outputfile = "out1.txt";
    if(!strcmp(argv[1], "--name"))
    {
        std::cout << "Andrew Hsu" << std::endl;
        return 0;
    }
    for(int i = 0; i < argc; i++)
    {
        if(!strcmp(argv[i], "-t"))
        {
            NUMTHREADS = atoi(argv[i + 1]); // standard numthreads is 5
        }
        if(argv[i][0] == *(char*)"-" && argv[i][1] == *(char*)"i")          // this allows us to pull -i=30 into the number of iterations
        {
            NUMITERATIONS = atoi(argv[i]+3);
        }
        if(!strcmp(argv[i], "--bar=sense"))
        {
            sense = true;
        }
        if(!strcmp(argv[i], "--bar=pthread"))
        {
            barpthread = true;
        }
        if(!strcmp(argv[i], "--lock=tas"))
        {
            tas = true;
        }
        if(!strcmp(argv[i], "--lock=ttas"))
        {
            ttas = true;
        }
        if(!strcmp(argv[i], "--lock=ticket"))
        {
            ticket = true;
        }
        if(!strcmp(argv[i], "--lock=pthread"))
        {
            lockpthread = true;
        }
        if(!strcmp(argv[i], "-o"))
        {
            outputfile = argv[i + 1];
        } 
    }
    if(barpthread == sense == tas == ttas == ticket == lockpthread) 
    {
        std::cout << "Must choose one barrier or one lock" << std::endl; 
        return -1;
    }
    int ret;
    threads = new pthread_t[NUMTHREADS];
    pthread_barrier_init(&pthreadbar, NULL, NUMTHREADS);
    for(int i = 0; i < NUMTHREADS; i++)
    {
        ret = pthread_create(&threads[i], NULL, &handler, (void*)NULL);  // create first thread
        if(ret)
        {
            std::cout << "ERROR; pthread_create: "<< ret << std::endl;
            exit(-1);
        }
    }

    for(int k = 0; k < NUMTHREADS; k++)
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
    std::cout << "counter is: " << cntr << std::endl;
    std::ofstream outfile(outputfile);
    if(outfile.is_open())
    {
       
        outfile <<  "counter is: " << cntr << std::endl;
        outfile << "total time is: " << elapsed_ns << " ns"<< std::endl;
    }
    outfile.close();
    return 0;
}



//scp ./* hsu@128.138.189.219:~
//ssh hsu@128.138.189.219

