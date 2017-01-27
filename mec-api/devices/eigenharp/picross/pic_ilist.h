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

#ifndef __PIC_ILIST__
#define __PIC_ILIST__

#include <picross/pic_nocopy.h>
#include <picross/pic_error.h>

namespace pic
{
    template <int N=0> class ilistbase_t;

    template <int N=0> class element_t
    {
        private:
            friend class ilistbase_t<N>;

        public:
            element_t(): prev_(0), next_(0)
            {
            }

            element_t(const element_t &): prev_(0), next_(0)
            {
            }

            void reset()
            {
                prev_=0;
                next_=0;
            }

            element_t &operator=(const element_t &)
            {
                return *this;
            }

            ~element_t()
            {
                remove();
            }

            void remove()
            {
                if(prev_)
                {
                    prev_->next_=next_;
                    if(next_) next_->prev_=prev_;
                    prev_=0;
                    next_=0;
                }
            }

            bool ison()
            {
                return prev_!=0;
            }

        private:
            element_t *prev_;
            element_t *next_;
    };

    template <int N> class ilistbase_t: public nocopy_t
    {
        public:
            ilistbase_t()
            {
                head_.prev_=&head_;
                head_.next_=&head_;
            }

            void takeover(ilistbase_t &l)
            {
                while(pop_front())
                {
                }

                if(l.head_.next_!=&(l.head_))
                {
                    head_.prev_=l.head_.prev_;
                    head_.next_=l.head_.next_;
                    head_.prev_->next_=&head_;
                    head_.next_->prev_=&head_;
                    l.head_.prev_=&(l.head_);
                    l.head_.next_=&(l.head_);
                }
            }

            ilistbase_t(ilistbase_t &l)
            {
                if(l.head_.next_==&(l.head_))
                {
                    head_.prev_=&head_;
                    head_.next_=&head_;
                }
                else
                {
                    head_.prev_=l.head_.prev_;
                    head_.next_=l.head_.next_;
                    head_.prev_->next_=&head_;
                    head_.next_->prev_=&head_;
                    l.head_.prev_=&(l.head_);
                    l.head_.next_=&(l.head_);
                }
            }

            void append(element_t<N> *e)
            {
                e->remove();
                e->next_=&head_;
                e->prev_=head_.prev_;
                e->prev_->next_=e;
                e->next_->prev_=e;
            }

            void prepend(element_t<N> *e)
            {
                e->remove();
                e->prev_=&head_;
                e->next_=head_.next_;
                e->prev_->next_=e;
                e->next_->prev_=e;
            }

            void insert(element_t<N> *e, element_t<N> *e2)
            {
                if(!e2)
                {
                    prepend(e);
                    return;
                }

                e->remove();
                e->prev_=e2->prev_;
                e->next_=e2;
                e->prev_->next_=e;
                e->next_->prev_=e;
            }

            void remove(element_t<N> *e)
            {
                e->remove();
            }

            ~ilistbase_t()
            {
                while(pop_front())
                {
                }
            }

            element_t<N> *pop_front()
            {
                element_t<N> *h = head();

                if(h)
                {
                    h->remove();
                }

                return h;
            }

            element_t<N> *head()
            {
                element_t<N> *ret=(head_.next_==&head_)?0:(head_.next_);
                return ret;
            }

            element_t<N> *tail()
            {
                element_t<N> *ret=(head_.prev_==&head_)?0:(head_.prev_);
                return ret;
            }

            element_t<N> *next(element_t<N> *e)
            {
                element_t<N> *ret=(e->next_==&head_)?0:(e->next_);
                return ret;
            }

            element_t<N> *prev(element_t<N> *e)
            {
                element_t<N> *ret=(e->prev_==&head_)?0:(e->prev_);
                return ret;
            }

            bool empty()
            {
                return (head_.next_==&head_)?true:false;
            }

        private:
            element_t<N> head_;
    };

    template <class E, int N=0, bool D=false> struct ilist_t
    {
        public:
            ilist_t() {}
            ilist_t(ilist_t &l): base_(l.base_) {}
            ~ilist_t() { if(D) { E *e; while((e=pop_front())) delete e; } }
            E *prev(E *e) { return static_cast<E *>(base_.prev(e)); }
            E *next(E *e) { return static_cast<E *>(base_.next(e)); }
            E *head() { return static_cast<E *>(base_.head()); }
            E *tail() { return static_cast<E *>(base_.tail()); }
            void insert(E *e, E *e2) { base_.insert(e,e2); }
            void prepend(E *e) { base_.prepend(e); }
            void append(E *e) { base_.append(e); }
            void remove(E *e) { base_.remove(e); }
            void takeover(ilist_t &l) { base_.takeover(l.base_); }
            E *pop_front() { return static_cast<E *>(base_.pop_front()); }
            bool empty() { return base_.empty(); }
        private:
            ilistbase_t<N> base_;
    };
};

#endif
