#ifndef LOCKSBARRIERS
#define LOCKSBARRIERS
#include <iostream>
#include "counter.h"
#include <atomic>

#define RELAXED std::memory_order_relaxed
#define SEQCST std::memory_order_seq_cst

extern volatile int NUMTHREADS;
class Barrier
{
    public:
        std::atomic<int> sense {0};
        std::atomic<int> cnt {0};
        int N = NUMTHREADS;
        void wait();

};

class Lock
{
    public:
        void tasLock();
        void ttasLock();
        void unlock();
        void ticketLock();
        void unlockTicket();
};


#endif 