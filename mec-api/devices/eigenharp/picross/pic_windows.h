
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

#ifdef WRITE_RESTRICTED
#define __COPY_WR__ WRITE_RESTRICTED
#undef WRITE_RESTRICTED
#pragma message ("Hiding winnt.h #DEFINE of WRITE_RESTRICTED")
#endif
#define NOMINMAX		// need if we use std::minmax - removes windows #define of the same name
#include "Ws2tcpip.h"
#include <windows.h>
#ifdef __COPY_WR__
#undef WRITE_RESTRICTED
#define  WRITE_RESTRICTED __COPY_WR__
#endif
