//
// Created by stefangliga on 12/04/23.
//

#include "../lib/console.h"
#include "../h/util.hpp"

#include "../h/Scheduler.hpp"
#include "../h/syscall_c.h"

int usermain();

extern "C" void* IVT;

int main()
{
    asm("csrw stvec, %0" : : "r"((uint64)&IVT | 1));

    Scheduler::Running = Scheduler::TCB::construct();

    volatile uint64 dummy = 0;
    thread_dispatch();
    while(!Scheduler::done)
    {
        dummy++; // Vrtim ovu nit ako nema ni jedna nit spremna za izvrsavanje a kernel nije gotov
    }

    putc('E'); // ovo se ustvari nikad ne ispisuje, ovo je trik da se pokrene drain_buffer

    return 0;
}