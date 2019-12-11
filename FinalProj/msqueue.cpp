#include <iostream>
#include <atomic>
#include <thread>
#include <time.h>
#include <fstream>
#include <cstring>
#include <mutex>
#include "stackqueue.h"

struct timespec start, end;
msqueue::msqueue()
{
    node * dummy = new node(0);
    head.store(dummy);
    tail.store(dummy);
}

void msqueue::enqueue(int val)
{
    node *t, *e, *n;
    node* temp = NULL;
    n = new node(val);
    
    while(true)
    {
        t = tail.load(); 
        e = t->next.load();
        if(t == tail.load())                // case where tail next is null, dont enqueue anything
        {
            if(e == NULL)
            {
                if(t->next.compare_exchange_weak(e,n))  break;
            }
            else                                // case where tail gets shifted one right
            {
                tail.compare_exchange_weak(t,e);
            }
        }
    }
    tail.compare_exchange_weak(t,n);
}

int msqueue::dequeue()
{
    node *t, *h, *n;
    while(1)
    {
        h = head.load(); t = tail.load(); n = h->next.load();
        if(h == head.load())
        {
            if(h == t)              // if head and tail are the same (queue is full or empty)
            {
                if(n == NULL) return -1;          // there is nothing to dequeue
                tail.compare_exchange_weak(t,n);
            }
            else                    // otherwise replace head with head next
            {
                int ret = n->val;
                if(head.compare_exchange_weak(h, n)) return ret;
            }
        }
    }
}

void msqueue::printqueue()
{
    node * temp;
    node * tempnext;
    std::atomic<node*> temphead{head.load()};
    while(head.load()->next.load() != NULL)
    {
        temp = head.load();
        tempnext = temp->next.load();
        std::cout << temp->val << std::endl;
        head.compare_exchange_weak(temp, tempnext);
    }
    std::cout << std::endl;
    head = temphead.load();     // reset the head
}

int main(int argc, char * argv[])
{
    msqueue q;
    int numarray[ARRAY_SIZE];
    int c;
    int i = 0;
    if(argc < 2) 
    {
        std::cout << "please enter sourcefile name" << std::endl;
        return -1;
    }
    std::fstream myfile(argv[1]);       // file input numbers into int array
    int NUMTHREADS = 5;                 // default 5 number of threads
    
    if(!strcmp(argv[1], "--name"))
    {
        std::cout << "Andrew Hsu" << std::endl;
        return 0;
    }

    for(int i = 0; i < argc; i++)
    {
        if(!strcmp(argv[i], "-h"))
        {
            std::cout << "Execution instructions: \n\nmsqueue [--name] [sourcefile.txt] [-t NumThreads]\n" << std::endl;
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
    std::cout << "testing msqueue push with 100 elements" << std::endl << std::endl;
    clock_gettime(CLOCK_MONOTONIC,&start);
    #pragma omp parallel default(none) shared(NUMTHREADS, q, threadData)  // put 300 data values in the stack testing push()
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)     // have omp handle threads for for loop
        {
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)
            {
                q.enqueue(*iter);
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    q.printqueue();
    unsigned long long elapsed_ns;                  // timing calculations
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed stack (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed stack (s): %lf\n",elapsed_s);
    std::cout << std::endl << std::endl;

    std::cout << "testing dequeue all" << std::endl << std::endl;
    // testing dequeue all 
    #pragma omp parallel default(none) shared(q, arrayElements)
    {
        #pragma omp for
        for(int i = 0; i < arrayElements; i++)
        {
            q.dequeue();                                                            // pop all of them in parallel to test pop
        }
    }
    q.printqueue();  
    std::cout << std::endl << std::endl;

    std::cout << "testing enqueue and dequeue concurrently" << std::endl << std::endl;
    // testing stack pop and push concurrently
   #pragma omp parallel default(none) shared(NUMTHREADS, q, threadData)
    {
        #pragma omp for
        for(int i = 0; i < NUMTHREADS; i++)
        {
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)   // test popping and pushing at the same time
            {
                q.enqueue(*iter);
            }                                           
                q.dequeue();
        }
    }
    q.printqueue();
        std::cout << std::endl << std::endl;
    return 0;
}