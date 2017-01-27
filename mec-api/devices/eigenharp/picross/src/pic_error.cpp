
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

#include <picross/pic_error.h>
#include <picross/pic_backtrace.h>
#include <picross/pic_log.h>
#include <picross/pic_time.h>

#include <cstdlib>
#include <cstdio>

std::string pic::backtrace()
{
    char buffer[10240];
    pic_backtrace(buffer,sizeof(buffer));
    return buffer;
}

void pic::exit(int rc)
{
    ::_exit(rc);
}

void pic::maybe_abort(const char *m, const char *f, unsigned l)
{
    if(getenv("PI_ABORT") != 0)
    {
        fprintf(stderr,"aborting: %s from %s:%u\n",m,f,l);
        fflush(stderr);
        pic_microsleep(5000);
        abort();
    }

    pic::msg() << m << pic::log;
    throw pic::error(m,f,l);
}

pic::error::error(const char *m, const char *f, unsigned l)
{
    char buffer[10240];
    sprintf(buffer,"%s from %s:%u (%s)",m,f,l,backtrace().c_str());
    _what=buffer;
}

pic::error::error(const char *m)
{
    char buffer[10240];
    sprintf(buffer,"%s (%s)",m,backtrace().c_str());
    _what=buffer;
}
