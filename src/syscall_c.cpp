#include "../h/syscall_c.h"

extern "C"
{

void* mem_alloc (size_t size)
{
    asm("mv a1, %0" : : "r"((size+MEM_BLOCK_SIZE-1) / MEM_BLOCK_SIZE));
    asm("li a0, 0x01");
    register void* ret;
    asm("ecall": "=r"(ret)); 
    return ret;
}
int mem_free (void* ptr)
{
    asm("mv a1, %0" : : "r"(ptr));
    asm("li a0, 0x02");
    register int ret;
    asm("ecall": "=r"(ret));
    return ret;
}


int thread_create (
 thread_t* handle,
 void(*start_routine)(void*),
 void* arg
)
{
    register void* stack = mem_alloc(DEFAULT_STACK_SIZE);
    // Pise "smeštajuči stek te niti u već odvojen prostor na čiju poslednju lokaciju ukazuje stack_space"
    // e sad da li to znaci poslednja memorijska adresa koja pripada steku(tj stack start)
    // ili poslednja lokacija iz perspektive steka tj start bloka odvojenog za stek
    // ja sam shvatio kao ovo drugo, a realno ima vise smisla

    asm("mv a1, %0" : : "r"(handle));
    asm("mv a2, %0" : : "r"(start_routine));
    asm("mv a3, %0" : : "r"(arg));
    asm("mv a4, %0" : : "r"(stack));
    asm("li a0, 0x11");
    register int ret;
    asm("ecall": "=r"(ret));
    return ret;
}
int thread_exit ()
{
    asm("li a0, 0x12");
    register int ret;
    asm("ecall": "=r"(ret));
    return ret;
}
void thread_dispatch ()
{
    asm("li a0, 0x13");
    asm("ecall");
    return;
}
void thread_join (
 thread_t handle
)
{
    asm("mv a1, %0" : : "r"(handle));
    asm("li a0, 0x14");
    asm("ecall");
    return;
}


int sem_open (
 sem_t* handle,
 unsigned init
)
{
    asm("mv a2, %0" : : "r"(init));
    asm("mv a1, %0" : : "r"(handle) : "a1");
    asm("li a0, 0x21");
    register int ret;
    asm("ecall": "=r"(ret));
    return ret;
}
int sem_close (sem_t handle)
{
    asm("mv a1, %0" : : "r"(handle));
    asm("li a0, 0x22");
    register int ret;
    asm("ecall": "=r"(ret));
    return ret;
}
int sem_wait (sem_t id)
{
    asm("mv a1, %0" : : "r"(id));
    asm("li a0, 0x23");
    register int ret;
    asm("ecall": "=r"(ret));
    return ret;
}
int sem_signal (sem_t id)
{
    asm("mv a1, %0" : : "r"(id));
    asm("li a0, 0x24");
    register int ret;
    asm("ecall": "=r"(ret));
    return ret;
}


int time_sleep (time_t dur)
{
    asm("mv a1, %0" : : "r"(dur));
    asm("li a0, 0x31");
    register int ret;
    asm("ecall": "=r"(ret));
    return ret;
}


char getc ()
{
    asm("li a0, 0x41");
    register char ret;
    asm("ecall": "=r"(ret));
    return ret;
}
void putc (char c)
{
    asm("mv a1, %0" : : "r"(c));
    asm("li a0, 0x42");
    asm("ecall");
    return;
}
}