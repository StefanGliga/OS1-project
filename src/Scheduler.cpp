#include "../h/Scheduler.hpp"
#include "../h/syscall_c.h"

Scheduler::TCB* Scheduler::Running = nullptr;
bool            Scheduler::done    = false; 

Scheduler::Scheduler()
{
    Scheduler::done = false;
    // Running se ne inicijalizuje ovde, zbog reda operacija mora vec biti inicijalizovan pre ulaska u prekid

    ready_head   = nullptr;
    ready_tail   = nullptr;
    waiting_list = nullptr;
    idle_thread  = nullptr;
    num_descheduled = 0;
}

Scheduler& Scheduler::get()
{
    static Scheduler instance;
    return instance;
}

void thread_start_helper(void(*start_routine)(void*), void* arg)
{
    start_routine(arg);
    thread_exit();
}

int Scheduler::create_thread(void* handle, void (*start_routine)(void*), void* arg, void* stack_start)
{
    TCB* self = TCB::construct();
    if(self == nullptr) return -1;

    self->ctx.pc  = reinterpret_cast<uint64>(thread_start_helper);
    self->ctx.x10 = reinterpret_cast<uint64>(start_routine);
    self->ctx.x11 = reinterpret_cast<uint64>(arg);
    self->ctx.x02 = reinterpret_cast<uint64>(((char*)stack_start)+DEFAULT_STACK_SIZE);
    self->stack_start = reinterpret_cast<char*>(stack_start);
    self->join_list = nullptr;

    if(handle)
    {
        *reinterpret_cast<TCB**>(handle) = self;
    }

    auto node = TCB_LL::construct(self, nullptr, DEFAULT_TIME_SLICE);
    if(node == nullptr)
    {
        Allocator::get().free((char*)self);
        return -1;
    }    
    if(ready_head == nullptr)
    {
        ready_head = ready_tail = node;
    }
    else
    {
        if(ready_tail->next != nullptr) panic("Ready tail nije na kraju!");
        ready_tail->next = node;
        ready_tail = ready_tail->next;
    }


    return 0;
}

void userMain();

void Scheduler::first_time_setup()
{
    idle_thread = Running;

    void* stack = Allocator::get().allocate((DEFAULT_STACK_SIZE+MEM_BLOCK_SIZE-1)/MEM_BLOCK_SIZE);

    int res = create_thread(nullptr, reinterpret_cast<void(*)(void*)>(userMain), nullptr, stack);

    asm("csrrw x31, sstatus, x31");
    asm("and x31, x31, 0xFFFFFFFFFFFFFEFF");
    asm("csrrw x31, sstatus, x31");

    if(res == -1)
    {
        panic("PUKLO STVARANJE POCETNE NITI");
    }
}


void Scheduler::dispatch()
{
    if(Running == idle_thread)
    {
        if(ready_head == nullptr)
            return;

        Running = ready_head->data;
        return;
    }

    if(ready_head == nullptr or ready_tail == nullptr) panic("READY NIJE U KONZISTENTNOM STANJU - DISPATCH");
    if(ready_tail->next != nullptr) panic("Ready tail nije na kraju!");

    ready_head->extra_data = DEFAULT_TIME_SLICE;
    ready_tail->next = ready_head;
    ready_head = ready_head->next;
    ready_tail = ready_tail->next;
    ready_tail->next = nullptr;

    Running = ready_head->data;
}

void Scheduler::destroy_thread()
{
    if(Running == idle_thread) panic("POKUSANO UNISTENJE SPECIJALNE NITI");
    if(ready_head == nullptr or ready_tail == nullptr) panic("READY NIJE U KONZISTENTNOM STANJU - DESTROY");

    TCB_LL* tmp = ready_head;
    ready_head = ready_head->next;
    if(ready_head == nullptr)
    {
        ready_tail = nullptr;
        Running = idle_thread;
    } 

    // drain join list
    if(ready_tail and ready_tail->next != nullptr) panic("Ready tail nije na kraju!");
    while(tmp->data->join_list)
    {
        auto tmp2 = tmp->data->join_list;
        tmp->data->join_list = tmp->data->join_list->next;
        reschedule(tmp2);
    } 

    tmp->destroy();
    if(ready_head)
        Running = ready_head->data;
    else
    {
        Running = idle_thread;
        if(waiting_list == nullptr and num_descheduled == 0)
        {
            done = true;


            asm("csrrw x31, sstatus, x31");
            asm("or x31, x31, 0x100");
            asm("csrrw x31, sstatus, x31");
        }
            
    }
}



