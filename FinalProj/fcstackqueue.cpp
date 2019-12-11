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
#include <stdbool.h>
#include <fstream>
#include <vector>
#include <omp.h>
#include <time.h>
#include "stackqueue.h"
#include "framework/cpp_framework.h"
#include "data_structures/ITest.h"

int _gNumThreads;

struct timespec start, end;
int volatile arrayElements;
    //push ......................................................
	bool fcstack::add(final int iThread, final int inValue) {
		int volatile* final my_value = &(_req_ary[iThread]);
		*my_value = inValue;

		do {
			bool is_cas = false;
			if(lock_fc(_fc_lock, is_cas)) {
				//sched_start(iThread);
				flat_combining();
				_fc_lock.set(0);
				//sched_stop(iThread);
				return true;
			} else {
				Memory::write_barrier();
				while(_NULL_VALUE != *my_value && 0 != _fc_lock.getNotSafe()) {
					thread_wait(_NUM_THREADS, true);
				} 
				Memory::read_barrier();
				if(_NULL_VALUE == *my_value)
					return true;
			}
		} while(true);
	}

	//pop ......................................................
	int fcstack::remove(final int iThread, final int inValue) 
    {
		int volatile* final my_value = &(_req_ary[iThread]);
		*my_value = _DEQ_VALUE;

		do {
			boolean is_cas = false;
			if(lock_fc(_fc_lock, is_cas)) {
				//sched_start(iThread);
				flat_combining();
				_fc_lock.set(0);
				//sched_stop(iThread);
				return -*my_value;
			} else {
				Memory::write_barrier();
				while (_DEQ_VALUE == *my_value && 0 != _fc_lock.getNotSafe()) {
					thread_wait(_NUM_THREADS, true);
				} 
				Memory::read_barrier();
				if(_DEQ_VALUE != *my_value)
					return -*my_value;
			}
		} while(true);
	}
int fcstack::contain(final int iThread, final int inValue) 
{
		return 0;
}
void fcstack::print()
{
  
    for(int volatile i = _top_indx; i < arrayElements ;i++)
    {
        std::cout << _req_ary[i] << std::endl;
    }
   
    return;
}
int fcqueue::contain(final int iThread, final int inValue) 
{
		return 0;
}
bool fcqueue::add(final int iThread, final int inValue)
{
        CasInfo& my_cas_info = _cas_info_ary[iThread];

		SlotInfo* my_slot = _tls_slot_info;
		if(null == my_slot)
			my_slot = get_new_slot();

		SlotInfo* volatile&	my_next = my_slot->_next;
		int volatile& my_re_ans = my_slot->_req_ans;
		my_re_ans = inValue;

		do {
			if (null == my_next)
				enq_slot(my_slot);

			boolean is_cas = true;
			if(lock_fc(_fc_lock, is_cas)) {
				++(my_cas_info._succ);
				flat_combining();
				_fc_lock.set(0);
				++(my_cas_info._ops);
				return true;
			} else {
				Memory::write_barrier();
				if(!is_cas)
					++(my_cas_info._failed);
				while(_NULL_VALUE != my_re_ans && 0 != _fc_lock.getNotSafe()) {
					thread_wait(iThread);
				} 
				Memory::read_barrier();
				if(_NULL_VALUE == my_re_ans) {
					++(my_cas_info._ops);
					return true;
				}
			}
		} while(true);
}

int fcqueue::remove(final int iThread, final int inValue)
{
        CasInfo& my_cas_info = _cas_info_ary[iThread];

		SlotInfo* my_slot = _tls_slot_info;
		if(null == my_slot)
			my_slot = get_new_slot();

		SlotInfo* volatile&	my_next = my_slot->_next;
		int volatile& my_re_ans = my_slot->_req_ans;
		my_re_ans = _DEQ_VALUE;

		do {
			if(null == my_next)
				enq_slot(my_slot);

			boolean is_cas = true;
			if(lock_fc(_fc_lock, is_cas)) {
				++(my_cas_info._succ);
				flat_combining();
				_fc_lock.set(0);
				++(my_cas_info._ops);
				return -(my_re_ans);
			} else {
				Memory::write_barrier();
				if(!is_cas)
					++(my_cas_info._failed);
				while(_DEQ_VALUE == my_re_ans && 0 != _fc_lock.getNotSafe()) {
					thread_wait(iThread);
				}
				Memory::read_barrier();
				if(_DEQ_VALUE != my_re_ans) {
					++(my_cas_info._ops);
					return -(my_re_ans);
				}
			}
		} while(true);
}

