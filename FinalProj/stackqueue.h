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

#ifndef STACKQUEUE_H
#define STACKQUEUE_H

#include <iostream>
#include <atomic>
#include <thread>
#include <stack>
#include <queue>
#include "framework/cpp_framework.h"
#include "data_structures/ITest.h"

using namespace CCP;

#define ARRAY_SIZE 200

// MS queue class
class msqueue
{
    private:
    class node
    {
        public:
            node(int v)
            {
                val = v;
                next.store(NULL);
            }
            int val; 
            std::atomic<node*> next;
    };
    public:
        std::atomic<node*> head, tail;
        msqueue();
        void enqueue(int val);
        int dequeue();
        void printqueue();
};

// Treiber stack class
class tstack
{
    private:
        class tnode
        {
            public:
                tnode(int v)
                {
                    val = v;
                }
                int val;
                std::atomic<tnode *> down;
        };
    public:
        std::atomic<tnode *> top;
        tstack();
        void push(int val);
        int pop();
        void printstack();
};

// Single global lock class
class sglock
{
    public:
        void lock();
        void unlock();
    private:
        bool tas();
};

class myqueue
{
    public:
        std::queue<int> q;
        sglock lk;
        void push(int val);
        int pop();
        void printqueue();
};

class mystack
{
    public:
        void push(int val);
        int pop();
        void printstack();
    private:
        std::stack<int> stack;
        sglock lk;

};

class fcqueue : public ITest
{
    public:
    fcqueue() 
	:	_NUM_REP(_NUM_THREADS),
		_REP_THRESHOLD((int)(Math::ceil(_NUM_THREADS/(1.7))))
	{
		_head = Node::get_new(_NUM_THREADS);
		_tail = _head;
		_head->_values[0] = 1;
		_head->_values[1] = 0;

		_tail_slot.set(new SlotInfo());
		_timestamp = 0;
		_NODE_SIZE = 4;
		_new_node = null;
	}
    virtual ~fcqueue(){}
    struct Node 
    {
        Node* volatile _next;
        int	volatile _values[256];

        static Node* get_new(const int in_num_values) 
        {
            final size_t new_size = (sizeof(Node) + (in_num_values + 2 - 256) * sizeof(int));
			Node* final new_node = (Node*) malloc(new_size);
            new_node->_next = NULL;
            return new_node;
        }
    };

    CCP::AtomicInteger	_fc_lock;
	char							pad1[64];
	final int		_NUM_REP;
	final int		_REP_THRESHOLD;
	Node* volatile	_head;
	Node* volatile	_tail;
	int volatile	_NODE_SIZE;
	Node* volatile	_new_node;

	//helper function -----------------------------
    inline_	void flat_combining() 
    {
		// prepare for enq
		int volatile* enq_value_ary;
		if(_new_node == NULL) 
			_new_node = Node::get_new(_NODE_SIZE);
		enq_value_ary = _new_node->_values;
		*enq_value_ary = 1;
		++enq_value_ary;

		// prepare for deq
		int volatile * deq_value_ary = _tail->_values;
		deq_value_ary += deq_value_ary[0];

		//
		int num_added = 0;
		for (int iTry=0;iTry<_NUM_REP; ++iTry) {
			CCP::Memory::read_barrier();

			int num_changes=0;
			SlotInfo * curr_slot = _tail_slot.get();
			while(null != curr_slot->_next) {
				final int curr_value = curr_slot->_req_ans;
				if(curr_value > _NULL_VALUE) {
					++num_changes;
					*enq_value_ary = curr_value;
					++enq_value_ary;
					curr_slot->_req_ans = _NULL_VALUE;
					curr_slot->_time_stamp = _NULL_VALUE;

					++num_added;
					if(num_added >= _NODE_SIZE) {
						Node* final new_node2 = Node::get_new(_NODE_SIZE+4);
						memcpy((void*)(new_node2->_values), (void*)(_new_node->_values), (_NODE_SIZE+2)*sizeof(int) );
						//free(_new_node);
						_new_node = new_node2; 
						enq_value_ary = _new_node->_values;
						*enq_value_ary = 1;
						++enq_value_ary;
						enq_value_ary += _NODE_SIZE;
						_NODE_SIZE += 4;
					}
				} else if(_DEQ_VALUE == curr_value) {
					++num_changes;
					final int curr_deq = *deq_value_ary;
					if(0 != curr_deq) {
						curr_slot->_req_ans = -curr_deq;
						curr_slot->_time_stamp = _NULL_VALUE;
						++deq_value_ary;
					} else if(null != _tail->_next) {
						_tail = _tail->_next;
						deq_value_ary = _tail->_values;
						deq_value_ary += deq_value_ary[0];
						continue;
					} else {
						curr_slot->_req_ans = _NULL_VALUE;
						curr_slot->_time_stamp = _NULL_VALUE;
					} 
				}
				curr_slot = curr_slot->_next;
			}//while on slots

			if(num_changes < _REP_THRESHOLD)
				break;
		}//for repetition

		if(0 == *deq_value_ary && null != _tail->_next) {
			_tail = _tail->_next;
		} else {
			_tail->_values[0] = (deq_value_ary -  _tail->_values);
		}

		if(enq_value_ary != (_new_node->_values + 1)) {
			*enq_value_ary = 0;
			_head->_next = _new_node;
			_head = _new_node;
			_new_node  = null;
		} 
	}

