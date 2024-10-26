//
// Created by stefangliga on 28/07/23.
//

#include "../h/syscall_c.h"
#include "../h/util.hpp"
#include "../h/syscall_cpp.hpp"

class mpt : public PeriodicThread
{
public:
    mpt(time_t t): PeriodicThread(t) {}

    void periodicActivation () override
    {
        putc('P');
    }
};

int userMain()
{
    time_t per = 10;
    mpt a(per);
    a.start();
    time_sleep(100);
    a.terminate();
    putc('m');

    return 0;
}