void fcqueue::print()
{
    Node * volatile temphead = _head;
    while(_head != NULL)
    {
        for(int i = 0; i < sizeof(_head->_values) / sizeof(*_head->_values); i++)
        {
            if(_head->_values[i] < 8000 && _head->_values[i] > 0)
            std::cout << _head->_values[i] << " " << std::endl;
        }

        _head = _head->_next;
    }
    _head = temphead;
    
}

int main(int argc, char * argv[])
{
    ITest * fcq = (ITest*)(new fcqueue()); 
    ITest * fcs = (ITest*)(new fcstack());
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
            std::cout << "Execution instructions: \nfcstackqueue [--name] [sourcefile.txt] [-t NumThreads]\n" << std::endl;
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
    fcq->print();
    std::cout << "------" << std::endl << std::endl;
std::cout << "testing add n elements" << std::endl;
    clock_gettime(CLOCK_MONOTONIC,&start);
    #pragma omp parallel default(none) shared(fcq, NUMTHREADS, threadData)
    {
        omp_set_num_threads(NUMTHREADS);
        #pragma omp for
        for(int i = 0; i < omp_get_num_threads(); i++)
        {
            int threadno = omp_get_thread_num();
            for(int j = 0; j < threadData[threadno].size(); j++)
                fcq->add(threadno, threadData[threadno][j]);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fcq->print();
    std::cout << "----------" << std::endl << std::endl;
    
    std::cout << "testing remove n elements" << std::endl;
    #pragma omp parallel default(none) shared(fcq, NUMTHREADS, arrayElements)
    {
        omp_set_num_threads(NUMTHREADS);
        #pragma omp for
        for(int i = 0; i < omp_get_num_threads(); i++)
        {
            int threadno = omp_get_thread_num();
            for(int j = 0; j < arrayElements; j++)
                fcq->remove(threadno, 0);
        }
    }
    fcq->print();
    std::cout << "----------" << std::endl << std::endl;

    std::cout << "testing enqueuing and dequeuing concurrently" << std::endl;
    #pragma omp parallel default(none) shared(fcq, NUMTHREADS, arrayElements, threadData)
    {
        omp_set_num_threads(NUMTHREADS);
        #pragma omp for
        for(int i = 0; i < omp_get_num_threads(); i++)
        {
            int threadno = omp_get_thread_num();
            for(int j = 0; j < threadData[threadno].size(); j++)     
                fcq->add(threadno, threadData[threadno][j]);
            fcq->remove(threadno, 0);
        }
    }
    fcq->print();
    unsigned long long elapsed_ns;                  // timing calculations
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed queue (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed queue (s): %lf\n",elapsed_s);
    std::cout << std::endl << std::endl;
    for(int j = 0; j < NUMTHREADS; j++)
    {
        for(std::vector<int>::iterator i = threadData[j].begin(); i != threadData[j].end(); i++)
        {
            std::cout << *i << std::endl;
        }

    }

    // std::cout << "original publication list: " << std::endl;
    // fcs->print();
    std::cout << "------" << std::endl << std::endl;
    std::cout << "testing add n elements" << std::endl;
    clock_gettime(CLOCK_MONOTONIC,&start);
    #pragma omp parallel default(none) shared(fcs, NUMTHREADS, threadData)
    {
        omp_set_num_threads(NUMTHREADS);
        #pragma omp for
        for(int i = 0; i < omp_get_num_threads(); i++)
        {
            int threadno = omp_get_thread_num();
        // for(std::vector<int>::iterator iter = threadData[i].begin(); iter != threadData[i].end(); iter++)
        //         fcs->add(threadno, *iter);
            for(int j = 0; j < threadData[threadno].size(); j++)
                fcs->add(threadno, threadData[threadno][j]);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fcs->print();
    std::cout << "----------" << std::endl << std::endl;
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed stack (ns): %llu\n",elapsed_ns);
    elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed stack (s): %lf\n",elapsed_s);
    std::cout << std::endl << std::endl;
    return 0;
}