    bool add(final int iThread, final int inValue);
    int remove(final int iThread, final int inValue);
    int contain(final int iThread, final int inValue);
	
    void print();
};

class fcstack : public ITest 
{
struct Node 
    {
		Node* volatile _next;
		int	_values[_MAX_THREADS];

		static Node* getNewNode(final int num_threads) {
			return (Node*) malloc(sizeof(Node) - (_MAX_THREADS-num_threads-2)*sizeof(int));
		}
	};

	//fields -------------------------------------- 
	int volatile	_req_ary[_MAX_THREADS];

	AtomicInteger	_fc_lock;

	int volatile	_top_indx;
	int volatile	_stack_ary_size;
	int volatile*	_stack_ary;		

	//helper function -----------------------------
	inline_ void flat_combining() {
		//
		int volatile* final stack_ary_end = _stack_ary + _stack_ary_size;		
		register int volatile* stack_ary = &(_stack_ary[_top_indx]);
		register int volatile* req_ary;

		for (register int iTry=0; iTry<3; ++iTry) {
			req_ary = _req_ary;
			for(register int iReq=0; iReq < _NUM_THREADS;) {
				register final int curr_value = *req_ary;
				if(curr_value > _NULL_VALUE) {
					if(stack_ary_end != stack_ary) {
						*stack_ary = curr_value;
						++stack_ary;
						*req_ary = _NULL_VALUE;
					}
				} else if(_DEQ_VALUE == curr_value) {
					if(stack_ary  != _stack_ary) {
						--stack_ary;
						*req_ary = -(*stack_ary);
					} else {
						*req_ary = _NULL_VALUE;
					}
				} 
				++req_ary;
				++iReq;
			}
		}

		//
		_top_indx = (stack_ary - _stack_ary);
	}

public:
	//public operations ---------------------------
	fcstack() 
	{
		for (int iReq=0; iReq<_MAX_THREADS; ++iReq) {
			_req_ary[iReq] = 0;
		}

		_stack_ary_size = 1024;
		_stack_ary = new int[_stack_ary_size];
		_top_indx = 0;
	}
    bool add(final int iThread, final int inValue);
	int remove(final int iThread, final int inValue);
    int contain(final int iThread, final int inValue);
    void print();
};

class basketqueue : public ITest 
{
private:
	static final int _MAX_HOPS = 3;

private:

	struct Node 
	{
		AtomicStampedReference<Node>	_next;
		final int						_value;

		Node() : _value(0), _next(null, 0) {}
		Node(int x) : _value(x), _next(null, 0) {}
	};

	AtomicStampedReference<Node>	_head;
	AtomicStampedReference<Node>	_tail;
	int volatile					_backoff;

	inline_ static _u32 getTag(AtomicStampedReference<Node>& inRef) {
		return ((inRef.getStamp() & ~0x8000) & 0xFFFF);
	}

	inline_ static bool getIsDel(AtomicStampedReference<Node>& inRef) {
		return 0 != (inRef.getStamp() & 0x8000);
	}

	inline_ static int addTag(AtomicStampedReference<Node>&	inRef, final int tagDelta) {
		return ((getTag(inRef) + tagDelta) & ~0x8000) & 0xFFFF;
	}

	inline_ static _u32 addTag(final int tag, final int tagDelta) {
		return ((tag + tagDelta) & ~0x8000) & 0xFFFF;
	}

	inline_ static _u32 createStamp(AtomicStampedReference<Node>&	inRef, final int tagDelta, final boolean isDeleted) {
		int new_stamp = addTag(inRef, tagDelta);
		if(isDeleted)
			new_stamp |= 0x8000;
		return new_stamp;
	}

	inline_ void free_chain(CasInfo& my_cas_info, AtomicStampedReference<Node> head, AtomicStampedReference<Node> new_head) {
		AtomicStampedReference<Node> next;

		if (_head.compareAndSet( head, new_head.getReference(), head.getStamp(), createStamp(head, 1, false))) {
			++(my_cas_info._succ);
			while (head.getReference() != new_head.getReference() ) {
				next = head->_next;
				//reclaim_node(head.ptr) 
				head = next;
			}
		} else {
			++(my_cas_info._failed);
		}
	}

	inline_ static void countBackOff(final int n) {
		for (int i = 0; i < n; i++) {;}
	}

public:
	basketqueue(int backoff_start_value=0) 
	: _backoff(backoff_start_value)
	{
		_backoff = 0;
		Node* sentinel = new Node();
		_head.set(sentinel, 0);
		_tail.set(sentinel, 0);
	}
	bool add(final int iThread, final int inValue); 
	int remove(final int iThread, final int inValue);
	int contain(final int iThread, final int inValue);
	void print();
};
#endif