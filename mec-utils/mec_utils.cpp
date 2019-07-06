#include "mec_utils.h"


#ifndef _WIN32
#include <pthread.h>
#include <stdint.h>

void makeThreadRealtime(std::thread& thread) {

    int policy;
    struct sched_param param;

    pthread_getschedparam(thread.native_handle(), &policy, &param);
    param.sched_priority = 95;
    pthread_setschedparam(thread.native_handle(), SCHED_FIFO, &param);

}

#else

void makeThreadRealtime(std::thread& thread) {
    ;
}

#endif


