
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

#ifndef __PIC_ATOMIC_H__
#define __PIC_ATOMIC_H__

#include "pic_config.h"

/*
 * pic_atomic increment/decrement operations
 * work on unsigned 32 bit, return new value.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <picross/pic_stdint.h>

typedef volatile uint32_t pic_atomic_t;
typedef void * volatile pic_atomicptr_t;

#if defined(PI_LINUX)
#if defined(PI_LINUX_86)

inline static pic_atomic_t pic_atomicinc(pic_atomic_t *p)
{
    volatile pic_atomic_t r;

    __asm__ __volatile__(
        "lock xaddl %0, (%1);"
        "inc   %0;"
        : "=r" (r)
        : "r" (p), "0" (0x1)
        : "cc", "memory"
        );

    return r;
}

inline static pic_atomic_t pic_atomicdec(pic_atomic_t *p)
{   
    volatile pic_atomic_t r;

    __asm__ __volatile__(
        "lock xaddl %0, (%1);"
        "dec   %0;"
        : "=r" (r)
        : "r" (p), "0" (-1)
        : "cc", "memory"
    );

    return r;
}

inline static int pic_atomiccas(pic_atomic_t *p, pic_atomic_t oval, pic_atomic_t nval)
{
    pic_atomic_t r;

    __asm__ __volatile__(
        "lock cmpxchgl %2, %1;"
        : "=a" (r), "=m" (*p)
        : "r" (nval), "m" (*p), "0" (oval)
    );

    return r == oval;
}

inline static int pic_atomicptrcas(void *p_, void *oval, void *nval)
{
    void *r;
    void **p = (void **)p_;

    __asm__ __volatile__(
        "lock cmpxchgl %2, %1;"
        : "=a" (r), "=m" (*p)
        : "r" (nval), "m" (*p), "0" (oval)
    );

    return r == oval;
}

#endif

#if defined(PI_LINUX_8664)

inline static pic_atomic_t pic_atomicinc(pic_atomic_t *p)
{
    volatile unsigned long r;

    __asm__ __volatile__(
        "lock xaddl %%ebx, (%%rax);"
        "incl  %%ebx;"
        : "=b" (r)
        : "a" (p), "b" (0x1)
        : "cc", "memory"
    );

    return r;
}

inline static pic_atomic_t pic_atomicdec(pic_atomic_t *p)
{
    volatile pic_atomic_t r;

    __asm__ __volatile__(
        "lock xaddl %%ebx, (%%rax);"
        "decl  %%ebx;"
        : "=b" (r)
        : "a" (p), "b" (-1)
        : "cc", "memory"
    );

    return r;
}

inline static int pic_atomiccas(pic_atomic_t *p, pic_atomic_t oval, pic_atomic_t nval)
{
    pic_atomic_t r;

    __asm__ __volatile__(
        "lock cmpxchgl %2, %1;"
        : "=a" (r), "=m" (*p)
        : "r" (nval), "m" (*p), "0" (oval)
    );

    return r == oval;
}

inline static int pic_atomicptrcas(void *p_, void *oval, void *nval)
{
    void *r;
    void **p = (void **)p_;

    __asm__ __volatile__(
        "lock cmpxchgq %q2, %1;"
        : "=a" (r), "=m" (*p)
        : "r" (nval), "m" (*p), "0" (oval)
    );

    return r == oval;
}

#endif

#if defined(PI_LINUX_PPC32)

inline static pic_atomic_t pic_atomicinc(pic_atomic_t *p)
{
    volatile pic_atomic_t r;

    __asm__ __volatile__ (
        "1:      lwarx  %0, %3, %2;\n"
        "        addi   %0, %0, 1;\n"
        "        stwcx. %0, %3, %2;\n"
        "        bne- 1b;"
         : "=b" (r)
         : "0" (r), "b" (p), "b" (0x0)
         : "cc", "memory"
         );
    
    return r;
}

inline static pic_atomic_t pic_atomicdec(pic_atomic_t *p)
{
    volatile pic_atomic_t r;

    __asm__ __volatile__ (
        "1:      lwarx  %0, %3, %2;\n"
        "        subi   %0, %0, 1;\n"
        "        stwcx. %0, %3, %2;\n"
        "        bne- 1b;"
         : "=b" (r)
         : "0" (r), "b" (p), "b" (0x0)
         : "cc", "memory"
         );
    
    return r;
}

inline static int pic_atomiccas(pic_atomic_t *p, pic_atomic_t oval, pic_atomic_t nval)
{
    pic_atomic_t r;

    __asm__ __volatile__ ("sync\n"
        "1:     lwarx   %0,0,%1\n"
        "       subf.   %0,%2,%0\n"
        "       bne     2f\n"
        "       stwcx.  %3,0,%1\n"
        "       bne-    1b\n"
        "2:     isync"
        : "=&r" (r)
        : "b" (p), "r" (oval), "r" (nval)
        : "cr0", "memory"); 

    return r == 0;
}

inline static int pic_atomicptrcas(void *p_, void *oval, void *nval)
{
    void *r;
    void **p = (void **)p_;

    __asm__ __volatile__ ("sync\n"
        "1:     lwarx   %0,0,%1\n"
        "       subf.   %0,%2,%0\n"
        "       bne     2f\n"
        "       stwcx.  %3,0,%1\n"
        "       bne-    1b\n"
        "2:     isync"
        : "=&r" (r)
        : "b" (p), "r" (oval), "r" (nval)
        : "cr0", "memory"); 

    return r == 0;
}

#endif

#if defined(PI_LINUX_PPC64)

inline static pic_atomic_t pic_atomicinc(pic_atomic_t *p)
{
    volatile pic_atomic_t r;

    __asm__ __volatile__ (
        "1:      lwarx  %0, %3, %2;\n"
        "        addi   %0, %0, 1;\n"
        "        stwcx. %0, %3, %2;\n"
        "        bne- 1b;"
         : "=b" (r)
         : "0" (r), "b" (p), "b" (0x0)
         : "cc", "memory"
         );
    
    return r;
}

inline static pic_atomic_t pic_atomicdec(pic_atomic_t *p)
{
    volatile pic_atomic_t r;

    __asm__ __volatile__ (
        "1:      lwarx  %0, %3, %2;\n"
        "        subi   %0, %0, 1;\n"
        "        stwcx. %0, %3, %2;\n"
        "        bne- 1b;"
         : "=b" (r)
         : "0" (r), "b" (p), "b" (0x0)
         : "cc", "memory"
         );
    
    return r;
}

inline static int pic_atomiccas(pic_atomic_t *p, pic_atomic_t oval, pic_atomic_t nval)
{
    pic_atomic_t r;

    __asm__ __volatile__ ("sync\n"
        "1:     lwarx   %0,0,%1\n"
        "       subf.   %0,%2,%0\n"
        "       bne     2f\n"
        "       stwcx.  %3,0,%1\n"
        "       bne-    1b\n"
        "2:     isync"
        : "=&r" (r)
        : "b" (p), "r" (oval), "r" (nval)
        : "cr0", "memory"); 

    return r == 0;
}

inline static int pic_atomicptrcas(void *p_, void *oval, void *nval)
{
    void *r;
    void **p = (void **)p_;

    __asm__ __volatile__ ("sync\n"
        "1:     ldarx   %0,0,%1\n"
        "       subf.   %0,%2,%0\n"
        "       bne     2f\n"
        "       stdcx.  %3,0,%1\n"
        "       bne-    1b\n"
        "2:     isync"
        : "=&r" (r)
        : "b" (p), "r" (oval), "r" (nval)
        : "cr0", "memory"); 

    return r == 0;
}

#endif


#ifdef PI_LINUX_ARMV7L
#include "pic_atomic_arm7.h"
#endif


#endif

#ifdef PI_MACOSX

#include <libkern/OSAtomic.h>

#if defined(PI_MACOSX_86) || defined(PI_MACOSX_8664)

inline static uint32_t pic_atomicinc(pic_atomic_t *p)
{
    //__asm__ __volatile__("lock incl (%0);" :: "r" (p));
    //return *p;
    return (uint32_t)OSAtomicIncrement32Barrier((int32_t *)p);
}

inline static uint32_t pic_atomicdec(pic_atomic_t *p)
{
    //__asm__ __volatile__("lock decl (%0);" :: "r" (p));
    //return *p;
    return (uint32_t)OSAtomicDecrement32Barrier((int32_t *)p);
}

#else

inline static uint32_t pic_atomicinc(pic_atomic_t *p)
{
    return OSAtomicIncrement32Barrier((int32_t *)p);
}

inline static uint32_t pic_atomicdec(pic_atomic_t *p)
{
    return OSAtomicDecrement32Barrier((int32_t *)p);
}

#endif

inline static int pic_atomiccas(pic_atomic_t *p, pic_atomic_t oval, pic_atomic_t nval)
{
    return OSAtomicCompareAndSwap32Barrier((int32_t)oval,(int32_t)nval,(int32_t *)p);
}

#if defined(PI_MACOSX_86) || defined(PI_MACOSX_PPC32)

inline static int pic_atomicptrcas(void *p, void *oval, void *nval)
{
    return OSAtomicCompareAndSwap32Barrier((int32_t)oval,(int32_t)nval,(int32_t *)p);
}

#endif

#if defined(PI_MACOSX_8664) || defined(PI_MACOSX_PPC64)

inline static int pic_atomicptrcas(void *p, void *oval, void *nval)
{
    return OSAtomicCompareAndSwap64Barrier((int64_t)oval,(int64_t)nval,(int64_t *)p);
}

#endif

#endif

#ifdef PI_WINDOWS

#include <picross/pic_windows.h>

inline static uint32_t pic_atomicinc(pic_atomic_t *p)
{
    return InterlockedIncrement((long *)p);
}

inline static uint32_t pic_atomicdec(pic_atomic_t *p)
{
    return InterlockedDecrement((long *)p);
}

inline static int pic_atomiccas(pic_atomic_t *p, pic_atomic_t oval, pic_atomic_t nval)
{
    return (InterlockedCompareExchange((long *)p,nval,oval)==oval);
}

inline static int pic_atomicptrcas(void *p, void *oval, void *nval)
{
	return (InterlockedCompareExchangePointer((PVOID *)p,nval,oval)==oval);
}

#endif


#ifdef __cplusplus
}
#endif

#endif
