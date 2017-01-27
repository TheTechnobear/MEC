
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
#include <picross/pic_resources.h>
#include <picross/pic_config.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <io.h>
#include <time.h>
#include <sys/types.h>
#include <xmmintrin.h>

#define _VALUE_MAX ((int) ((~0u) >> 1))

void pic_thread_yield(void)
{
    Sleep(0);
}

void pic_set_fpu(void)
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
}

void pic_init_dll_path(void)
{
   SetDllDirectoryA(pic::private_exe_dir().c_str());
}

void pic_set_foreground(bool rt)
{
    if(rt)
    {
        if (SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS) == 0 )
        {
            printf( "Failed to set thread priority to real time\n"); 
        }
    }
}

pic_threadid_t pic_current_threadid(void)
{
    return GetCurrentThreadId();
}

bool pic_threadid_equal(pic_threadid_t a, pic_threadid_t b)
{
    return a==b;
}

static int __lock_stack()
{
    char stackstuff[16384];
    VirtualLock(stackstuff,sizeof(stackstuff)); //mlock(stackstuff,sizeof(stackstuff));
    return 1;
}

static int __realtime(int pri)
{
    unsigned pri2 = THREAD_PRIORITY_NORMAL;

    switch(pri)
    {
        case PIC_THREAD_PRIORITY_LOW:       pri2=THREAD_PRIORITY_BELOW_NORMAL; break;
        case PIC_THREAD_PRIORITY_NORMAL:    pri2=THREAD_PRIORITY_NORMAL; break;
        case PIC_THREAD_PRIORITY_HIGH:      pri2=THREAD_PRIORITY_ABOVE_NORMAL; break;
        case PIC_THREAD_PRIORITY_REALTIME:  pri2=THREAD_PRIORITY_TIME_CRITICAL; break;
    }

    if (SetThreadPriority(GetCurrentThread(),pri2) == 0 )
    {
        printf( "Failed to set thread priority to real time" ); 
    }

    if(pri==PIC_THREAD_PRIORITY_REALTIME)
    {
        __lock_stack();
    }

    return 1;
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

DWORD pic::thread_t::run3__(LPVOID t_)
{
    pic::thread_t *t = (pic::thread_t *)t_;

    __realtime(t->realtime_);
    run__(t);

    t->run_gate_.open();
    return 0;
}

bool pic::thread_t::run2__()
{
    if(!run_gate_.shut())
    {
        return false;
    }

    handle_ = CreateThread(NULL,0,run3__,this,0,&id_);

    if(handle_==NULL)
    {
        run_gate_.open();
        return false;
    }

    return true;
}

void pic_thread_lck_free(void *ptr, unsigned size)
{ 
	VirtualUnlock(ptr,size);	//  munlock(ptr,size);
	_aligned_free( ptr );
}

void *pic_thread_lck_malloc(unsigned size)
{
   
	//void *ptr = malloc( size );
	//void *ptr = VirtualAlloc( NULL,size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE );
	SYSTEM_INFO sys_info;
	GetSystemInfo( &sys_info );
	void *ptr = _aligned_malloc( size,sys_info.dwPageSize );

	// Prepare for locking. - NEED TO SORT QUOTA
	DWORD pid = GetCurrentProcessId();
	DWORD sz_min_curr, sz_max_curr;
	unsigned long min,max;
	unsigned long inc = 1024 * 1024 * 20;
	
	//printf( "allocating mem block: %d processID: %d",size,pid );
	if (ptr != NULL)
	{
		while( true )
		{
			int ret = VirtualLock(ptr,size);

			if(!ret)
			{
				unsigned long	error;
				char	Buffer[512];

				//error = GetLastError();
				//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,error,LANG_NEUTRAL,(LPSTR)Buffer,256,NULL);
				//printf("mlock failed %s\n",Buffer);
				
				HANDLE hProcess = 0;
				hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

				if( hProcess == INVALID_HANDLE_VALUE )
				{
					printf( "INVALID handler processid: %d \n",pid );
				}

				if( ! GetProcessWorkingSetSize( hProcess,&sz_min_curr,&sz_max_curr ) )
				{
					error = GetLastError();
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,error,LANG_NEUTRAL,(LPSTR)Buffer,256,NULL);
					printf("mlock failed %s\n",Buffer);
				}

				min = sz_min_curr + inc; // 20 MB
				max = sz_max_curr + inc; // 20 MB

                BOOL result = SetProcessWorkingSetSize(hProcess,min,max); 

                if( !result )
                {
                    error = GetLastError();
                    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,error,LANG_NEUTRAL,(LPSTR)Buffer,256,NULL);
                    printf( "Failed to set process working set size %s\n",Buffer );
                }
				
				CloseHandle( hProcess );
			} 
			else
			{
				//printf("Virtual lock succeeded \n");
				break;
			}
			
		}
	}
	else
	{
		printf( "insufficient memory!!! \n" );
	}


    return ptr;
}

