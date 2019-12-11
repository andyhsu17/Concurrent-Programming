////////////////////////////////////////////////////////////////////////////////
// File    : fcqueue.cpp
// Author  : Ms.Moran Tzafrir;  email: morantza@gmail.com; tel: 0505-779961
// Written : 27 October 2009
//
// Copyright (C) 2009 Moran Tzafrir.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License 
// along with this program; if not, write to the Free Software Foundation
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

// Used helper functions written by Moran Tzafrir
// Libraries used in fcstackqueue.cpp, basketqueue.cpp, stackqueue.h 
// December 9, 2019 Andrew Hsu
// ECEN4313 Concurrent Programming

#include <iostream>
#include <fstream>
#include <stdbool.h>
#include <time.h>
#include <omp.h>
#include "stackqueue.h"
#include "framework/cpp_framework.h"
#include "data_structures/ITest.h"

volatile int arrayElements;
struct timespec start, end;
int _gNumThreads;
bool basketqueue::add(final int iThread, final int inValue) 
{
    CasInfo& my_cas_info = _cas_info_ary[iThread];
    Node* nd = new Node(inValue);
    int backoff = 1;
    AtomicStampedReference<Node> tail, next;

    while (true) {
        Memory::read_write_barrier();
        tail = _tail;
        next = tail->_next;
        if (tail == _tail) {
            if (null == next.getReference()) {
                nd->_next.set(null, createStamp(tail, 2, false));
                if (tail->_next.compareAndSet(next.getReference(), nd, next.getStamp(), createStamp(tail, 1, false) )) {
                    ++(my_cas_info._succ);
                    if(_tail.compareAndSet(tail.getReference(), nd, tail.getStamp(), createStamp(tail, 1, false)))
                        ++(my_cas_info._succ);
                    else 
                        ++(my_cas_info._failed);

                    ++(my_cas_info._ops);
                    return true;
                } else {
                    ++(my_cas_info._failed);
                }

                next = tail->_next;
                while( (getTag(next) == addTag(getTag(tail), 1)) && (!getIsDel(next)) ) {
                    countBackOff(backoff * (iThread+1)); backoff*=2;
                    nd->_next = next;

                    if (tail->_next.compareAndSet(next.getReference(), nd, next.getStamp(), createStamp(tail, 1, false))) {
                        ++(my_cas_info._succ);
                        ++(my_cas_info._ops);
                        return true;
                    } else {
                        ++(my_cas_info._failed);
                    }
                    next = tail->_next;
                }
            } else {
                while (null != next->_next.getReference() && _tail == tail) {
                    next = next->_next;
                }
                if(_tail.compareAndSet(tail.getReference(), next.getReference(), tail.getStamp(), createStamp(tail, 1, false) ))
                    ++(my_cas_info._succ);
                else 
                    ++(my_cas_info._failed);
            }
        }
    }
}

	int basketqueue::remove(final int iThread, final int inValue) {
		CasInfo& my_cas_info = _cas_info_ary[iThread];

		AtomicStampedReference<Node> head, tail, next, iter;
		int backoff = 1;

		while (true) {
			Memory::read_write_barrier();
			head = _head;
			tail = _tail;
			next = head->_next;

			if (head == _head) {
				if (head.getReference() == tail.getReference()) {
					if (null == next.getReference()) {
						++(my_cas_info._ops);
						return _NULL_VALUE;
					}
					while ( (null != next->_next.getReference())  && _tail == tail) {
						next = next->_next;
					}
					if(_tail.compareAndSet(tail.getReference(), next.getReference(), tail.getStamp(), createStamp(tail, 1, false)))
						++(my_cas_info._succ);
					else 
						++(my_cas_info._failed);

				} else {
					iter = head;
					int hops = 0;

					while (getIsDel(next) && (iter.getReference() != tail.getReference()) && _head == head) {
						iter = next;
						next = iter->_next;
						++hops;
					}

					if (_head != head) {
						continue;
					} else if (iter.getReference() == tail.getReference()) {
						free_chain(my_cas_info, head, iter);
					} else {
						final int rc_value = next->_value;
						if (iter->_next.compareAndSet(next, next.getReference(), next.getStamp(), createStamp(next, 1, true)) ) {
							++(my_cas_info._succ);

							if (hops >= _MAX_HOPS) {
								free_chain(my_cas_info, head, next);
							}
							++(my_cas_info._ops);
							return rc_value;
						} else {
							++(my_cas_info._failed);
						}
						countBackOff(backoff * (iThread+1)); backoff*=2;
					}
				}
			}
		}
	}

	int basketqueue::contain(final int iThread, final int inValue) {
		return 0;
	}

    void basketqueue::print()
    {
            // AtomicStampedReference<Node> tail, next;
        AtomicStampedReference<Node> savehead = _head;
        while(_head->_next != NULL)
        {
            std::cout << _head->_value << std::endl;
            _head = _head->_next;
        }
        _head = savehead;
        return;
    }
int main(int argc, char * argv[])
{
    ITest * basket = (ITest*)(new basketqueue());
    int NUMTHREADS = 5;
    int c;
    int i = 0;
    int numarray[ARRAY_SIZE];

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
        if(!strcmp(argv[i], "-t"))
        {
            NUMTHREADS = atoi(argv[i + 1]); // standard numthreads is 5
        }
        if(!strcmp(argv[i], "-h"))
        {
            std::cout << "Execution instructions: \nbasketqueue [--name] [sourcefile.txt] [-t NumThreads]\n" << std::endl;
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
    // int const arrayElements = i;
    arrayElements = i;
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
    std::cout << "original publication list: " << std::endl;
    basket->print();
    std::cout << "------" << std::endl << std::endl;
std::cout << "testing enqueue n elements" << std::endl;
    clock_gettime(CLOCK_MONOTONIC,&start);
    #pragma omp parallel default(none) shared(basket, NUMTHREADS, threadData)
    {
        omp_set_num_threads(NUMTHREADS);
        #pragma omp for
        for(int i = 0; i < omp_get_num_threads(); i++)
        {
            int threadno = omp_get_thread_num();
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)
                basket->add(threadno, *iter);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    basket->print();
    std::cout << "----------" << std::endl << std::endl;
    std::cout << "testing dequeue all elements" << std::endl;
    #pragma omp parallel default(none) shared(basket, NUMTHREADS, threadData)
    {
        omp_set_num_threads(NUMTHREADS);
        #pragma omp for
        for(int i = 0; i < omp_get_num_threads(); i++)
        {
            int threadno = omp_get_thread_num();
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)
                basket->remove(threadno, 0);
        }
    }
        basket->print();
    std::cout << "----------" << std::endl << std::endl;


    std::cout << "testing concurrently enqueuing and dequeueing all elements" << std::endl;
    #pragma omp parallel default(none) shared(basket, NUMTHREADS, threadData)
    {
        omp_set_num_threads(NUMTHREADS);
        #pragma omp for
        for(int i = 0; i < omp_get_num_threads(); i++)
        {
            int threadno = omp_get_thread_num();
            for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)
                basket->add(threadno, *iter);
            basket->remove(threadno, 0);
        }
    }
        basket->print();
    std::cout << "----------" << std::endl << std::endl;
    unsigned long long elapsed_ns;                  // timing calculations
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed stack (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed stack (s): %lf\n",elapsed_s);
    std::cout << std::endl << std::endl;

    return 0;
}