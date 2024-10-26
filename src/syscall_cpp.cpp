//
// Created by stefangliga on 14/08/23.
//

#include "../h/syscall_cpp.hpp"

void* operator new(size_t sz)
{
    return mem_alloc(sz);
}

void operator delete(void* p)
{
    mem_free(p);
}


Thread::Thread(void (* body)(void*), void* arg):body{body}, arg{arg}
{

}

Thread::~Thread()
{
    // treba li ovde join?? verovatno ne?
}

int Thread::start()
{
    return thread_create(&myHandle, body, arg);
}

void Thread::join()
{
    thread_join(myHandle);
}

void Thread::dispatch()
{
    thread_dispatch();
}

int Thread::sleep(time_t t)
{
    return time_sleep(t);
}

void poly_launch_helper(void* arg)
{
    auto* self = static_cast<Thread*>(arg);
    self->run();
}

Thread::Thread()
{
    body = poly_launch_helper;
    arg = this;
}

Semaphore::Semaphore(unsigned int init)
{
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore()
{
    sem_close(myHandle);
}

int Semaphore::wait()
{
    return sem_wait(myHandle);
}

int Semaphore::signal()
{
    return sem_signal(myHandle);
}

void PeriodicThread::terminate()
{
    period = 0;
    this->join();
}

#ifdef periodicActivation_je_spor
void periodic_poly_launch_helper(void* arg)
{
    auto* self = static_cast<PeriodicThread*>(arg);
    self->periodicActivation();
}
#endif

void periodic_launcher(void* arg)
{

    auto* self = static_cast<PeriodicThread*>(arg);
    while(self->period != 0)
    {
#ifndef periodicActivation_je_spor
        self->periodicActivation();
#else
        thread_create(nullptr, periodic_poly_launch_helper, self);
#endif
        time_sleep(self->period);
    }



}

PeriodicThread::PeriodicThread(time_t period): Thread(periodic_launcher, this), period{period}
{

}

char Console::getc()
{
    char c = ::getc();
    return c;
}

void Console::putc(char c)
{
    ::putc(c);
}
