#include "../h/syscall_abi_internal.hpp"

#include "../lib/hw.h"
#include "../h/IOManager.hpp"

extern "C"
{

// ceo ovaj fajl je ostatak od kad sam dispatching radio u assembly-ju i kad su sve ovo bile naked funkcije sa popctx()+sret na kraju


void mem_alloc_impl (size_t blocks)
{
    register char* ret = Allocator::get().allocate(blocks);

    Scheduler::Running->ctx.x10 = reinterpret_cast<uint64>(ret);
}

void mem_free_impl (void* ptr)
{
    Allocator::get().free((char*)ptr);
    Scheduler::Running->ctx.x10 = 0;
}


void thread_create_impl (
 void* handle, // ustvari pointer na _thread/TCB
 void(*start_routine)(void*),
 void* arg,
 void* stack_start
)
{
    register int ret = Scheduler::get().create_thread(handle, start_routine, arg, stack_start);
    Scheduler::Running->ctx.x10 = ret;
}

void thread_exit_impl ()
{
    // ne moze da padne pa ne moze da vrati vrednots
    Scheduler::get().destroy_thread();
}

void thread_dispatch_impl ()
{
    Scheduler::get().dispatch();
}

void thread_join_impl (
 thread_t handle
)
{
    Scheduler::get().join_thread(handle);
}


void sem_open_impl (
 void* handle, // ustvari pointer na _sem
 unsigned init
)
{
    if(handle == nullptr)
    {
        Scheduler::Running->ctx.x10 = -1;
        return;
    }
    KSemaphore* sem = KSemaphore::construct(init);
    if(sem == nullptr)
        Scheduler::Running->ctx.x10 = -1;
    else
        Scheduler::Running->ctx.x10 = 0;

    *reinterpret_cast<sem_t*>(handle) = sem;
}

void sem_close_impl (void* handle)
{
    if(handle == nullptr)
    {
        Scheduler::Running->ctx.x10 = -1;
        return;
    }
    KSemaphore& sem = *reinterpret_cast<KSemaphore*>(handle);
    sem.destroy();
    Scheduler::Running->ctx.x10 = 0;
}

void sem_wait_impl (void* handle)
{
    if(handle == nullptr)
    {
        Scheduler::Running->ctx.x10 = -1;
        return;
    }
    KSemaphore& sem = *reinterpret_cast<KSemaphore*>(handle);
    sem.wait(); // povratna vrednost unutra
}

void sem_signal_impl (void* handle)
{
    if(handle == nullptr)
    {
        Scheduler::Running->ctx.x10 = -1;
        return;
    }
    KSemaphore& sem = *reinterpret_cast<KSemaphore*>(handle);
    sem.post();
    Scheduler::Running->ctx.x10 = 0;
}


void time_sleep_impl (time_t time)
{
    Scheduler::Running->ctx.x10 = 0;
    if(time>0)
        Scheduler::get().sleep_current(time);
    else
        Scheduler::get().dispatch();
}


void getc_impl ()
{
    IOManager::get().getc(); // povratak kroz upis u Running->ctx
}

void putc_impl (char c)
{
    IOManager::get().putc(c);
}

}