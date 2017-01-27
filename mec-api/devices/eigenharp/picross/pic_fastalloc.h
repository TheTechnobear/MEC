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

#ifndef __PICROSS_PIC_FASTALLOC__
#define __PICROSS_PIC_FASTALLOC__

#include "pic_exports.h"

#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <new>
#include "pic_config.h"
#include <picross/pic_thread.h>
#include <picross/pic_nocopy.h>



#ifndef PI_WINDOWS
#include <sys/mman.h>
#endif

#define PIC_ALLOC_NORMAL 0
#define PIC_ALLOC_LCK 1
#define PIC_ALLOC_NB 2

#define PIC_ALLOC_SLABSIZE (4096*15-8)

namespace pic
{
    struct PIC_DECLSPEC_CLASS nballocator_t
    {
        virtual ~nballocator_t() {}
        typedef void (*deallocator_t)(void *, void *);
        virtual void *allocator_xmalloc(unsigned nb, size_t size, deallocator_t *dealloc, void **dealloc_arg) = 0;
        static pic::tsd_t nballoc__;
        static void tsd_setnballocator(nballocator_t *alloc) { nballoc__.set(alloc); }
        static void tsd_clearnballocator() { tsd_setnballocator(0); }
        static nballocator_t *tsd_getnballocator() { return (pic::nballocator_t *)(nballoc__.get()); }

    };

    PIC_DECLSPEC_FUNC(void) *nb_malloc(unsigned nb,nballocator_t *allocator, size_t size);
    PIC_DECLSPEC_FUNC(void) *nb_malloc(unsigned nb,size_t size);
    PIC_DECLSPEC_FUNC(void) nb_free(void *ptr);

    template<typename T> class stllckallocator_t
    {
        public:
            typedef size_t size_type;
            typedef ptrdiff_t  difference_type;
            typedef T* pointer;
            typedef const T* const_pointer;
            typedef T& reference;
            typedef const T& const_reference;
            typedef T value_type;

            template<typename X> struct rebind { typedef stllckallocator_t<X> other; };

            stllckallocator_t() throw() {}
            stllckallocator_t(const stllckallocator_t&) throw() {}
            template<typename X> stllckallocator_t(const stllckallocator_t<X>&) throw() {}
            ~stllckallocator_t() throw() {}

            size_t max_size() const { return 65536U; }

            pointer address(reference x) const { return &x; }
            const_pointer address(const_reference x) const { return &x; }

            pointer allocate(size_type n, const void* =0) { T *t=static_cast<T*>(nb_malloc(PIC_ALLOC_NB,n*sizeof(T))); return t; }
            void deallocate(pointer p, size_type) { nb_free(static_cast<void *>(p)); }

            void construct(pointer p, const T &val) { ::new(p) T(val); }
            void destroy(pointer p) { p->~T(); }
    };

    template<typename T> inline bool operator==(const stllckallocator_t<T>&, const stllckallocator_t<T>&) { return true; }
    template<typename T> inline bool operator!=(const stllckallocator_t<T>&, const stllckallocator_t<T>&) { return false; }

    template<typename T> class stlnballocator_t
    {
        public:
            typedef size_t size_type;
            typedef ptrdiff_t  difference_type;
            typedef T* pointer;
            typedef const T* const_pointer;
            typedef T& reference;
            typedef const T& const_reference;
            typedef T value_type;

            template<typename X> struct rebind { typedef stlnballocator_t<X> other; };

            stlnballocator_t() throw() {}
            stlnballocator_t(const stlnballocator_t&) throw() {}
            template<typename X> stlnballocator_t(const stlnballocator_t<X>&) throw() {}
            ~stlnballocator_t() throw() {}

            size_t max_size() const { return 65536U; }

            pointer address(reference x) const { return &x; }
            const_pointer address(const_reference x) const { return &x; }

            pointer allocate(size_type n, const void* =0) { T *t=static_cast<T*>(nb_malloc(PIC_ALLOC_NB,n*sizeof(T))); return t; }
            void deallocate(pointer p, size_type) { nb_free(static_cast<void *>(p)); }

            void construct(pointer p, const T &val) { ::new(p) T(val); }
            void destroy(pointer p) { p->~T(); }
    };

    template<typename T> inline bool operator==(const stlnballocator_t<T>&, const stlnballocator_t<T>&) { return true; }
    template<typename T> inline bool operator!=(const stlnballocator_t<T>&, const stlnballocator_t<T>&) { return false; }

    class PIC_DECLSPEC_CLASS lckobject_t
    {
        public:
            static void *operator new(size_t size, nballocator_t *allocator) { return nb_malloc(PIC_ALLOC_NB,allocator,size); }
            static void *operator new(size_t size) { return nb_malloc(PIC_ALLOC_NB,size); }
            static void operator delete(void *ptr) { nb_free(ptr); }
    };

    template<class T, unsigned N>
    class lckarray_t : public pic::nocopy_t
    {
        public:
#ifndef PI_WINDOWS        
            lckarray_t(T *ptr): ptr_(ptr) { mlock(ptr_,N*sizeof(T)); }
            ~lckarray_t() { munlock(ptr_,N*sizeof(T)); }
#else
            lckarray_t(T *ptr): ptr_(ptr) { VirtualLock(ptr_,N*sizeof(T)); }
            ~lckarray_t() { VirtualUnlock(ptr_,N*sizeof(T)); }
#endif
        private:
            T *ptr_;
    };
};

#endif
