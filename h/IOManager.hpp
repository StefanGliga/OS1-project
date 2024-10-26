#pragma once

#include "../h/Scheduler.hpp"

class IOManager
{
public:
    static IOManager& get();

    void getc();
    void putc(char c);

    void interrupt();
    void drain_buffer();
    
private:
    class CircularBuffer
    {
    public:
        CircularBuffer(): front{-1}, back{-1} {}
        bool full();
        bool empty();
        void push(char c);
        int pop();
    private:
        constexpr static int SZ = 1024;
        char buf[SZ];
        int front;
        int back;
    };

    IOManager();
    CircularBuffer in_buf;
    CircularBuffer out_buf;

    // tako cu da zazalim sto do sad nisam napravio apstrakciju LL_Queue
    
    Scheduler::TCB_LL* in_q_head;
    Scheduler::TCB_LL* in_q_tail;
    Scheduler::TCB_LL* out_q_head;
    Scheduler::TCB_LL* out_q_tail;
};
