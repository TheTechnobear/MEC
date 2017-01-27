
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

#include <picross/pic_safeq.h>

class pic::safe_t: public nocopy_t , virtual public lckobject_t
{
    private:
        friend class safeq_t;

        safe_t(void (*cb)(void *,void *,void *,void *), void *c1, void *c2, void *c3, void *c4):
            callback_(cb), ctx1_(c1), ctx2_(c2), ctx3_(c3), ctx4_(c4), next_(0)
        {
        }

        void (*callback_)(void *, void *, void *, void *);
        void *ctx1_;
        void *ctx2_;
        void *ctx3_;
        void *ctx4_;
        safe_t *next_;
};

pic::safeq_t::safeq_t(): head_(0)
{
}

bool pic::safeq_t::add(void (*cb)(void *, void *, void *, void *), void *ctx1, void *ctx2, void *ctx3, void *ctx4)
{
    bool first;

    safe_t *job = new safe_t(cb,ctx1,ctx2,ctx3,ctx4);

    for(;;)
    {
        job->next_ = head_;
        first = (job->next_==0);

        if(pic_atomicptrcas((void *)&head_, job->next_, job))
        {
            break;
        }
    }

    return first;
}

void pic::safeq_t::run()
{
    safe_t *tmp,*flip;
    safe_t * volatile head;

    for(;;)
    {
        head=head_;

        if(pic_atomicptrcas((void *)&head_, (safe_t *)head, 0))
        {
            break;
        }
    }

    if(!head)
    {
        return;
    }

    flip=0;

    while((tmp=(safe_t *)head)!=0)
    {
        head=head->next_;
        tmp->next_=flip;
        flip=tmp;
    }

    while((tmp=flip)!=0)
    {
        flip=flip->next_;

        try
        {
            last_=tmp;
            lastcb_=tmp->callback_;
            lasta1_=tmp->ctx1_;
            lasta2_=tmp->ctx2_;
            lasta3_=tmp->ctx3_;
            lasta4_=tmp->ctx4_;
            tmp->callback_(tmp->ctx1_,tmp->ctx2_,tmp->ctx3_,tmp->ctx4_);
        }
        CATCHLOG()

        delete tmp;
    }
}

pic::safe_worker_t::safe_worker_t(unsigned ping,unsigned priority): thread_t(priority), quit_(false), ping_(ping), pinged_(false)
{
}

void pic::safe_worker_t::quit()
{
    add(__quit,this,0,0,0);
    wait();
}

void pic::safe_worker_t::add(void (*cb)(void *,void *,void *,void *),void *a,void *b,void *c,void *d)
{
    safeq_.add(cb,a,b,c,d);
    gate_.open();
}

void pic::safe_worker_t::thread_main()
{
#ifdef DEBUG_DATA_ATOMICITY
    std::cout << "Started safeq worker thread with ID " << pic_current_threadid() << std::endl;
#endif
 
    while(!quit_)
    {
        if(ping_ && !pinged_)
        {
            if(!gate_.pass_and_shut_timed(1000ULL*ping_))
            {
                if(!ping())
                    pinged_ = true;
            }
        }
        else
        {
            gate_.pass_and_shut();
            pinged_ = false;
        }

        safeq_.run();
    }
}

void pic::safe_worker_t::__quit(void *a, void *b, void *c, void *d)
{
    safe_worker_t *q = (safe_worker_t *)a;
    q->quit_=true;
}

