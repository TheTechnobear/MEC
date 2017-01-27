
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

#include <picross/pic_fastalloc.h>
#include <picross/pic_error.h>

#include <sys/types.h>

#include <picross/pic_config.h>

#ifndef PI_WINDOWS
#include <sys/mman.h>
#endif

namespace
{
    union nbhdr_t
    {
        struct
        {
            pic::nballocator_t::deallocator_t dealloc;
            void *dealloc_arg;
        };
#ifdef ALIGN_16
        uint32_t __align[4];
#endif
    };

    static void ordinary_free(void *ptr, void *arg)
    {
        free(ptr);
    }
};

pic::tsd_t pic::nballocator_t::nballoc__;

void *pic::nb_malloc(unsigned nb,pic::nballocator_t *allocator, size_t size)
{
    pic::nballocator_t::deallocator_t dealloc;
    void *dealloc_arg;

    size+=sizeof(nbhdr_t);
    nbhdr_t *h = (nbhdr_t *)(allocator->allocator_xmalloc(nb,size,&dealloc,&dealloc_arg));

    PIC_ASSERT(h);

    h->dealloc=dealloc;
    h->dealloc_arg=dealloc_arg;

    return (void *)(h+1);
}

void *pic::nb_malloc(unsigned nb,size_t size)
{
    pic::nballocator_t *a = pic::nballocator_t::tsd_getnballocator();

    pic::nballocator_t::deallocator_t dealloc;
    void *dealloc_arg = 0;

    size+=sizeof(nbhdr_t);

    nbhdr_t *h;

    if(a)
    {
        h = (nbhdr_t *)(a->allocator_xmalloc(nb,size,&dealloc,&dealloc_arg));
    }
    else
    {
        //if(nb!=PIC_ALLOC_NORMAL)
        //    fprintf(stderr,"warning: nb_malloc without allocator\n");

        h = (nbhdr_t *)malloc(size);
        dealloc = ordinary_free;
    }

    PIC_ASSERT(h);

    h->dealloc=dealloc;
    h->dealloc_arg=dealloc_arg;

    return (void *)(h+1);
}

void pic::nb_free(void *ptr)
{
    nbhdr_t *h = ((nbhdr_t *)ptr)-1;
    (h->dealloc)(h,h->dealloc_arg);
}
