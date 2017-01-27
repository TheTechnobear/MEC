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

#ifndef __PIC_CFRUNLOOP__
#define __PIC_CFRUNLOOP__

#include "pic_exports.h"
#include "pic_config.h"

#ifdef PI_MACOSX

#include <CoreFoundation/CFRunLoop.h>
#include "pic_thread.h"

namespace pic
{
    class PIC_DECLSPEC_CLASS cfrunloop_t: private thread_t
    {
        public:
            cfrunloop_t(unsigned realtime);
            virtual ~cfrunloop_t();
            void stop();
            int cmd(void *a1, void *a2);
            void cmd2(void *a1, void *a2);

            using thread_t::run;
            using thread_t::isrunning;

        protected:
            virtual void runloop_init();
            virtual void runloop_start();
            virtual void runloop_term();
            virtual int runloop_cmd(void *, void *);
            virtual void runloop_cmd2(void *, void *);
            void thread_init();
            void thread_main();

        private:
            static void _command(CFRunLoopTimerRef, void *self);
            static void _command2(CFRunLoopTimerRef, void *self);

            void init0();
            void term0();

        private:
            CFRunLoopRef _loop;
            CFRunLoopTimerRef _cmd;
            CFRunLoopTimerRef _cmd2;
            void *_arg1, *_arg2;
            void *_2arg1, *_2arg2;
            int _rv; bool _re;
            pic::gate_t _gate;
    };

};

#endif

#endif