void Scheduler::sleep_current(uint quants)
{
    if(Running == idle_thread)
        panic("POKUSANO USPAVLJIVANJE SPECIJALNE NITI");

    if(waiting_list == nullptr)
    {
        waiting_list = ready_head;
        ready_head = ready_head->next;
        waiting_list->next = nullptr;
        if(ready_head)
            Running = ready_head->data;
        else
            Running = idle_thread;

        waiting_list->extra_data = quants;
        return;
    }

    TCB_LL* prev = nullptr;
    TCB_LL* curr = waiting_list;
    TCB_LL* tmp = ready_head;
    ready_head = ready_head->next;
    tmp->next = nullptr;

    
    if(ready_head)
        Running = ready_head->data;
    else
        Running = idle_thread;

    while(curr and quants>=curr->extra_data)
    {
        prev = curr;
        quants -= curr->extra_data;
        curr = curr->next;
    }
    if(prev)
    {
        prev->next = tmp;
    }
    else
    {
        waiting_list = tmp;
        curr->extra_data -= quants;
    }
    tmp->next = curr;
    tmp->extra_data = quants;


}

void Scheduler::join_thread(TCB* th)
{
    if(Running == idle_thread) panic("POKUSANO JOIN-OVANJE SPECIJALNE NITI");
    if(ready_head == nullptr or ready_tail == nullptr) panic("READY NIJE U KONZISTENTNOM STANJU - JOIN");

    TCB_LL* tmp = ready_head;
    ready_head = ready_head->next;
    num_descheduled++;

    if(ready_head)
        Running = ready_head->data;
    else
        Running = idle_thread;


    tmp->extra_data = DEFAULT_TIME_SLICE;
    tmp->next = th->join_list;
    th->join_list = tmp;
}

void Scheduler::time_tick()
{
    if(Running != idle_thread)
    {
        if(ready_head == nullptr or ready_tail == nullptr) panic("READY NIJE U KONZISTENTNOM STANJU - TICK");
        if(ready_tail->next != nullptr) panic("Ready tail nije na kraju!");

        ready_head->extra_data--;
    }

    // ceo ovaj kod mi tako smrdi jer na duze vreme lomi invarijantu da Running==idle_thread -> ready_head == nullptr

    // LMAO na kraju nije ni to bilo problematicno nego bezulsovno dekrementiranje vremena ako postoji cvor, sto ne radi ako se sleep-uje na 0
    // HACK: sleep(0) == yield, ako bih isao kroz ovu strukturu sleep(0)==sleep(1) sto nije divno
    
    if(waiting_list)
    {
        waiting_list->extra_data--;
        while(waiting_list and waiting_list->extra_data == 0)
        {
            waiting_list->extra_data = DEFAULT_TIME_SLICE;
            if(ready_head == nullptr)
            {
                ready_head = ready_tail = waiting_list;
                waiting_list = waiting_list->next;
                ready_head->next = nullptr;
            }
            else
            {
                ready_tail->next = waiting_list;
                waiting_list = waiting_list->next;
                ready_tail = ready_tail->next;
                ready_tail->next = nullptr;
            }
        }
    }
    
        


    if(ready_head)
    {
        if(ready_head->extra_data == 0 or Running == idle_thread)
            dispatch();
    }
}

Scheduler::TCB_LL* Scheduler::deschedule_current()
{
    if(Running == idle_thread) panic("POKUSANO BLOKIRANJE SPECIJALNE NITI");
    if(ready_head == nullptr or ready_tail == nullptr) panic("READY NIJE U KONZISTENTNOM STANJU - DESCHED");

    num_descheduled++;
    TCB_LL* ret = ready_head;
    ready_head = ready_head->next;
    ret->next = nullptr;
    if(ready_head == nullptr)
    {
        ready_tail = nullptr;
        Running = idle_thread;
    }
    else
        Running = ready_head->data;

    
    return ret;
}

void Scheduler::reschedule(Scheduler::TCB_LL* th)
{

    num_descheduled--;
    if(num_descheduled<0) panic("NE UKLAPA SE BROJ SKINUTIH NITI");
    if(Running == idle_thread)
    {
        ready_head = ready_tail = th;
        th->extra_data = DEFAULT_TIME_SLICE;
        ready_tail->next = nullptr;
    }
    else
    {
        if(ready_head == nullptr or ready_tail == nullptr) panic("READY NIJE U KONZISTENTNOM STANJU - RESCHED");
        ready_tail->next = th;
        ready_tail = ready_tail->next;
        th->extra_data = DEFAULT_TIME_SLICE;
        ready_tail->next = nullptr;
    }

    Running = ready_head->data;
}
