//
// Created by stefangliga on 27/07/23.
//

#include "../h/util.hpp"
#include "../h/Scheduler.hpp"
#include "../h/syscall_abi_internal.hpp"
#include "../h/IOManager.hpp"

extern "C"
{

static inline __attribute__((always_inline))
void pushcontext()
{
    asm("csrrw x31, sscratch, x31");
    asm("ld x31, _ZN9Scheduler7RunningE");
    asm("sd x1, 0(x31)");
    asm("sd x2, 8(x31)");
    asm("sd x3, 16(x31)");
    asm("sd x4, 24(x31)");
    asm("sd x5, 32(x31)");
    asm("sd x6, 40(x31)");
    asm("sd x7, 48(x31)");
    asm("sd x8, 56(x31)");
    asm("sd x9, 64(x31)");
    asm("sd x10, 72(x31)");
    asm("sd x11, 80(x31)");
    asm("sd x12, 88(x31)");
    asm("sd x13, 96(x31)");
    asm("sd x14, 104(x31)");
    asm("sd x15, 112(x31)");
    asm("sd x16, 120(x31)");
    asm("sd x17, 128(x31)");
    asm("sd x18, 136(x31)");
    asm("sd x19, 144(x31)");
    asm("sd x20, 152(x31)");
    asm("sd x21, 160(x31)");
    asm("sd x22, 168(x31)");
    asm("sd x23, 176(x31)");
    asm("sd x24, 184(x31)");
    asm("sd x25, 192(x31)");
    asm("sd x26, 200(x31)");
    asm("sd x27, 208(x31)");
    asm("sd x28, 216(x31)");
    asm("sd x29, 224(x31)");
    asm("sd x30, 232(x31)");
    asm("csrr x30, sepc" : : : "x30");
    asm("sd x30, 248(x31)");
    asm("mv x30, x31");
    asm("csrrw x31, sscratch, x31");
    asm("sd x31, 240(x30)");
};

static inline __attribute__((always_inline))
void popcontext()
{
    asm("ld x31, _ZN9Scheduler7RunningE");
    asm("ld x1, 0(x31)");
    asm("ld x2, 8(x31)");
    asm("ld x3, 16(x31)");
    asm("ld x4, 24(x31)");
    asm("ld x5, 32(x31)");
    asm("ld x6, 40(x31)");
    asm("ld x7, 48(x31)");
    asm("ld x8, 56(x31)");
    asm("ld x9, 64(x31)");
    asm("ld x10, 72(x31)");
    asm("ld x11, 80(x31)");
    asm("ld x12, 88(x31)");
    asm("ld x13, 96(x31)");
    asm("ld x14, 104(x31)");
    asm("ld x15, 112(x31)");
    asm("ld x16, 120(x31)");
    asm("ld x17, 128(x31)");
    asm("ld x18, 136(x31)");
    asm("ld x19, 144(x31)");
    asm("ld x20, 152(x31)");
    asm("ld x21, 160(x31)");
    asm("ld x22, 168(x31)");
    asm("ld x23, 176(x31)");
    asm("ld x24, 184(x31)");
    asm("ld x25, 192(x31)");
    asm("ld x26, 200(x31)");
    asm("ld x27, 208(x31)");
    asm("ld x28, 216(x31)");
    asm("ld x29, 224(x31)");
    asm("ld x30, 248(x31)");
    asm("csrw sepc, x30");
    asm("ld x30, 232(x31)");
    asm("ld x31, 240(x31)");
};

__attribute__((naked))
[[noreturn]]
void int_helper_syscall_err(uint64 id)
{
    kprintuint(id);
    panic("ZAHTEVAN JE NEPOSTOJECI SISTEMSKI POZIV");
}

__attribute__((naked))
[[noreturn]]
void int_internals()
{
    pushcontext();
    register uint64 opcode asm("a0");
    register uint64 param1 asm("a1");
    register uint64 param2 asm("a2");
    register uint64 param3 asm("a3");
    register uint64 param4 asm("a4");
    register uint64 cause;
    asm("mv %0, a0" : "=r"(opcode));
    asm("mv %0, a1" : "=r"(param1));
    asm("mv %0, a2" : "=r"(param2));
    asm("mv %0, a3" : "=r"(param3));
    asm("mv %0, a4" : "=r"(param4));
    asm("csrr %0, scause" : "=r"(cause));
    Scheduler::Running->ctx.pc += 4;
    
    
    switch(cause)
    {
        case 0: impl__panic("Instruction address misaligned");
        case 1: impl__panic("Instruction access fault");
        case 2: impl__panic("Illegal instruction");
        case 3: impl__panic("Breakpoint");
        case 4: impl__panic("Load address misaligned");
        case 5: impl__panic("Load access fault");
        case 6: impl__panic("Store/AMO address misaligned");
        case 7: impl__panic("Store/AMO access fault");
        case 8: case 9: break;
        case 12: impl__panic("Instruction page fault");
        case 13: impl__panic("Load page fault");
        case 15: impl__panic("Store/AMO page fault");
        default: impl__panic("Unsupported scause");
    }

    if(cause == 8)
    {
        switch(opcode)
        {
            case 0x01: mem_alloc_impl(param1); break;
            case 0x02: mem_free_impl(reinterpret_cast<void*>(param1)); break;
            case 0x11: thread_create_impl(reinterpret_cast<void*>(param1), reinterpret_cast<void (*)(void*)>(param2), reinterpret_cast<void*>(param3), reinterpret_cast<void*>(param4)); break;
            case 0x12: thread_exit_impl(); break;
            case 0x13: thread_dispatch_impl(); break;
            case 0x14: thread_join_impl(reinterpret_cast<thread_t>(param1)); break;
            case 0x21: sem_open_impl(reinterpret_cast<void*>(param1), param2); break;
            case 0x22: sem_close_impl(reinterpret_cast<void*>(param1)); break;
            case 0x23: sem_wait_impl(reinterpret_cast<void*>(param1)); break;
            case 0x24: sem_signal_impl(reinterpret_cast<void*>(param1)); break;
            case 0x31: time_sleep_impl(param1); break;
            case 0x41: getc_impl(); break;
            case 0x42: putc_impl(param1); break;
            default: int_helper_syscall_err(opcode);
        }
    }else 
    if(cause == 9)
    {
        if(opcode == 0x13)
            Scheduler::get().first_time_setup();
        else if(opcode == 0x42)
            IOManager::get().drain_buffer();
        else
            panic("PREKIDI IZ SISTEMSKOG REZIMA NISU PODRZANI");
    }

    popcontext();
    asm("sret");
}


__attribute__((naked))
void int_timer()
{
    pushcontext();
    Scheduler::get().time_tick();
    asm("csrci sip, 0x2");
    popcontext();
    asm("sret");
}

__attribute__((naked))
[[noreturn]]
void int_invalid()
{
    panic("STIGAO JE NEPODRZAN PREKID");
}

__attribute__((naked))
void int_tty()
{
    pushcontext();
    int code = plic_claim();
    IOManager::get().interrupt();
    plic_complete(code);
    popcontext();
    asm("sret");
}
}