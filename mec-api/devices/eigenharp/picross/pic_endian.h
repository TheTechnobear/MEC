
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

#ifndef __PIC_ENDIAN__
#define __PIC_ENDIAN__

#include <picross/pic_config.h>
#include <picross/pic_stdint.h>

namespace pic
{
    inline uint32_t swap32(uint32_t x)
    {
        return ((x & 0xff000000) >> 24) | 
               ((x & 0x00ff0000) >>  8) | 
               ((x & 0x0000ff00) <<  8) | 
               ((x & 0x000000ff) << 24);
    }

    inline uint16_t swap16(uint16_t x)
    {
        return ((x & 0xff00) >> 8) |
               ((x & 0x00ff) << 8);
    }
}

#ifdef PI_BIGENDIAN
inline uint32_t pic_ntohl(uint32_t x) { return x; }
inline uint16_t pic_ntohs(uint16_t x) { return x; }
#else
inline uint32_t pic_ntohl(uint32_t x) { return pic::swap32(x); }
inline uint16_t pic_ntohs(uint16_t x) { return pic::swap16(x); }
#endif

#endif // __PIC_ENDIAN__
