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

#ifndef __PIC_REF__
#define __PIC_REF__

#include <picross/pic_error.h>
#include <picross/pic_atomic.h>

namespace pic
{
    template <class T> class ref_t
    {
        public:
            ~ref_t() { assign(0); }

            ref_t(): ptr_(0) {}
            ref_t(const ref_t &other): ptr_(0) { assign(other.ptr()); }
            ref_t &operator=(const ref_t &other) { assign(other.ptr()); return *this; }

            template <class X> ref_t(const ref_t<X> &r): ptr_(0) { assign(r.ptr()); }
            template <class X> ref_t &operator=(const ref_t<X> &r) { assign(r.ptr()); return *this; }
            template <class X> bool operator==(const ref_t<X> &r) const { return r.ptr()==ptr_; }
            template <class X> bool operator==(const X *r) const { return r==ptr_; }
            template <class X> bool operator<(const ref_t<X> &r) const { return ptr_<r.ptr(); }
            template <class X> bool operator<(const X *r) const { return ptr_<r; }

            void release() { if(ptr_) { pic_decref(ptr_); ptr_ = 0; } } 
            const T& operator*() const { PIC_ASSERT(ptr_); return *ptr_; } 
            T& operator*() { PIC_ASSERT(ptr_); return *ptr_; }
            const T* operator->() const { PIC_ASSERT(ptr_); return ptr_; }
            T* operator->() { PIC_ASSERT(ptr_); return ptr_; }
            T *ptr() const { return ptr_; } 
            T *checked_ptr() const { PIC_ASSERT(ptr_); return ptr_; } 
            T *lend() const { return ptr_; }
            T *give() const { if(ptr_) pic_incref(ptr_); return ptr_; }

            bool isvalid() const { return ptr_ != 0; }
            void clear() { assign(0); }
            void reset() { ptr_=0; }

            static ref_t from_given(T *ptr) { return ref_t(ptr); }
            static ref_t from_lent(T *ptr) { if(ptr) pic_incref(ptr); return ref_t(ptr); }

            void assign(T *p) { if(ptr_ != p) { if(ptr_) { pic_decref(ptr_); } ptr_ = p; if(ptr_) { pic_incref(ptr_); } } }

        private:
            ref_t(T *t) : ptr_(t) { } 

        private:
            T *ptr_;
    };

    template <class T> ref_t<T> ref(T *x) { return ref_t<T>::from_given(x); }

    class counted_t
    {
        public:
            void incref() { count_++; }
            void decref() { if(--count_==0) counted_deallocate(); }
            unsigned count() const { return count_; }
        protected:
            counted_t(): count_(1) {}
            virtual void counted_deallocate() { delete this; }
            virtual ~counted_t() {}
        private:
            unsigned count_;
    };

    class atomic_counted_t
    {
        public:
            void incref() { pic_atomicinc(&count_); }
            void decref() { if(pic_atomicdec(&count_)==0) counted_deallocate(); }
            unsigned count() const { return count_; }
        protected:
            atomic_counted_t(): count_(1) {}
            virtual void counted_deallocate() { delete this; }
            virtual ~atomic_counted_t() {}
        private:
            pic_atomic_t count_;
    };

inline void pic_incref(pic::counted_t *c) { c->incref(); }
inline void pic_decref(pic::counted_t *c) { c->decref(); }
inline void pic_incref(pic::atomic_counted_t *c) { c->incref(); }
inline void pic_decref(pic::atomic_counted_t *c) { c->decref(); }
};

#endif
