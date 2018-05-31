#ifndef _WIN32

#include <unistd.h>

#ifdef __COBALT__
#include <xenomai/init.h>
#endif

#else

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

inline void usleep(unsigned int u) { Sleep(u / 1000); }
inline void sleep(unsigned int seconds) { Sleep(seconds * 1000); }

#endif

#include <signal.h>
#include <string.h>

#include "mec_app.h"
#include <mec_prefs.h>

#ifndef __COBALT__

std::condition_variable waitCond;
std::mutex waitMtx;

mecAppLock::mecAppLock() : lock_(waitMtx) {
}
mecAppLock:: ~mecAppLock() {
}

void mec_initAppLock() {
}

void mec_waitFor(mecAppLock& lock,  unsigned mSec) {
    waitCond.wait_for(lock.lock(), std::chrono::milliseconds(mSec));
}
void mec_notifyAll() {
    waitCond.notify_all();
}
#else

pthread_mutex_t waitMtx;
pthread_cond_t waitCond;

void mec_initAppLock() {
    pthread_mutex_init(&waitMtx,0);
    pthread_cond_init(&waitCond,0);
}

void mec_waitFor(mecAppLock& lock,  unsigned mSec) {
    
	unsigned long long t = mSec * 1000;
    struct timespec ts = { t/1000000ULL, 1000ULL*(t%1000000ULL) };

    clock_gettime(CLOCK_REALTIME, &ts);

    t += (ts.tv_nsec/1000ULL);
    ts.tv_nsec = 0;
    ts.tv_sec += t/1000000ULL;
    ts.tv_nsec += 1000ULL*(t%1000000ULL);

    pthread_cond_timedwait(&waitCond,&waitMtx,&ts);
}
void mec_notifyAll() {
    pthread_cond_broadcast(&waitCond);
}

mecAppLock::mecAppLock() {
    pthread_mutex_lock(&waitMtx);
}
mecAppLock:: ~mecAppLock() {
    pthread_mutex_unlock(&waitMtx);
}
#endif


volatile bool keepRunning = true;

void exitHandler() {
    LOG_0("mec_app exit handler called");
}

void intHandler(int sig) {
    // only called in main thread
    if (sig == SIGINT) {
        LOG_0("mec_app int handler called");
        keepRunning = 0;
        mec_notifyAll();
    }
}

#include <mec_msg_queue.h>
//#include <mec_log.h>


int main(int ac, char **av) {
    atexit(exitHandler);

#ifndef WIN32
    // block sigint from other threads
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
#endif


#ifdef __COBALT__
    {
        int argc = 0;
        char *const *argv;
        xenomai_init(&argc, &argv);
    }
#endif


    LOG_0("mec_app initialise ");

    const char *pref_file = "./mec.json";
    if (ac > 1) pref_file = av[1];
    mec::Preferences prefs(pref_file);
    if (!prefs.valid()) return -1;

    keepRunning = true;
    LOG_0("loaded preferences");
    prefs.print();

    std::thread mec_thread;
    if (prefs.exists("mec") && prefs.exists("mec-app")) {
        LOG_1("mec api initialise ");
#ifdef __COBALT__
        pthread_t ph = mec_thread.native_handle();
        pthread_create(&ph, 0,mecapi_proc,prefs.getTree());
#else
        mec_thread = std::thread(mecapi_proc, prefs.getTree());
#endif
        usleep(1000);
    }

#ifndef WIN32
    // Install the signal handler for SIGINT.
    struct sigaction s;
    s.sa_handler = intHandler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);
    // Restore the old signal mask only for this thread.
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
#endif

    {
        mecAppLock lock;
        LOG_0("mec_app running ");
        while (keepRunning) {
            mec_waitFor(lock, 5000);
        }

    }

    LOG_0("mec_app wait for mec thread ");
    if (mec_thread.joinable()) {
        mec_thread.join();
    }
    LOG_0("mec_app mec thread stopped ");

#ifndef WIN32
    // been told to stop, block SIGINT, to allow clean termination
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
#endif

    sleep(5);
    LOG_0("mec_app exit ");


    exit(0);
//    return 0;
}
