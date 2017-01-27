
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

#include <picross/pic_cfrunloop.h>
#include <picross/pic_log.h>

pic::cfrunloop_t::cfrunloop_t(unsigned realtime): thread_t(realtime), _loop(0)
{
    _gate.open();
}

pic::cfrunloop_t::~cfrunloop_t()
{
    stop();
}

void pic::cfrunloop_t::stop()
{
    if(isrunning())
    {
        CFRunLoopStop(_loop);
    }

    wait();
}

int pic::cfrunloop_t::cmd(void *a1, void *a2)
{
    PIC_ASSERT(_gate.shut());
    _arg1=a1; _arg2=a2; _re=false;
    CFRunLoopTimerSetNextFireDate(_cmd,CFAbsoluteTimeGetCurrent());
    _gate.untimedpass();
    PIC_ASSERT(_re==false);
    return _rv;
}

void pic::cfrunloop_t::cmd2(void *a1, void *a2)
{
    _2arg1=a1; _2arg2=a2;
    CFRunLoopTimerSetNextFireDate(_cmd2,CFAbsoluteTimeGetCurrent());
}

void pic::cfrunloop_t::runloop_init()
{
}

void pic::cfrunloop_t::runloop_start()
{
}

void pic::cfrunloop_t::runloop_term()
{
}

int pic::cfrunloop_t::runloop_cmd(void *, void *)
{
    return 0;
}

void pic::cfrunloop_t::runloop_cmd2(void *, void *)
{
}

void pic::cfrunloop_t::thread_init()
{
    init0();

    try
    {
        runloop_init();
    }
    catch(...)
    {
        term0();
        throw;
    }
}

void pic::cfrunloop_t::thread_main()
{
#ifdef DEBUG_DATA_ATOMICITY
    std::cout << "Started CFRunLoop thread with ID " << pic_current_threadid() << std::endl;
#endif

    try
    {
        runloop_start();

        try
        {
            CFRunLoopRun();
        }
        catch(...)
        {
            try
            {
                runloop_term();
            }
            CATCHLOG()
            throw;
        }

        runloop_term();

    }
    CATCHLOG()
}

void pic::cfrunloop_t::_command2(CFRunLoopTimerRef, void *self)
{
    cfrunloop_t *t = (cfrunloop_t *)self;

    try
    {
        t->runloop_cmd2(t->_2arg1,t->_2arg2);
    }
    catch(...)
    {
    }
}

void pic::cfrunloop_t::_command(CFRunLoopTimerRef, void *self)
{
    cfrunloop_t *t = (cfrunloop_t *)self;

    try
    {
        t->_rv=t->runloop_cmd(t->_arg1, t->_arg2);
        t->_re=false;
    }
    catch(...)
    {
        t->_re=true;
    }

    t->_gate.open();
}


void pic::cfrunloop_t::init0()
{
    CFRunLoopTimerContext ctx; memset(&ctx,0,sizeof(ctx)); ctx.info=this;
    CFRunLoopTimerContext ctx2; memset(&ctx2,0,sizeof(ctx2)); ctx2.info=this;

    _loop=CFRunLoopGetCurrent();
    _cmd=CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent()+1000000000,10000000,0,0,_command,&ctx);
    _cmd2=CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent()+1000000000,10000000,0,0,_command2,&ctx2);

    CFRunLoopAddTimer(_loop,_cmd,kCFRunLoopCommonModes);
    CFRunLoopAddTimer(_loop,_cmd2,kCFRunLoopCommonModes);
}

void pic::cfrunloop_t::term0()
{
    CFRunLoopRemoveTimer(_loop,_cmd,kCFRunLoopCommonModes);
    CFRelease(_cmd);
}
