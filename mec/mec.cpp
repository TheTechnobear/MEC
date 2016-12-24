#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <pthread.h>

#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>


#include "mec.h"

#include "mec_prefs.h"


pthread_cond_t  waitCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t waitMtx = PTHREAD_MUTEX_INITIALIZER;


#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
void getWaitTime(struct timespec& ts, int t) {
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;
    t += (ts.tv_nsec / 1000);
    ts.tv_nsec = 0;
    ts.tv_sec += t / 1000000;
    ts.tv_nsec += 1000 * (t % 1000000);
}
#else
void getWaitTime(struct timespec& ts, int t) {
    clock_gettime(CLOCK_REALTIME, &ts);

    t += (ts.tv_nsec / 1000);
    ts.tv_nsec = 0;
    ts.tv_sec += t / 1000000;
    ts.tv_nsec += 1000 * (t % 1000000);
}
#endif


volatile int keepRunning = 1;

void exitHandler() {
    LOG_1(std::cerr  << "exit handler called" << std::endl;)
}

void intHandler(int sig) {
    // only called in main thread
    if (sig == SIGINT) {
        LOG_1(std::cerr  << "int handler called" << std::endl;)
        keepRunning = 0;
        pthread_cond_broadcast(&waitCond);
    }
}



int main(int ac, char **av)
{

    atexit(exitHandler);

    // block sigint from other threads
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);

    int rc = 0;

    LOG_0(std::cout   << "mec initialise " << std::endl;)


    MecPreferences prefs;
    if (!prefs.valid()) return -1;


    if (prefs.exists("osc")) {
        LOG_1(std::cout   << "osc initialise " << std::endl;)
        pthread_t command_thread;
        rc = pthread_create(&command_thread, NULL, osc_command_proc, prefs.getSubTree("osc"));
        if (rc) {
            LOG_1(std::cerr << "unabled to create osc thread" << rc << std::endl;)
            exit(-1);
        }
        usleep(1000);
    }

    if (prefs.exists("eigenharp")) {
        LOG_1(std::cout   << "eigenharp initialise " << std::endl;)
        pthread_t eigen_thread;
        rc = pthread_create(&eigen_thread, NULL, eigenharp_proc, prefs.getSubTree("eigenharp"));
        if (rc) {
            LOG_1(std::cerr << "unabled to create eigen thread" << rc << std::endl;)
            exit(-1);
        }
        usleep(1000);
    }

    if (prefs.exists("soundplane")) {
        LOG_1(std::cout   << "soundplane initialise " << std::endl;)
        pthread_t soundplane_thread;
        rc = pthread_create(&soundplane_thread, NULL, soundplane_proc, prefs.getSubTree("soundplane"));
        if (rc) {
            LOG_1(std::cerr << "unabled to create soundplane thread" << rc << std::endl;)
            exit(-1);
        }
        usleep(1000);
    }

    if (prefs.exists("push2")) {
        LOG_1(std::cout   << "push2 initialise " << std::endl;)
        pthread_t push2_thread;
        rc = pthread_create(&push2_thread, NULL, push2_proc, prefs.getSubTree("push2"));
        if (rc) {
            LOG_1(std::cerr << "unabled to create push2 thread" << rc << std::endl;)
            exit(-1);
        }
        usleep(1000);
    }

    if (prefs.exists("midi")) {
        LOG_1(std::cout   << "midi initialise " << std::endl;)
        pthread_t midi_thread;
        rc = pthread_create(&midi_thread, NULL, midi_proc, prefs.getSubTree("midi"));
        if (rc) {
            LOG_1(std::cerr << "unabled to create midi thread" << rc << std::endl;)
            exit(-1);
        }
        usleep(1000);
    }


    // Install the signal handler for SIGINT.
    struct sigaction s;
    s.sa_handler = intHandler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);

    // Restore the old signal mask only for this thread.
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    pthread_mutex_lock(&waitMtx);

    LOG_0(std::cout   << "mec running " << std::endl;)
    while (keepRunning) {
        pthread_cond_wait(&waitCond, &waitMtx);
    }
    pthread_mutex_unlock(&waitMtx);

    // really we should join threads where to do a nice exit
    LOG_0(std::cout   << "mec stopping " << std::endl;)
    sleep(5);
    LOG_0(std::cout   << "mec exit " << std::endl;)


    pthread_cond_destroy(&waitCond);
    pthread_mutex_destroy(&waitMtx);
    exit(0);
    return 0;
}
