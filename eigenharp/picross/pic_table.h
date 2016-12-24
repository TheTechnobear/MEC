
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

#ifndef __PICROSS_TABLE__
#define __PICROSS_TABLE__

#include <picross/pic_stl.h>
#include <pic_exports.h>
#include <picross/pic_error.h>

namespace pic
{
    class ftable_t
    {
        public:
            ftable_t(unsigned size)
            {
                set_size(size);
            }

            float interp(float v) const
            {
                float v0 = v-lbound_;

                if(v0<=0.0) return entries_[0];
                if(v0>=range_) return entries_[size_-1];

                float v1 = v0/bwidth_;
                unsigned n = (unsigned)v1;
                float v2 = v1-(float)n;
                float e0 = entries_[n];
                float e1 = entries_[n+1];

                return e0+v2*(e1-e0);
            }

            void set_size(unsigned size)
            {
                size_=size;
                fsize_=size-1;
                entries_.resize(size);
                lbound_=0.0;
                bwidth_=1.0/float(size);
                range_=1.0;
            }

            void set_bounds(float l, float u)
            {
                lbound_=l;
                range_=u-l;
                bwidth_=range_/fsize_;
            }

            void set_entry(unsigned i, float v)
            {
                PIC_ASSERT(i<size_);
                entries_[i]=v;
            }

            unsigned size() const
            {
                return size_;
            }

        private:
            unsigned size_;
            float fsize_;
            pic::lckvector_t<float>::nbtype entries_;
            float lbound_;
            float bwidth_;
            float range_;
    };
};

#endif