void pic_set_interrupt()
{
}

void pic_set_core_unlimited()
{
}

pic::tsd_t pic::thread_t::genctx__;

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

pic::semaphore_t::semaphore_t()
{
    sem_ = CreateSemaphore(0,0,_VALUE_MAX,0);
}

pic::semaphore_t::~semaphore_t()
{
    CloseHandle(sem_);
}

void pic::semaphore_t::up()
{
    ReleaseSemaphore(sem_,1,0);
}

bool pic::semaphore_t::untimeddown()
{
    switch(WaitForSingleObject(sem_,INFINITE))
    {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_ABANDONED:
            printf("semaphore_t::untimeddown() Error = %d\n",GetLastError());
            return false;
        case WAIT_TIMEOUT:
            return false;
    }
    
    return false;
}

bool pic::semaphore_t::timeddown(unsigned long long t)
{
    t /= 1000;
    
	if (t<1)
		t = 1;

    switch(WaitForSingleObject(sem_,t))
    {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_ABANDONED:
            printf("semaphore_t::timeddown() Error = %d\n",GetLastError());
            return false;
        case WAIT_TIMEOUT:
            return false;
    }
    
    return false;
}

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
}

pic::gate_t::gate_t()
{
    InitializeCriticalSection(&lock_);
    event_ = CreateEvent(NULL,TRUE,FALSE,NULL);
    f_=0;
}

pic::gate_t::~gate_t()
{
    DeleteCriticalSection(&lock_);
    CloseHandle(event_);
}

bool pic::gate_t::open()
{
    EnterCriticalSection(&lock_);

    if(f_)
    {
        LeaveCriticalSection(&lock_);
        return false;
    }

    f_=1;
    SetEvent(event_);
    LeaveCriticalSection(&lock_);

    return true;
}

bool pic::gate_t::shut()
{
    EnterCriticalSection(&lock_);

    if(!f_)
    {
        LeaveCriticalSection(&lock_);
        return false;
    }

    f_=0;
    ResetEvent(event_);
    LeaveCriticalSection(&lock_);

    return true;
}

void pic::gate_t::untimedpass()
{
    WaitForSingleObject(event_,INFINITE);
}



bool pic::gate_t::timedpass(unsigned long long timeout)
{
    timeout /= 1000;

    if(timeout<1)
    {
        timeout=1;
    }

    if(WaitForSingleObject(event_,timeout)==WAIT_OBJECT_0)
    {
        return true;
    }

    return false;
}


bool pic::gate_t::isopen()
{
    return f_!=0;
}

pic::mutex_t::mutex_t(bool recursive,bool inheritance) { InitializeCriticalSection(&data_); }
pic::mutex_t::~mutex_t() { DeleteCriticalSection(&data_); }
void pic::mutex_t::lock() { EnterCriticalSection(&data_); }
void pic::mutex_t::unlock() { LeaveCriticalSection(&data_); }
bool pic::mutex_t::trylock() { return TryEnterCriticalSection(&data_)!=0; }

