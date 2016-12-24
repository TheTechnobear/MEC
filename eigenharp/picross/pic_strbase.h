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

#ifndef __PIC_STRBASE__
#define __PIC_STRBASE__

#include <picross/pic_stdint.h>

#define	PIC_FLDOFF(name, field) (((uintptr_t)&(((struct name *)1)->field))-1)
#define	PIC_STRBASE(name, addr, field) ((struct name *)((char *)(addr) - PIC_FLDOFF(name, field)))

#endif

/* vim: set filetype=cpp : */
