
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

#include "pic_config.h"

#define PIC_THREAD_PRIORITY_LOW -1
#define PIC_THREAD_PRIORITY_NORMAL 0
#define PIC_THREAD_PRIORITY_HIGH 1
#define PIC_THREAD_PRIORITY_REALTIME 2

#ifdef PI_WINDOWS
#include "pic_thread_win32.h"
#else
#include "pic_thread_posix.h"
#endif
