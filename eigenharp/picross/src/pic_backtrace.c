
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

#include <picross/pic_backtrace.h>
#include <picross/pic_config.h>

#ifdef PI_LINUX

#include <execinfo.h>
#include <string.h>
#include <signal.h>

void pic_breakpoint(void)
{
    kill(0,19);
}

#if 0

void pic_backtrace(char *buffer, unsigned len)
{
    buffer[0]=0;
}

#else

void pic_backtrace(char *buffer, unsigned len)
{
    void *trace[16];
    char **messages = 0;
    int trace_size,i,p,ml;

    trace_size=backtrace(trace,16);
    messages=backtrace_symbols(trace,trace_size);
    len=len-1;
    p=0;

    for(i=0;i<trace_size;i++)
    {
        if(i>0)
        {
            if(len-p<4) break;
            strcpy(&buffer[p]," <- "); p+=4;
        }

        ml=strlen(messages[i]);
        if(len-p<ml) break;
        strcpy(&buffer[p],messages[i]); p+=ml;
    }

    buffer[p]=0;
}

#endif

#endif

#ifdef PI_MACOSX

#include <signal.h>

void pic_breakpoint(void)
{
    kill(0,18);
}

void pic_backtrace(char *buffer, unsigned len)
{
    buffer[0]=0;
}

#endif

#ifdef PI_WINDOWS

#include <picross/pic_windows.h>

void pic_breakpoint(void)
{
	DebugBreak();
}

void pic_backtrace(char *buffer, unsigned len)
{
    buffer[0]=0;
}

#endif
