#ifndef MEC_APP_H
#define MEC_APP_H

#include <thread>
#include <mutex>
#include <condition_variable>

#include <mec_log.h>

void *osc_command_proc(void *);
void *mecapi_proc(void *);
void *midi_proc(void *);
void *push2_proc(void *);

void getWaitTime(struct timespec& ts, int t);


extern volatile bool keepRunning;

class mecAppLock {
public:
    mecAppLock();
    ~mecAppLock();
    std::unique_lock<std::mutex>& lock() { return lock_;}
private:
    std::unique_lock<std::mutex> lock_;
};

void mec_initAppLock();
void mec_waitFor(mecAppLock& lock,  unsigned mSec);
void mec_notifyAll();

#endif
