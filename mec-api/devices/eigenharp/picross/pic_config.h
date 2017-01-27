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

#ifndef __PICROSS_PIC_CONFIG__
#define __PICROSS_PIC_CONFIG__

/* echo foo | gcc -dM -E - */
#ifdef _WIN32
#pragma warning (disable : 4251 4275 4355 4244 4800 4099 4996 4305 4200 4291 4018)
#define PI_WINDOWS
#define PI_IS_LINUX 0
#define PI_IS_MACOSX 0
#define PI_IS_WINDOWS 1
#define __PI_ARCH "win32"
#define lround(tt) floor((tt) + 0.5f)
#define lroundf(tt) floorf((tt) + 0.5f)

#define NOMINMAX
#endif

#ifdef __linux__
#define PI_LINUX
#define PI_IS_LINUX 1
#define PI_IS_MACOSX 0
#define PI_IS_WINDOWS 0
#ifdef __PPC__
#define PI_LINUX_PPC32
#define __PI_ARCH "linux ppc32"
#define __PI_BIGENDIAN
#endif
#ifdef __i386__
#define PI_LINUX_86
#define __PI_ARCH "linux x86"
#endif
#ifdef __x86_64__
#define PI_LINUX_8664
#define __PI_ARCH "linux x86-64"
#endif
#if defined(__ARM_ARCH_6__) || defined( __ARM_ARCH_7__) || defined( __ARM_ARCH_7A__)
#define PI_LINUX_ARMV7L
#define __PI_ARCH "linux armv7l"
#endif
#endif

#ifdef __APPLE__
#define PI_MACOSX
#define PI_IS_LINUX 0
#define PI_IS_MACOSX 1
#define PI_IS_WINDOWS 0
#ifdef __ppc__
#define PI_MACOSX_PPC32
#define __PI_ARCH "macosx ppc32"
#define __PI_BIGENDIAN
#endif
#ifdef __ppc64__
#define PI_MACOSX_PPC64
#define __PI_ARCH "macosx ppc64"
#define __PI_BIGENDIAN
#endif
#ifdef __i386__
#define PI_MACOSX_86
#define __PI_ARCH "macosx x86"
#endif
#ifdef __x86_64__
#define PI_MACOSX_8664
#define __PI_ARCH "macosx x86-64"
#endif
#endif

#if !defined(PI_ARCH) || (!defined(PI_LITTLEENDIAN) && !defined(PI_BIGENDIAN))

#ifndef __PI_ARCH
#error "unsupported platform"
#else
#define PI_ARCH __PI_ARCH
#ifdef __PI_BIGENDIAN
#define PI_BIGENDIAN
#else
#define PI_LITTLENDIAN
#endif
#endif

#endif

#ifdef PI_MACOSX
//as we use -Werror downgrade certain warnings, and ignore ones which are very frequent
#pragma GCC diagnostic ignored "-Wmismatched-tags"
 
#endif

#endif
