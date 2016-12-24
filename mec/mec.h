#ifndef MEC_H
#define MEC_H

#include <pthread.h>

void *eigenharp_proc(void *);
void *osc_command_proc(void *);
void *soundplane_proc(void *);
void *midi_proc(void *);
void *push2_proc(void *);

void getWaitTime(struct timespec& ts, int t);

// define to null to omit logging
#define LOG_0(x) x
// #define LOG_0(x) 
#define LOG_1(x) x
// #define LOG_1(x) 
// #define LOG_2(x) x
#define LOG_2(x) 


extern pthread_cond_t  waitCond;
extern pthread_mutex_t waitMtx;
extern volatile int keepRunning;

#endif
