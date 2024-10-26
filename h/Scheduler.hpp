#pragma once

#include "../lib/hw.h"
#include "../h/Allocator.hpp"

class Scheduler
{
public:
    struct Context;
    struct TCB;
    struct TCB_LL;

    static TCB* Running;
    static bool done;

    static Scheduler& get();

    void first_time_setup();

    // ove funkcije imaju znacajan preklop u logici
    // da li je trebalo da refaktorisem? Da? Jel' me briga? Pa ne bas. Pre svega TO MORA DA RADI.
    void sleep_current(uint quants);
    void dispatch();
    void join_thread(TCB* th);
    void destroy_thread();
    TCB_LL* deschedule_current(); // for sem
    void reschedule(TCB_LL* th);


    void time_tick();

    int create_thread(void* handle, void(*start_routine)(void*), void* arg, void* stack_start);

private:
    Scheduler();

    // realno je trebalo da samo napravim LL_Queue klasu
    TCB_LL* ready_head; // currently running - except if Running == idle_thread
    TCB_LL* ready_tail;

    TCB_LL* waiting_list;

    TCB* idle_thread;

    int num_descheduled;
};

struct Scheduler::Context
{
    uint64 x01;
    uint64 x02;
    uint64 x03;
    uint64 x04;
    uint64 x05;
    uint64 x06;
    uint64 x07;
    uint64 x08;
    uint64 x09;
    uint64 x10;
    uint64 x11;
    uint64 x12;
    uint64 x13;
    uint64 x14;
    uint64 x15;
    uint64 x16;
    uint64 x17;
    uint64 x18;
    uint64 x19;
    uint64 x20;
    uint64 x21;
    uint64 x22;
    uint64 x23;
    uint64 x24;
    uint64 x25;
    uint64 x26;
    uint64 x27;
    uint64 x28;
    uint64 x29;
    uint64 x30;
    uint64 x31;
    uint64 pc;
};

struct Scheduler::TCB
{
    Scheduler::Context ctx;
    char* stack_start;
    Scheduler::TCB_LL* join_list;
    

    static Scheduler::TCB* construct()
    {
        return reinterpret_cast<TCB*>(Allocator::get().allocate(  (sizeof(TCB)+MEM_BLOCK_SIZE-1) / MEM_BLOCK_SIZE  ));
    }
};

struct Scheduler::TCB_LL
{
    TCB* data;
    TCB_LL* next;
    uint64 extra_data; // ne koristi se svuda, ali mrzi me da pravim dodatne strukture

    TCB_LL(TCB* data = nullptr, TCB_LL* next = nullptr, uint64 extra_data = 0): data{data}, next{next}, extra_data{extra_data} {}

    static Scheduler::TCB_LL* construct(TCB* data = nullptr, TCB_LL* next = nullptr, uint64 extra_data = 0)
    {
        TCB_LL* ptr = reinterpret_cast<TCB_LL*>(Allocator::get().allocate(  (sizeof(TCB_LL)+MEM_BLOCK_SIZE-1) / MEM_BLOCK_SIZE  ));
        if(ptr == nullptr) return ptr;
        ptr->data = data;
        ptr->next = next;
        ptr->extra_data = extra_data;
        return ptr;
    }

    void destroy()
    {
        Allocator::get().free(reinterpret_cast<char*>(data));
        Allocator::get().free(reinterpret_cast<char*>(this));
    }
};
