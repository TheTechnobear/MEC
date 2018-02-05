#ifndef MEC_APP_H
#define MEC_APP_H

#include <thread>
#include <mutex>

#include <mec_log.h>

void *osc_command_proc(void *);
void *mecapi_proc(void *);
void *midi_proc(void *);
void *push2_proc(void *);

void getWaitTime(struct timespec& ts, int t);


extern std::condition_variable  waitCond;
extern std::mutex waitMtx;
extern volatile bool keepRunning;

#endif
