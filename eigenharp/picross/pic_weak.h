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

#ifndef __PIC_WEAK__
#define __PIC_WEAK__

#include <picross/pic_error.h>
#include <picross/pic_ilist.h>
#include <picross/pic_atomic.h>
#include <picross/pic_ref.h>
#include <picross/pic_flipflop.h>
#include <picross/pic_fastalloc.h>

namespace pic
{
    class PIC_DECLSPEC_INLINE_CLASS tracked_t
    {
        public:
            class PIC_DECLSPEC_INLINE_CLASS tinterlock_t: virtual public pic::atomic_counted_t, virtual public pic::lckobject_t
            {
                public:
                    tinterlock_t(tracked_t *p): flipflop_(p) {}
                    void invalidate() { flipflop_.set(0); }
                    const pic::flipflop_t<tracked_t *> *flipflop() const { return &flipflop_; }
                    bool isvalid() const { return flipflop_.current()!=0; }
                    tracked_t *current() const { return flipflop_.current(); }
                private:
                    pic::flipflop_t<tracked_t *> flipflop_;
            };

        public:
            tracked_t()
            {
            }

            virtual ~tracked_t()
            {
                tracked_invalidate();
            }

            void tracked_invalidate()
            {
                if(ptr_.isvalid()) ptr_->invalidate();
            }

            pic::ref_t<tinterlock_t> interlock() const
            {
                if(!ptr_.isvalid())
                {
                    ptr_=pic::ref(new tinterlock_t((tracked_t *)this));
                }

                return ptr_;
            }

        private:
            mutable pic::ref_t<tinterlock_t> ptr_;
    };

    template <class T> class PIC_DECLSPEC_INLINE_CLASS weak_t
    {
        public:
            class guard_t
            {
                public:
                    guard_t(const weak_t &w): guard_(w.interlock()->flipflop()) {}
                    T *value() { return dynamic_cast<T *>(guard_.value()); }
                private:
                    pic::flipflop_t<tracked_t *>::guard_t guard_;
            };
        public:
            weak_t() {}
            weak_t(const weak_t &p): ptr_(p.ptr_) {}
            weak_t &operator=(const weak_t &p) { ptr_=p.ptr_; return *this; }
            weak_t(const T *t) { if(t) ptr_=t->interlock(); else ptr_.clear(); }
            weak_t(const T &t) { ptr_=t.interlock(); }
            void assign(const weak_t &p) { ptr_=p.ptr_; }
            void assign(const T *t) { ptr_=t->interlock(); }
            bool operator==(const weak_t &w) const { return ptr_==w.ptr_; }
            void clear() { ptr_.clear(); }
            pic::ref_t<tracked_t::tinterlock_t> interlock() const { return ptr_; }
            bool isvalid() const { return ptr_.isvalid() && ptr_->isvalid(); }
            bool operator<(const weak_t &w) const { return ptr() < w.ptr(); }
            T *checkedptr() const { PIC_ASSERT(isvalid()); return dynamic_cast<T *>(ptr_->current()); }
            T *ptr() const { if(!ptr_.isvalid()) return 0; return dynamic_cast<T *>(ptr_->current()); }
            T &operator *() const { return *checkedptr(); }
            T *operator->() const { return checkedptr(); }
        private:
            pic::ref_t<tracked_t::tinterlock_t> ptr_;
    };
};

#endif
