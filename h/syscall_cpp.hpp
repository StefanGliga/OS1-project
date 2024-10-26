#ifndef _syscall_cpp
#define _syscall_cpp
#include "../h/syscall_c.h"


void* operator new(size_t sz);

void operator delete(void* p);

class Thread {
public:
    Thread (void (*body)(void*), void* arg);
    virtual ~Thread ();
    int start ();
    void join();
    static void dispatch ();
    static int sleep (time_t);
protected:
    Thread ();
    virtual void run () {}
private:
    thread_t myHandle;
    void (*body)(void*); void* arg;

    friend void poly_launch_helper(void* arg);
};

class Semaphore {
public:
    Semaphore (unsigned init = 1);
    virtual ~Semaphore ();
    int wait ();
    int signal ();
private:
    sem_t myHandle;
};

// cini da se periodicActivation lansira na zasebnoj niti
#define periodicActivation_je_spor

class PeriodicThread : public Thread {
public:
    void terminate ();
protected:
    PeriodicThread (time_t period);
    virtual void periodicActivation () {}
private:
    time_t period;

    friend void periodic_launcher(void* arg);
#ifdef periodicActivation_je_spor
    friend void periodic_poly_launch_helper(void* arg);
#endif
};

class Console {
public:
    static char getc ();
    static void putc (char);
};
#endif