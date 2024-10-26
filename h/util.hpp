//
// Created by stefangliga on 01/05/23.
//

#pragma once

#include "../lib/hw.h"

#define DEBUG

#ifdef DEBUG
#define panic(x) impl__panic(x);
#else
#define panic(x) {};
#endif

[[noreturn]]
void impl__panic(char const* msg);

void kprintstr(char const* msg);
void kprintuint(uint32 num);