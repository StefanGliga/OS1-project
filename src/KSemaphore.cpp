#include "../h/KSemaphore.hpp"

void KSemaphore::wait()
{
    if(magic != 0x0D15EA5E) panic("NEVALIDAN SEMAFOR");
    Scheduler::Running->ctx.x10 = 0;
    if(count > 0)
    {
        if(head != nullptr or tail != nullptr) panic("SEMAFOR U NEKOZISTENTNOM STANJU - LISTA");
        count--;
    }
    else
    {
        Scheduler::TCB_LL* tmp = Scheduler::get().deschedule_current();
        if(head == nullptr)
        {
            if(tail != nullptr) panic("SEMAFOR U NEKOZISTENTNOM STANJU - LISTA");
            head = tail = tmp;
            tail->next = nullptr;
        }
        else
        {
            tail->next = tmp;
            tail = tail->next;
        }
    }
}

void KSemaphore::post()
{
    if(magic != 0x0D15EA5E) panic("NEVALIDAN SEMAFOR");
    if(head == nullptr)
    {
        if(tail != nullptr) panic("SEMAFOR U NEKOZISTENTNOM STANJU - LISTA");
        count++;
    }
    else
    {
        if(count != 0) panic("SEMAFOR U NEKOZISTENTNOM STANJU - BROJAC");

        Scheduler::TCB_LL* tmp = head;
        head = head->next;
        tmp->next = nullptr;
        if(head == nullptr) tail = nullptr;
        Scheduler::get().reschedule(tmp);
    }
}

void KSemaphore::destroy()
{
    while(head)
    {
        head->data->ctx.x10 = -1;
        auto tmp = head;
        head = head->next;
        Scheduler::get().reschedule(tmp);
    }
    magic = 0;
    Allocator::get().free(reinterpret_cast<char*>(this));
}