pic::tsd_t::tsd_t(): index_(TlsAlloc())
{
    PIC_ASSERT(index_!=TLS_OUT_OF_INDEXES);
}

pic::tsd_t::~tsd_t()
{
    TlsFree(index_);
}

void *pic::tsd_t::get()
{
    return TlsGetValue(index_);
}

void *pic::tsd_t::set(void *x)
{
    void *o = get();
    TlsSetValue(index_,x);
    return o;
}

pic::rwmutex_t::rwmutex_t()
{
   write_favoured_ = false;
   reader_count_ = 0;
   reader_waiting_ = 0;
   writer_count_ = 0;
   writer_waiting_ = 0;
   
   InitializeCriticalSection(&lock_);

   reader_event_ = CreateEvent(NULL, FALSE, TRUE, NULL);
   writer_event_ = CreateEvent(NULL, FALSE, TRUE, NULL);
}

pic::rwmutex_t::~rwmutex_t(void)
{
   CloseHandle(reader_event_);
   CloseHandle(writer_event_);
   
   DeleteCriticalSection(&lock_);
}

bool pic::rwmutex_t::rlock0(unsigned timeout)
{
   bool wait = false;
   
   do
   {
      EnterCriticalSection(&lock_);

      if(!writer_count_ && (!write_favoured_ || !writer_waiting_))
      {
         if(wait)
         {
            reader_waiting_--;
            wait = false;
         }
         reader_count_++;
      }
      else
      {
         if(!wait)
         {
            reader_waiting_++;
            wait = true;
         }
         ResetEvent(reader_event_);
      }

      LeaveCriticalSection(&lock_);
      
      if(wait)
      {
         if(WaitForSingleObject(reader_event_, timeout) != WAIT_OBJECT_0)
         {
            EnterCriticalSection(&lock_);
            reader_waiting_--;
            SetEvent(reader_event_); SetEvent(writer_event_);
            LeaveCriticalSection(&lock_);
            return false;
         }
      }
      
   } while(wait);
   
   return true;
}

void pic::rwmutex_t::runlock(void)
{
   EnterCriticalSection(&lock_);

   reader_count_--;

   if(write_favoured_)
   {
      if(writer_waiting_)
      {
         SetEvent(writer_event_);
      }
      else if(reader_waiting_)
      {
         SetEvent(reader_event_);
      }
   }
   else
   {
      if(reader_waiting_)
      {
         SetEvent(reader_event_);
      }
      else if(writer_waiting_)
      {
         SetEvent(writer_event_);
      }
   }

   LeaveCriticalSection(&lock_);
}

bool pic::rwmutex_t::wlock0(unsigned timeout)
{
   bool wait = false;
   
   do
   {
      EnterCriticalSection(&lock_);

      if(!reader_count_ && !writer_count_ && (write_favoured_ || !reader_waiting_))
      {
         if(wait)
         {
            writer_waiting_--;
            wait = false;
         }
         writer_count_++;
      }
      else
      {
         if(!wait)
         {
            writer_waiting_++;
            wait = true;
         }
         ResetEvent(writer_event_);
      }

      LeaveCriticalSection(&lock_);

      if(wait)
      {
         if(WaitForSingleObject(writer_event_, timeout) != WAIT_OBJECT_0)
         {
            EnterCriticalSection(&lock_);
            writer_waiting_--;
            SetEvent(reader_event_); SetEvent(writer_event_);
            LeaveCriticalSection(&lock_);
            return false;
         }
      }
         
   } while(wait);
   
   return true;
}

void pic::rwmutex_t::wunlock()
{
   EnterCriticalSection(&lock_);

   writer_count_--;

   if(write_favoured_)
   {
      if(writer_waiting_)
      {
         SetEvent(writer_event_);
      }
      else if(reader_waiting_)
      {
         SetEvent(reader_event_);
      }
   }
   else
   {
      if(reader_waiting_)
      {
         SetEvent(reader_event_);
      }
      else if(writer_waiting_)
      {
         SetEvent(writer_event_);
      }
   }
      
   LeaveCriticalSection(&lock_);
}
