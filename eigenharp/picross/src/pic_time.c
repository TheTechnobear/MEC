
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

#include <picross/pic_time.h>
#include <picross/pic_config.h>


#ifdef PI_WINDOWS

#include <picross/pic_windows.h>

static __int64 __ticks_per_second = 0LL;

void pic_init_time(void)
{
    LARGE_INTEGER l;
    QueryPerformanceFrequency(&l);
    __ticks_per_second = l.QuadPart;
}

unsigned long long pic_microtime()
{
    LARGE_INTEGER ticks;
    __int64 quot, rem;

    if(!__ticks_per_second)
    {
        pic_init_time();
    }

    QueryPerformanceCounter(&ticks);

    quot = ticks.QuadPart/__ticks_per_second;
    rem = ticks.QuadPart%__ticks_per_second;

    return (quot*1000000)+((rem*1000000)/__ticks_per_second);
}

void pic_microsleep(unsigned long micro)
{
	SleepEx(micro/1000,TRUE);
    //nanosleep(&ts,0);
}

void pic_nanosleep(unsigned long nano)
{
	SleepEx(nano/1000000,TRUE);
    //nanosleep(&ts,0);
}



#endif


#if defined(PI_LINUX)

#include <time.h>
#include <sys/time.h>

void pic_init_time(void)
{
}

void pic_microsleep(unsigned long micro)
{
    struct timespec ts;
    ts.tv_sec=micro/1000000;
    ts.tv_nsec=(micro%1000000)*1000;
    nanosleep(&ts,0);
}

void pic_nanosleep(unsigned long nano)
{
    struct timespec ts;
    ts.tv_sec=nano/1000000000;
    ts.tv_nsec=(nano%1000000000);
    nanosleep(&ts,0);
}

unsigned long long pic_microtime()
{
    struct timeval tv;
    unsigned long long now;

    gettimeofday(&tv,0);
    now = 1000000ULL * (unsigned long long)(tv.tv_sec);
    now += (unsigned long long)tv.tv_usec;

    return now;
}

#endif

#if defined(PI_MACOSX)

#include <CoreAudio/HostTime.h>
#include <time.h>

void pic_init_time(void)
{
}

void pic_microsleep(unsigned long micro)
{
    struct timespec ts;
    ts.tv_sec=micro/1000000;
    ts.tv_nsec=(micro%1000000)*1000;
    nanosleep(&ts,0);
}

void pic_nanosleep(unsigned long nano)
{
    struct timespec ts;
    ts.tv_sec=nano/1000000000;
    ts.tv_nsec=(nano%1000000000);
    nanosleep(&ts,0);
}

unsigned long long pic_microtime()
{
    return AudioConvertHostTimeToNanos(AudioGetCurrentHostTime())/1000LL;
}

#endif
