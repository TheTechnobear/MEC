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

#ifndef __PIC_XREF__
#define __PIC_XREF__

#include <picross/pic_log.h>
#include <picross/pic_error.h>

namespace pic
{
    template <class PTRTYPE> class xref_t
    {
        public:
            typedef void (*dispose_t)(PTRTYPE *, void *);

        private:
            struct shared_t
            {
                dispose_t dispose;
                void *context;
                unsigned count;
            };

        public:
            xref_t(): _ptr(0), _shared(0) {}
            ~xref_t() { assign(0,0); }

            static xref_t create(PTRTYPE *ptr, dispose_t dispose = 0, void *context = 0) { return xref_t(ptr,dispose,context); }

            xref_t(const xref_t &r): _ptr(0), _shared(0) { assign(r._ptr,r._shared); }
            xref_t &operator=(const xref_t &r) { assign(r._ptr,r._shared); return *this; }

            template <class X> xref_t(const xref_t<X> &r): _ptr(0), _shared(0) { assign(r._ptr,r._shared); }
            template <class X> xref_t &operator=(const xref_t<X> &r) { assign(r._ptr,r._shared); return *this; }
            template <class X> bool operator==(const xref_t<X> &r) const { return r._ptr==_ptr; }
            template <class X> bool operator==(const X *r) const { return r==_ptr; }
            template <class X> bool operator<(const xref_t<X> &r) const { return _ptr<r._ptr; }
            template <class X> bool operator<(const X *r) const { return _ptr<r; }

            void clear() { assign(0,0); }

            PTRTYPE *operator->() const { PIC_ASSERT(_ptr); return _ptr; }
            PTRTYPE *ptr() const { return _ptr; }
            PTRTYPE *checked_ptr() const { PIC_ASSERT(_ptr); return _ptr; }

        private:
            xref_t(PTRTYPE *p, dispose_t dispose, void *c): _ptr(p), _shared(0) { if(!p) return; _shared=new shared_t; _shared->count=1; _shared->dispose=dispose; _shared->context=c; }
            void assign(PTRTYPE *x, shared_t *c) { if(x!=_ptr) { if(_ptr && --(_shared->count)==0) dispose();  _ptr=x; _shared=c; if(_ptr) _shared->count++; } }
            void dispose() { try { if(_shared->dispose) (_shared->dispose)(_ptr, _shared->context); else delete _ptr; } CATCHLOG() try { delete _shared; } CATCHLOG() _ptr=0; _shared=0; }

        private:
            PTRTYPE *_ptr;
            shared_t *_shared;
    };

    template <class PTRTYPE> xref_t<PTRTYPE> xref(PTRTYPE *x, typename xref_t<PTRTYPE>::dispose_t d=0, void *c=0) { return xref_t<PTRTYPE>::create(x,d,c); }

};

#endif
