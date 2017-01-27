#ifndef MEC_APP_H
#define MEC_APP_H

#include <pthread.h>
#include <mec_log.h>

void *osc_command_proc(void *);
void *mecapi_proc(void *);
void *midi_proc(void *);
void *push2_proc(void *);

void getWaitTime(struct timespec& ts, int t);

extern pthread_cond_t  waitCond;
extern pthread_mutex_t waitMtx;
extern volatile int keepRunning;

#endif
