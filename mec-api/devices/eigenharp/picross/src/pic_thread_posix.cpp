
/*
 Copyright 2009 Eigenlabs Ltd.  http://www.eigenlabs.com

 This file is part of EigenD.

 EigenD is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 EigenD is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with EigenD.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <picross/pic_thread.h>
#include <picross/pic_log.h>
#include <picross/pic_time.h>
#include <picross/pic_fastalloc.h>

#ifdef PI_MACOSX
#include <mach/task.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif

#include <picross/pic_config.h>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#ifdef PI_LINUX

#define LINUX_REALTIME_BASE 0

#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/mman.h>

void pic_thread_yield(void)
{
    sched_yield();
}

void pic_init_dll_path(void)
{
}

void pic_set_fpu(void)
{
}

void pic_set_foreground(bool rt)
{
}

pic_threadid_t pic_current_threadid(void)
{
    return pthread_self();
}

bool pic_threadid_equal(pic_threadid_t a, pic_threadid_t b)
{
    return pthread_equal(a, b);
}

static int __realtime(int pri)
{
    struct sched_param sp;

    if(pri>=PIC_THREAD_PRIORITY_HIGH)
    {
        sp.sched_priority=LINUX_REALTIME_BASE+(pri==PIC_THREAD_PRIORITY_REALTIME)?19:10;

        if(sched_setscheduler(0,SCHED_FIFO,&sp) == -1)
        {
            perror("realtime");
        }
    }

    return 1;
}

static void __affinity(pthread_t thread, int affinity_mask)
{
	if(affinity_mask==0) return;

	int mask=affinity_mask;
	int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	for(int core_id=0;core_id<num_cores;core_id++)
	{
		if(mask & 0x01)
		{
	   		CPU_SET(core_id, &cpuset);
		}
		mask = mask >> 1;
	}

	if(CPU_COUNT(&cpuset)>0)
	{
		int s;
   		s=pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	    if (s != 0) {
		   printf("pthread_setaffinity_np error %d\n",s);
		}
	}
}

#endif

#ifdef PI_MACOSX

#define MACOS_REALTIME_BASE 48

#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/mach_error.h>
#include <mach/thread_policy.h>
#include <mach/task.h>
#include <mach/task_policy.h>
#include <mach/thread_act.h>
#include <mach/vm_map.h>
#include <mach/thread_act.h>
#include <CoreAudio/HostTime.h>

void pic_init_dll_path(void)
{
}

static void __realtime(pthread_t thread, int pri)
{
    kern_return_t err;

    unsigned pri2 = 1;

    switch(pri)
    {
        case PIC_THREAD_PRIORITY_LOW:       pri2=0; break;
        case PIC_THREAD_PRIORITY_NORMAL:    pri2=1; break;
        case PIC_THREAD_PRIORITY_HIGH:      pri2=10; break;
        case PIC_THREAD_PRIORITY_REALTIME:  pri2=19; break;
    }

    thread_extended_policy_data_t policy_fixed;
    thread_precedence_policy_data_t policy_prec;

    policy_fixed.timeshare = (pri>=PIC_THREAD_PRIORITY_HIGH)?1:0;
    policy_prec.importance = pri2; //MACOS_REALTIME_BASE+pri-parent_priority-1;

    err = thread_policy_set (pthread_mach_thread_np(thread), THREAD_EXTENDED_POLICY, (thread_policy_t)&policy_fixed, THREAD_EXTENDED_POLICY_COUNT);

    if(err != KERN_SUCCESS)
    {
        printf("thread policy set (1) failed\n");
        return;
    }

    err = thread_policy_set (pthread_mach_thread_np(thread), THREAD_PRECEDENCE_POLICY, (thread_policy_t)&policy_prec, THREAD_PRECEDENCE_POLICY_COUNT);

    if(err != KERN_SUCCESS)
    {
        printf("thread policy set (2) failed\n");
        return;
    }
}

#include <fenv.h>
#pragma STDC FENV_ACCESS ON

void pic_set_fpu()
{
    // disable Intel denormal support to stop recursive DSP functions using high CPU for very small numbers
    fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV);
}


void pic_set_foreground(bool rt)
{
    int ret;
    struct task_category_policy tcatpolicy;
#if 0
    struct rlimit lim;
#endif

    tcatpolicy.role = rt?TASK_FOREGROUND_APPLICATION:TASK_BACKGROUND_APPLICATION;

    if ((ret=task_policy_set(mach_task_self(), TASK_CATEGORY_POLICY, (thread_policy_t)&tcatpolicy, TASK_CATEGORY_POLICY_COUNT)) != KERN_SUCCESS)
    {
          printf("task_policy_set() failed.\n");
    }


#if 0
    lim.rlim_cur = RLIM_INFINITY;
    lim.rlim_max = RLIM_INFINITY;

    setrlimit(RLIMIT_CORE,&lim);
#endif

}

void pic_thread_yield()
{
    sched_yield();
}

pic_threadid_t pic_current_threadid()
{
    return pthread_self();
}

bool pic_threadid_equal(pic_threadid_t a, pic_threadid_t b)
{
    return pthread_equal(a, b);
}

#endif

static int __lock_stack()
{
    char stackstuff[16384];
    mlock(stackstuff,sizeof(stackstuff));
    return 1;
}

static void __mask_signals()
{
    sigset_t sigs;
    sigfillset(&sigs);
    pthread_sigmask(SIG_BLOCK,&sigs,0);
}

void pic::thread_t::run__(pic::thread_t *t)
{
    try
    {
        tsd_setcontext(t->context_);
        pic::logger_t::tsd_setlogger(t->logger_);
        pic::nballocator_t::tsd_setnballocator(t->allocator_);

        t->thread_init();
    }
    catch(...)
    {
        t->init_=false;
        t->init_gate_.open();
        return;
    }

    t->init_=true;
    t->init_gate_.open();

    try
    {
        t->thread_main();
    }
    CATCHLOG()

    try
    {
        t->thread_term();
    }
    CATCHLOG()
}

void *pic::thread_t::run3__(void *t_)
{
    pic::thread_t *t = (pic::thread_t *)t_;

    __lock_stack();
    __mask_signals();

#ifdef PI_LINUX
    if(t->realtime_>0)
    {
        __realtime(t->realtime_);
    }
	
	if(t->affinity_mask_>0)
	{
        __affinity(t->id_,t->affinity_mask_);
	}
#endif

    run__(t);

    t->run_gate_.open();
    return 0;
}

bool pic::thread_t::run2__()
{
    int e;

    if(!run_gate_.shut())
    {
        return false;
    }

    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a,1);
    e=pthread_create(&id_,&a,run3__,this);
    pthread_attr_destroy(&a);

    if(e!=0)
    {
        run_gate_.open();
        return false;
    }

#ifdef PI_MACOSX
    __realtime(id_, realtime_);
#endif

    return true;
}

void pic_thread_lck_free(void *ptr, unsigned size)
{
    munlock(ptr,size);
    free(ptr);
}

void *pic_thread_lck_malloc(unsigned size)
{
    void *ptr = valloc(size);
    int r=0;

    if(!ptr || (r=mlock(ptr,size))!=0)
    {
	int en=errno;
        perror("mlock failed");
        printf("mlock failed %d %d %d\n",size, r,en );
        //return 0;
    }

    return ptr;
}

static void interrupt__(int s)
{
    printf("kbd interrupt\n");
    exit(0);
}

void pic_set_interrupt()
{
    signal(SIGINT,interrupt__);
    signal(SIGQUIT,interrupt__);
}

#ifdef PI_LINUX
void pic_set_core_unlimited()
{
}
#endif

#ifdef PI_MACOSX
void pic_set_core_unlimited()
{
    struct rlimit rl;
    if( getrlimit( RLIMIT_CORE, &rl ) != 0 )
        printf( "Failed to get core limit" );
    else
        rl.rlim_cur = RLIM_INFINITY;
        rl.rlim_max = RLIM_INFINITY;

    if( setrlimit( RLIMIT_CORE, &rl ) != 0 )
        printf( "Failed to set core unlimited" );
}
#endif

pic::tsd_t pic::thread_t::genctx__;

pic::mutex_t::mutex_t(bool recursive, bool inheritance)
{
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
#ifdef PI_LINUX
    if(recursive)
    {
        pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE_NP);
    }
    else
    {
        pthread_mutexattr_settype(&a,PTHREAD_MUTEX_ERRORCHECK_NP);
    }

    if(inheritance)
    {
        pthread_mutexattr_setprotocol(&a,PTHREAD_PRIO_INHERIT);
    }
    else
    {
        pthread_mutexattr_setprotocol(&a,PTHREAD_PRIO_NONE);
    }
#endif

#ifdef PI_MACOSX
    if(recursive)
    {
        pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE);
    }
    else
    {
        pthread_mutexattr_settype(&a,PTHREAD_MUTEX_ERRORCHECK);
    }

    if(inheritance)
    {
        pthread_mutexattr_setprotocol(&a,PTHREAD_PRIO_INHERIT);
    }
    else
    {
        pthread_mutexattr_setprotocol(&a,PTHREAD_PRIO_NONE);
    }
#endif

    pthread_mutex_init(&data_,&a);
    pthread_mutexattr_destroy(&a);
    pthread_mutex_unlock(&data_);
}

pic::mutex_t::~mutex_t()
{
    pthread_mutex_destroy(&data_);
}

void pic::mutex_t::lock() { PIC_ASSERT(pthread_mutex_lock(&data_)==0); }
void pic::mutex_t::unlock() { PIC_ASSERT(pthread_mutex_unlock(&data_)==0); }
bool pic::mutex_t::trylock() { return (pthread_mutex_trylock(&data_)==0); }

pic::thread_t::thread_t(int realtime,int affinity_mask)
{
    realtime_=realtime;
	affinity_mask_=affinity_mask;
    run_gate_.open();
    init_gate_.open();
}

pic::thread_t::~thread_t()
{
    run_gate_.untimedpass();
}

void pic::thread_t::run()
{
    PIC_ASSERT(init_gate_.shut());
    context_ = tsd_getcontext();
    logger_ = pic::logger_t::tsd_getlogger();
    allocator_ = pic::nballocator_t::tsd_getnballocator();

    PIC_ASSERT(run2__());

    init_gate_.untimedpass();

    if(!init_)
    {
        run_gate_.untimedpass();
        PIC_THROW("thread didn't initialise");
    }
}

void pic::thread_t::wait()
{
    run_gate_.untimedpass();
}

bool pic::thread_t::isrunning()
{
    return !run_gate_.isopen();
}

#ifdef PI_MACOSX

pic::semaphore_t::semaphore_t()
{
    kern_return_t kr;

    if((kr = semaphore_create(mach_task_self(),&sem_,SYNC_POLICY_FIFO,0)) != KERN_SUCCESS)
    {
        PIC_THROW("cant create semaphore");
    }
}

pic::semaphore_t::~semaphore_t()
{
    semaphore_destroy(mach_task_self(),sem_);
}

void pic::semaphore_t::up()
{
    semaphore_signal(sem_);
}

bool pic::semaphore_t::untimeddown()
{
    kern_return_t kr = semaphore_wait(sem_);

    if(kr==KERN_SUCCESS)
    {
        return true;
    }

    printf("sem wait returned %u\n",kr);
    return false;
}

bool pic::semaphore_t::timeddown(unsigned long long t)
{
    kern_return_t kr;
    mach_timespec_t delay = { static_cast<unsigned int>(t/1000000ULL), static_cast<clock_res_t>(1000ULL*(t%1000000ULL)) };

    kr = semaphore_timedwait(sem_,delay);

    if(kr==KERN_SUCCESS)
    {
        return true;
    }

    if(kr==KERN_OPERATION_TIMED_OUT)
    {
        return false;
    }

    printf("sem wait returned %u\n",kr);
    return false;
}

#else

pic::semaphore_t::semaphore_t()
{
    if(sem_init(&sem_,0,0)<0)
    {
        PIC_THROW("cant create semaphore");
    }
}

pic::semaphore_t::~semaphore_t()
{
    sem_destroy(&sem_);
}

void pic::semaphore_t::up()
{
    sem_post(&sem_);
}

bool pic::semaphore_t::untimeddown()
{
    int ret = sem_wait(&sem_);
    if(ret==0)
    {
        return true;
    }

    perror("sem_wait");
    return false;
}

bool pic::semaphore_t::timeddown(unsigned long long t)
{
    struct timespec ts = { t/1000000ULL, 1000ULL*(t%1000000ULL) };

    clock_gettime(CLOCK_REALTIME, &ts);

    t += (ts.tv_nsec/1000ULL);
    ts.tv_nsec = 0;
    ts.tv_sec += t/1000000ULL;
    ts.tv_nsec += 1000ULL*(t%1000000ULL);

    int ret = sem_timedwait(&sem_,&ts);

    if(ret==0)
    {
        return true;
    }
    if(errno==ETIMEDOUT)
    {
        return false;
    }

    perror("sem_timedwait");
    return false;
}

#endif

pic::xgate_t::xgate_t(): flag_(0)
{
}

void pic::xgate_t::open()
{
    if(pic_atomicinc(&flag_)==1)
    {
        sem_.up();
    }
}

unsigned pic::xgate_t::pass_and_shut_timed(unsigned long long t)
{
    for(;;)
    {
        for(;;)
        {
            pic_atomic_t oflag = flag_;

            if(pic_atomiccas(&flag_,oflag,0))
            {
                if(oflag)
                {
                    return oflag;
                }
                else
                {
                    break;
                }
            }
        }

        if(!sem_.timeddown(t))
        {
            return 0;
        }
    }
    return 0;
}

unsigned pic::xgate_t::pass_and_shut()
{
    for(;;)
    {
        for(;;)
        {
            pic_atomic_t oflag = flag_;

            if(pic_atomiccas(&flag_,oflag,0))
            {
                if(oflag)
                {
                    return oflag;
                }
                else
                {
                    break;
                }
            }
        }

        sem_.untimeddown();
    }
    return 0;
}

pic::gate_t::gate_t()
{
    pthread_mutex_init(&m,0);
    pthread_mutex_unlock(&m);
    pthread_cond_init(&c,0);
    f=0;
}

pic::gate_t::~gate_t()
{
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&c);
}

bool pic::gate_t::open()
{
    int o;

    pthread_mutex_lock(&m);

    o=f;
    f=1;

    if(o==0) pthread_cond_broadcast(&c);

    pthread_mutex_unlock(&m);

    return (o==0);
}

bool pic::gate_t::shut()
{
    int o;

    pthread_mutex_lock(&m);

    o=f;
    f=0;

    pthread_mutex_unlock(&m);

    return (o==1);
}

void pic::gate_t::untimedpass()
{
    pthread_mutex_lock(&m);

    while(!f)
    {
        pthread_cond_wait(&c,&m);
    }

    pthread_mutex_unlock(&m);
}

bool pic::gate_t::timedpass(unsigned long long timeout)
{
    struct timespec nano;
    struct timeval micro;
    int rv;

    unsigned long long timeout_s = timeout/1000000ULL;
    unsigned long long timeout_us = timeout%1000000ULL;

    gettimeofday(&micro,0);

    nano.tv_sec = micro.tv_sec + timeout_s;
    nano.tv_nsec = (micro.tv_usec+timeout_us)*1000;

    while(nano.tv_nsec >= 1000000000)
    {
        nano.tv_sec += 1;
        nano.tv_nsec -= 1000000000;
    }

    pthread_mutex_lock(&m);

    while(!f)
    {
        rv=pthread_cond_timedwait(&c,&m,&nano);

        if(rv == ETIMEDOUT)
        {
            pthread_mutex_unlock(&m);
            return false;
        }
    }

    pthread_mutex_unlock(&m);
    return true;
}

bool pic::gate_t::isopen()
{
    return f;
}

void pic::rwmutex_t::wlock() { PIC_ASSERT(pthread_rwlock_wrlock(&data_)==0); }
void pic::rwmutex_t::wunlock() { PIC_ASSERT(pthread_rwlock_unlock(&data_)==0); }
bool pic::rwmutex_t::trywlock() { return (pthread_rwlock_trywrlock(&data_)==0); }
void pic::rwmutex_t::rlock() { PIC_ASSERT(pthread_rwlock_rdlock(&data_)==0); }
void pic::rwmutex_t::runlock() { PIC_ASSERT(pthread_rwlock_unlock(&data_)==0); }
bool pic::rwmutex_t::tryrlock() { return (pthread_rwlock_tryrdlock(&data_)==0); }

pic::rwmutex_t::rwmutex_t()
{
    pthread_rwlock_init(&data_,0);
    //pthread_rwlock_unlock(&data_);
}

pic::rwmutex_t::~rwmutex_t()
{
    pthread_rwlock_destroy(&data_);
}

