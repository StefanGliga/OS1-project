#pragma once

#include "../lib/hw.h"
#include "../h/syscall_c.h"

extern "C"
{

// Svim funkcijama implementacijama je dodat syscallid/dummy parametar da bi se ustedelo na shiftovanju registara


void mem_alloc_impl (size_t blocks);


void mem_free_impl (void*);



void thread_create_impl (
 void* handle, // ustvari pointer na _thread/TCB
 void(*start_routine)(void*),
 void* arg,
 void* stack_start
);


void thread_exit_impl ();


void thread_dispatch_impl ();


void thread_join_impl (
 thread_t handle
);



void sem_open_impl (
 void* handle, // ustvari pointer na _sem
 unsigned init
);


void sem_close_impl (void* handle);


void sem_wait_impl (void* handle);


void sem_signal_impl (void* handle);



void time_sleep_impl (time_t);



void getc_impl ();


void putc_impl (char);/*  Ispisuje dati znak na konzolu. */

}