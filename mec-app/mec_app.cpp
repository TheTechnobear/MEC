#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <pthread.h>

// #include <osc/OscOutboundPacketStream.h>
// #include <osc/OscReceivedElements.h>
// #include <osc/OscPacketListener.h>
// #include <ip/UdpSocket.h>

#include "mec_app.h"
#include <mec_prefs.h>

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


volatile bool keepRunning = true;

void exitHandler() {
    LOG_0("mec_app exit handler called" );
}

void intHandler(int sig) {
    // only called in main thread
    if (sig == SIGINT) {
        LOG_0("mec_app int handler called" );
        keepRunning = 0;
        pthread_cond_broadcast(&waitCond);
    }
}

#include <mec_msg_queue.h>
#include <mec_log.h>


int main(int ac, char **av) {
    atexit(exitHandler);

    // block sigint from other threads
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);

    int rc = 0;

    LOG_0("mec_app initialise ");

    mec::Preferences prefs;
    if (!prefs.valid()) return -1;
    keepRunning = true;
    LOG_0("loaded preferences");
    prefs.print();

    if (prefs.exists("mec") && prefs.exists("mec-app")) {
        LOG_1("mec api initialise ");
        pthread_t mec_thread;
        rc = pthread_create(&mec_thread, NULL, mecapi_proc, prefs.getTree());
        if (rc) {
            LOG_0("unabled to create mecapi thread" << rc );
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

    LOG_0("mec_app running ");

    while (keepRunning) {
        pthread_cond_wait(&waitCond, &waitMtx);
    }

    // been told to stop, block SIGINT, to allow clean termination 
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);

    pthread_mutex_unlock(&waitMtx);

    // really we should join threads where to do a nice exit
    LOG_1("mec_app stopping ");
    sleep(5);
    LOG_0("mec_app exit ");


    pthread_cond_destroy(&waitCond);
    pthread_mutex_destroy(&waitMtx);
    exit(0);
    return 0;
}
