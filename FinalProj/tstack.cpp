#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <omp.h>
#include <cstring>
#include <vector>
#include <time.h>
#include "stackqueue.h"

struct timespec start, end;
int numarray[ARRAY_SIZE];

tstack::tstack()
{
    tnode * dummy = new tnode(0);
    top.store(dummy);
}

void tstack::push(int val)
{            
    tnode * n = new tnode(val);
    tnode * t = NULL;
    do
    {
        t = top.load(std::memory_order_acquire);
        n->down = t;
    }while(!top.compare_exchange_weak(t, n, std::memory_order_acq_rel));
}

int tstack::pop()
{
    int v;
    tnode * t;
    tnode * n;
    do
    {
        t = top.load(std::memory_order_acquire);
        if(t == NULL) return NULL;
        n = t->down;
        v = t->val;
    }while(!top.compare_exchange_weak(t, n, std::memory_order_acq_rel));
    return v;
}

void tstack::printstack()
{
    tnode * t;
    tnode * n;
    std::atomic<tnode *> temptop {top.load()};   // save top pointer so we can reset the top

    while(top.load() != NULL)
    {
        t = top.load();
        n = t->down.load();
        std::cout << t->val << std::endl;
        top.compare_exchange_weak(t,n); // move top pointer to one below
    } 
    std::cout << std::endl;
    top = temptop.load();
}

int main(int argc, char * argv[])
{
    tstack stack;
    int c;
    int i = 0;
    if(argc < 2) 
    {
        std::cout << "please enter sourcefile name" << std::endl;
        return -1;
    }
    std::fstream myfile(argv[1]);       // file input numbers into int array
    int NUMTHREADS = 5;
    
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
        if(!strcmp(argv[i], "-h"))
        {
            std::cout << "Execution instructions: \ntstack [--name] [sourcefile.txt] [-t NumThreads]\n" << std::endl;
            return 0;
        }
    }
   
    if(myfile.is_open())
    {
        while(myfile >> c)
        {
            numarray[i++] = c;
        }
    }
    int arrayElements = i;
    
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
    std::cout << "pushing 300 elements to treiber stack..." << std::endl << std::endl;
        clock_gettime(CLOCK_MONOTONIC,&start);
    #pragma omp parallel default(none) shared(numarray, NUMTHREADS, stack, threadData)  // put 300 data values in the stack testing push()
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)     // have omp handle threads for for oop
        {
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)
            {
                stack.push(*iter);
            }
        }
    }
        clock_gettime(CLOCK_MONOTONIC,&end);
    stack.printstack();                                                                 // print 300 values
    unsigned long long elapsed_ns;                  // timing calculations
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed stack (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed stack (s): %lf\n",elapsed_s);
    std::cout <<  "--------" << std::endl << std::endl;

    std::cout << "popping 1/5 of elements..." << std::endl;
    #pragma omp parallel default(none) shared(NUMTHREADS, stack, arrayElements)
    {
        #pragma omp for
        for(int i = 0; i < arrayElements / 5; i++)
        {
            stack.pop();                                                            // pop 1/5 of them in parallel to test pop
        }
    }
    stack.printstack();
    std::cout <<  "--------" << std::endl << std::endl;
    std::cout << "popping remaining elements..." << std::endl;
    for(int i = 0; i < 4 * arrayElements / 5; i++)
    {
        stack.pop();
    }
    stack.printstack();


    std::cout << "testing stack pop and push concurrently" << std::endl << std::endl;
   // testing queue pop and push concurrently
   #pragma omp parallel default(none) shared(NUMTHREADS, stack, threadData)
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)
        {
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)   // test popping and pushing at the same time
            {
                stack.push(*iter);
            }                                           
                stack.pop();
        }
    }
    stack.printstack();

    return 0;
}