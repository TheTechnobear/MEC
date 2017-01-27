
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

#ifndef __PIC_SAFEQ__
#define __PIC_SAFEQ__

#include "pic_exports.h"

#include <picross/pic_nocopy.h>
#include <picross/pic_atomic.h>
#include <picross/pic_fastalloc.h>
#include <picross/pic_thread.h>
#include <picross/pic_log.h>

namespace pic
{
    class safe_t;

    class PIC_DECLSPEC_CLASS safeq_t: public nocopy_t, virtual public lckobject_t
    {
        public:
            safeq_t();
            bool add(void (*cb)(void *, void *, void *, void *), void *ctx1, void *ctx2, void *ctx3, void *ctx4);
            void run();

        private:
            safe_t * volatile head_;
            safe_t * last_;
            void (*lastcb_)(void *, void *, void *, void *);
            void *lasta1_;
            void *lasta2_;
            void *lasta3_;
            void *lasta4_;
    };

    class PIC_DECLSPEC_CLASS safe_worker_t: public nocopy_t, public thread_t
    {
        public:
            safe_worker_t(unsigned ping,unsigned priority);
            virtual ~safe_worker_t() {}

            void quit();
            void add(void (*cb)(void *,void *,void *,void *),void *a,void *b,void *c,void *d);
            void thread_main();
            virtual bool ping() { return false; }

        private:
            static void __quit(void *a, void *b, void *c, void *d);

            safeq_t safeq_;
            xgate_t gate_;
            bool quit_;
            unsigned ping_;
            bool pinged_;
    };
}

#endif
