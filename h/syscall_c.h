#pragma once

#include "../lib/hw.h"
#include "../h/Scheduler.hpp"
#include "../h/KSemaphore.hpp"

extern "C"
{

void* mem_alloc (size_t size);
int mem_free (void*);

using thread_t = Scheduler::TCB*;
int thread_create (
 thread_t* handle,
 void(*start_routine)(void*),
 void* arg
);

int thread_exit ();

void thread_dispatch ();

void thread_join (
 thread_t handle
);

using sem_t = KSemaphore*;

int sem_open (
 sem_t* handle,
 unsigned init
);

int sem_close (sem_t handle);
int sem_wait (sem_t id);
int sem_signal (sem_t id);

typedef unsigned long time_t;
int time_sleep (time_t);

const int EOF = -1;
char getc ();
void putc (char);

}