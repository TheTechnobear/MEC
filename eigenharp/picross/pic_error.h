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

#ifndef __PIC_ERROR__
#define __PIC_ERROR__

#include <exception>
#include <string>

#include "pic_config.h"
#include "pic_exports.h"

namespace pic
{
    PIC_DECLSPEC_FUNC(std::string) backtrace();
    PIC_DECLSPEC_FUNC(void) maybe_abort(const char *,const char *,unsigned)
#ifndef PI_WINDOWS
	__attribute__((noreturn))
#endif
	;
    PIC_DECLSPEC_FUNC(void) exit(int);

    class PIC_DECLSPEC_CLASS error: public std::exception
    {
        public:
            error() {}
            error(const char *m);
            error(const char *m, const char *f, unsigned l);
            ~error() throw() {}

            const char *what() const throw() { return _what.c_str(); }

        private:
            std::string _what;
    };
};

#define PIC_THROW(m) pic::maybe_abort(m,__FILE__,__LINE__)
#define PIC_ASSERT(e) do { if(!(e)) PIC_THROW("assertion failure: " #e); } while(0)

#endif
