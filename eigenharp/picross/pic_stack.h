
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

#ifndef __PICROSS_STACK__
#define __PICROSS_STACK__

#include <picross/pic_atomic.h>

namespace pic
{
    struct stacknode_t
    {
        stacknode_t(): next_(0)
        {
        }

        stacknode_t *next_;
    };

    struct stack_t
    {
        stack_t(): head_(0)
        {
        }

        void push(stacknode_t *n)
        {
            for(;;)
            {
                n->next_ = head_;
                if(pic_atomicptrcas(&head_,n->next_,n))
                {
                    break;
                }
            }
        }

        stacknode_t *pop()
        {
            if(!head_)
            {
                return 0;
            }

            stacknode_t *n;

            for(;;)
            {
                n = head_;
                if(pic_atomicptrcas(&head_,n,n->next_))
                {
                    break;
                }
            }

            return n;
        }

        stacknode_t *head_;
    };
}

#endif
