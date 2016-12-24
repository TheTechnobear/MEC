// ThreadUtility.cpp
//
// Returns raw data frames from the Soundplane.  The frames are reclocked if needed (TODO)
// to reconstruct a steady sample rate.
//
// Two threads are used to do this work.  A grab thread maintains a stream of
// low-latency isochronous transfers. A process thread looks through the buffers
// specified by these transfers every ms or so.  When new frames of data arrive,
// the process thread reclocks them and pushes them to a ring buffer where they
// can be read by clients.

#include "ThreadUtility.h"

#ifdef __APPLE__
#include <mach/mach.h>

void setThreadPriority(pthread_t inThread, uint32_t inPriority, bool inIsFixed)
{
    if (inPriority == 96)
    {
        // REAL-TIME / TIME-CONSTRAINT THREAD
        thread_time_constraint_policy_data_t    theTCPolicy;

        theTCPolicy.period = 1000 * 1000;
        theTCPolicy.computation = 50 * 1000;
        theTCPolicy.constraint = 1000 * 1000;
        theTCPolicy.preemptible = true;
        thread_policy_set (pthread_mach_thread_np(inThread), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&theTCPolicy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
    }
    else
    {
        // OTHER THREADS
        thread_extended_policy_data_t   theFixedPolicy;
        thread_precedence_policy_data_t   thePrecedencePolicy;
        int32_t                relativePriority;

        // [1] SET FIXED / NOT FIXED
        theFixedPolicy.timeshare = !inIsFixed;
        thread_policy_set (pthread_mach_thread_np(inThread), THREAD_EXTENDED_POLICY, (thread_policy_t)&theFixedPolicy, THREAD_EXTENDED_POLICY_COUNT);

        // [2] SET PRECEDENCE
        relativePriority = inPriority;
        thePrecedencePolicy.importance = relativePriority;
        thread_policy_set (pthread_mach_thread_np(inThread), THREAD_PRECEDENCE_POLICY, (thread_policy_t)&thePrecedencePolicy, THREAD_PRECEDENCE_POLICY_COUNT);
    }
}

#else

void setThreadPriority(pthread_t inThread, uint32_t inPriority, bool inIsFixed)
{
    int policy;
    struct sched_param param;

    pthread_getschedparam(inThread, &policy, &param);
    param.sched_priority = sched_get_priority_max(policy);
    pthread_setschedparam(inThread, policy, &param);
}

#endif
