#include <atomic>
#include <iostream>
#include <stdbool.h>
#include <thread>
#include "locksbarriers.h"

std::atomic<bool> flag {false};
std::atomic<int> next_num{0};
std::atomic<int> now_serving{0};

void Barrier::wait()
{
    bool thread_local mysense = 0;
    if(mysense == 0) mysense = 1;
    else mysense = 0;
    int cnt_cpy = cnt.fetch_add(1);
    if(cnt_cpy == (NUMTHREADS-1)) // last to arrive at barrier
    {
        cnt.store(0, std::memory_order_relaxed);
        sense.store(mysense);
    }
    else
    {
        while(sense.load() != mysense);
    }
    
}
bool tas()
{
    if(flag == false)
    {
        flag = true;
        return true;
    }
    else return false;
}

void Lock::ttasLock()
{
        while(flag.load() == true || tas() == false);
}

void Lock::tasLock()
{
        while(tas() == false);
}

void Lock::ticketLock()
{
    int my_num = next_num.fetch_add(1);
    while(now_serving.load() != my_num);
}

void Lock::unlock()
{
    flag.store(false);
}

void Lock::unlockTicket()
{
    now_serving.fetch_add(1);
}