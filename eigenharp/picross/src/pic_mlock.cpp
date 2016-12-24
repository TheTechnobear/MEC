
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

#include <picross/pic_thread.h>

#ifdef PI_MACOSX

extern "C"
{
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>
#include <mach-o/ldsyms.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
}

static const char *name__(const struct mach_header *mh)
{
    unsigned c = _dyld_image_count();

    for(unsigned i=0;i<c;i++)
    {
        if(mh==_dyld_get_image_header(i))
        {
            return _dyld_get_image_name(i);
        }
    }

    return "unknown image";
}

static const void *finder__(const struct mach_header* mh, intptr_t slide,const char *seg, const char *section, unsigned long *size)
{

#if __LP64__
    const struct section_64 *sect = getsectbynamefromheader_64((struct mach_header_64 *)mh,seg,section);
#else
    const struct section *sect = getsectbynamefromheader((struct mach_header *)mh,seg,section);
#endif

    if(!sect)
    {
        return 0;
    }

    unsigned long p = getpagesize();
    unsigned long a = sect->addr+slide;
    unsigned long s = sect->size;

    a &= ~(p-1);
    s = p*((s+p-1)/p);

    if(size) *size = s;
    return (const void *)a;
}

static void locker__(const struct mach_header* mh, intptr_t slide,const char *seg, const char *section)
{
    const void *p;
    unsigned long s;

    if((p=finder__(mh,slide,seg,section,&s))!=0)
    {
        //printf("locking section (%s:%s) %p %lu\n",seg,section,p,s);
        mlock(p,s);
    }
}

static void handler__(const struct mach_header* mh, intptr_t slide)
{
    if(finder__(mh,slide,"__DATA","__fastdata",0))
    {
        printf("locking %s\n",name__(mh));
        locker__(mh,slide,"__DATA","__const"); // vtables in here...
        locker__(mh,slide,"__TEXT","__text"); // code in here...
    }
}

void pic_mlock_code()
{
    _dyld_register_func_for_add_image(handler__);
}

#else

void pic_mlock_code()
{
}

#endif
