#include <unistd.h>
#include <signal.h>
#include <string.h>

// #include <osc/OscOutboundPacketStream.h>
// #include <osc/OscReceivedElements.h>
// #include <osc/OscPacketListener.h>
// #include <ip/UdpSocket.h>

#include "mec_app.h"
#include <mec_prefs.h>

// for signal


std::condition_variable  waitCond;
std::mutex waitMtx;



volatile bool keepRunning = true;

void exitHandler() {
    LOG_0("mec_app exit handler called");
}

void intHandler(int sig) {
    // only called in main thread
    if (sig == SIGINT) {
        LOG_0("mec_app int handler called");
        keepRunning = 0;
        waitCond.notify_all();
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
    int rc = 0;

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
        mec_thread = std::thread(mecapi_proc, prefs.getTree());
        usleep(1000);
    }

#ifndef WIN32
    // Install the signal handler for SIGINT.
    struct sigaction s;
    s.sa_handler = intHandler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);
#endif
    // Restore the old signal mask only for this thread.
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    {
        std::unique_lock<std::mutex> lock(waitMtx);
        LOG_0("mec_app running ");
        while (keepRunning) {
            waitCond.wait_for(lock, std::chrono::milliseconds(1000));
        }

    }

    LOG_0("mec_app wait for mec thread ");
    if(mec_thread.joinable()) {
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
