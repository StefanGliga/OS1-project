#pragma once

#include "../h/Scheduler.hpp"

class KSemaphore
{
public:
    void wait();
    void post();

    static KSemaphore* construct(uint init)
    {
        auto self = reinterpret_cast<KSemaphore*>(Allocator::get().allocate((sizeof(KSemaphore) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE  ));
        if(self == nullptr) return self;
        self->count = init;
        self->magic = 0x0D15EA5E;
        return self;
    }

    void destroy();

private:
    uint64 count;

    Scheduler::TCB_LL* head;
    Scheduler::TCB_LL* tail;

    uint32 magic;
};