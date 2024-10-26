//
// Created by stefangliga on 01/05/23.
//


#include "../h/util.hpp"
#include "../lib/hw.h"

void kputc(char c) // unbuffered, FOR PANICS ONLY
{
    char* status_reg = reinterpret_cast<char*>(CONSOLE_STATUS);
    while(not (*status_reg & CONSOLE_TX_STATUS_BIT))
    {
        ;
    }
    *reinterpret_cast<char*>(CONSOLE_TX_DATA) = c;
}

[[noreturn]]
void impl__panic(char const* const msg)
{
    char const* c = "[PANIC]";
    kprintstr(c);
    kprintstr(msg);
    kputc('\n');
    while(1)
    {
        volatile unsigned i = 0;
        do{
            i++;
        } while(i);
        kputc('!');
    }
}

void kprintstr(const char* msg)
{
    while(msg && *msg != 0)
    {
        kputc(*msg);
        msg++;
    }
}

void kprintuint(uint32 num)
{
    if(num == 0)
    {
        kputc('0');
        return;
    }
    char buf[11];
    char* p = buf;
    while(num)
    {
        char a = num % 10;
        *p = '0' + a;
        num = num / 10;
        p++;
    }
    *p = 0;
    p--;
    while(p >= buf)
    {
        kputc(*p);
        p--;
    }
}

