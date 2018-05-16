#ifndef _WIN32

#include "KontrolRack.h"

__attribute__((destructor)) void kontrol_uninit(void)
{
    KontrolRack_cleanup();
}

#endif
