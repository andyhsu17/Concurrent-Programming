#include <iostream>
#include <atomic>
#include <stdbool.h>
#include <stack>
#include <queue>
#include <fstream>
#include <omp.h>
#include <cstring>
#include <time.h>
#include "stackqueue.h"

std::atomic_flag globalflag = ATOMIC_FLAG_INIT;
struct timespec start, end;

void sglock::lock()
{
    while(globalflag.test_and_set(std::memory_order_acquire));
}

void sglock::unlock()
{
    globalflag.clear(std::memory_order_release);
}

void myqueue::push(int val)
{
    lk.lock();
    q.push(val);
    lk.unlock();
}

int myqueue::pop()
{
    while(true)
    {
        lk.lock();
        size_t size = q.size();
        if(size == 0) lk.unlock();
        else
        {
            int ret = q.front();           
            q.pop();
            lk.unlock();
            return ret;
        }
    }
    
    std::cout << std::endl << std::endl;
}

void myqueue::printqueue()
{
    std::queue<int> temp = q;
    if(temp.empty()) std::cout << "queue is empty" << std::endl;
    while(!temp.empty())
    {
        std::cout << temp.front() << std::endl;
        temp.pop();
    }
    std::cout << std::endl;
}

void mystack::push(int val)
{
    lk.lock();
    stack.push(val);
    lk.unlock();
}

int mystack::pop()
{
    while(1)
    {
        lk.lock();
        size_t size = stack.size();

        if(size == 0) lk.unlock();
        else
        {
            int ret = stack.top();            
            stack.pop();
            lk.unlock();
            return ret;
        }
    }
}

void mystack::printstack()
{
    std::stack<int> temp = stack;
    if(temp.empty()) std::cout << "stack is empty" << std::endl;
    while(!temp.empty())
    {
        std::cout << temp.top() << std::endl;
        temp.pop();
    }
        std::cout << std::endl;
}

int main(int argc, char * argv[])
{

    myqueue theq;
    mystack stk;
    int numarray[ARRAY_SIZE];
    int c;
    int i = 0;

    int NUMTHREADS = 5;                 // default 5 number of threads
    if(argc < 2) 
    {
        std::cout << "please enter sourcefile name" << std::endl;
        return -1;
    }
    std::fstream myfile(argv[1]);       // file input numbers into int array

    if(!strcmp(argv[1], "--name"))
    {
        std::cout << "Andrew Hsu" << std::endl;
        return 0;
    }

    for(int i = 0; i < argc; i++)
    {
        if(!strcmp(argv[i], "-h"))
        {
            std::cout << "Execution instructions: \n\nsglstackqueue [--name] [sourcefile.txt] [-t NumThreads]\n" << std::endl;
            return 0;
        }
        if(!strcmp(argv[i], "-t"))
        {
            NUMTHREADS = atoi(argv[i + 1]); // standard numthreads is 5
        }
    }
    
    if(myfile.is_open())
    {
        while(myfile >> c)
        {
            numarray[i++] = c;
        }
    }
    int const arrayElements = i;
    int temp = arrayElements / NUMTHREADS;
    int index = 0;
    std::vector <std::vector<int>> threadData; 
    for(int i = 0; i < NUMTHREADS; i++)     
    {
        std::vector <int> v;       
        for(int k = index; k < index + temp; k++)
        {
            v.push_back(numarray[k]);
        }
        index += temp;
        if(i == NUMTHREADS - 1 && arrayElements % NUMTHREADS != 0)
        {
            int remainder = arrayElements % NUMTHREADS;
            for(int l = index; l < index + remainder; l++)
            {
                v.push_back(numarray[l]);
            }
        }
        threadData.push_back(v);    
    }
    // testing stack push 300 vals
    std::cout << "testing stack push with SGL... " << std::endl << std::endl;
    #pragma omp parallel default(none) shared(NUMTHREADS, stk, threadData)  // put 300 data values in the stack testing push()
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)     // have omp handle threads for for loop
        {
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)
            {
                stk.push(*iter);
            }
        }
    }
    stk.printstack();                                                                 // print 300 values
    std::cout <<  "--------" << std::endl << std::endl;                             // separation for easy reading
    
    std::cout << "testing stack pop all..." << std::endl << std::endl;
    // testing stack pop all 
    #pragma omp parallel default(none) shared(NUMTHREADS, stk, arrayElements)
    {
        #pragma omp for
        for(int i = 0; i < arrayElements; i++)
        {
            stk.pop();                                                            // pop all of them in parallel to test pop
        }
    }
    stk.printstack();                                                           // now should be empty
    std::cout <<  "--------" << std::endl << std::endl;
    
    std::cout << "testing stack pop and push concurrently..." << std::endl << std::endl;
    clock_gettime(CLOCK_MONOTONIC,&start);
   // testing stack pop and push concurrently
   #pragma omp parallel default(none) shared(NUMTHREADS, stk, threadData)
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)
        {
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)   // test popping and pushing at the same time
            {
                stk.push(*iter);
            }                                           
                stk.pop();
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    stk.printstack();
    unsigned long long elapsed_ns;                  // timing calculations
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed stack (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed stack (s): %lf\n",elapsed_s);
    std::cout << std::endl << std::endl;

// testing queue push 300 vals
    std::cout << "testing queue push  with SGL... " << std::endl << std::endl;
    #pragma omp parallel default(none) shared(NUMTHREADS, theq, threadData)  // put 300 data values in the stack testing push()
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)     // have omp handle threads for for loop
        {
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)
            {
                theq.push(*iter);
            }
        }
    }
    theq.printqueue();                                                                 // print 300 values
    std::cout <<  "--------" << std::endl << std::endl;                             // separation for easy reading
    
    std::cout << "testing queue pop all with SGL..." << std::endl << std::endl;
    // testing queue pop all 
    #pragma omp parallel default(none) shared(NUMTHREADS, theq, arrayElements)
    {
        #pragma omp for
        for(int i = 0; i < arrayElements; i++)
        {
            theq.pop();                                                            // pop all of them in parallel to test pop
        }
    }
    theq.printqueue();                                                           // now should be empty
    std::cout <<  "--------" << std::endl << std::endl;
    std::cout << "testing queue pop and push concurrently" << std::endl << std::endl;
    clock_gettime(CLOCK_MONOTONIC,&start);
   // testing queue pop and push concurrently
   #pragma omp parallel default(none) shared(NUMTHREADS, theq, threadData)
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)
        {
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)   // test popping and pushing at the same time
            {
                theq.push(*iter);
            }                                           
                theq.pop();
        }
    }
        clock_gettime(CLOCK_MONOTONIC,&end);
    theq.printqueue();
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed queue (ns): %llu\n",elapsed_ns);
    elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed queue (s): %lf\n",elapsed_s);

    return 0;
}

/*
scp ./* hsu@128.138.189.219:~
ssh hsu@128.138.189.219
*/