
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

#ifndef __PIC_RESOURCES_H__
#define __PIC_RESOURCES_H__

#include "pic_exports.h"
#include <string>
#include <stdio.h>

namespace pic
{
    PIC_DECLSPEC_FUNC(std::string) global_resource_dir();
    PIC_DECLSPEC_FUNC(std::string) release_resource_dir();
    PIC_DECLSPEC_FUNC(std::string) release_root_dir();
    PIC_DECLSPEC_FUNC(std::string) contrib_root_dir();
    PIC_DECLSPEC_FUNC(std::string) contrib_compatible_dir();
    PIC_DECLSPEC_FUNC(std::string) global_library_dir();
    PIC_DECLSPEC_FUNC(std::string) release_library_dir();
    PIC_DECLSPEC_FUNC(std::string) python_prefix_dir();
    PIC_DECLSPEC_FUNC(std::string) public_tools_dir();
    PIC_DECLSPEC_FUNC(std::string) private_tools_dir();
    PIC_DECLSPEC_FUNC(std::string) private_exe_dir();
    PIC_DECLSPEC_FUNC(char) platform_seperator();
    PIC_DECLSPEC_FUNC(std::string) release();
    PIC_DECLSPEC_FUNC(std::string) username();
    PIC_DECLSPEC_FUNC(int) mkdir(std::string);
    PIC_DECLSPEC_FUNC(int) mkdir(const char *);
    PIC_DECLSPEC_FUNC(int) remove(std::string);
    PIC_DECLSPEC_FUNC(int) remove(const char *);
    PIC_DECLSPEC_FUNC(FILE) *fopen(std::string, const char *);
    PIC_DECLSPEC_FUNC(FILE) *fopen(const char *, const char *);
    PIC_DECLSPEC_FUNC(int) open(std::string, int);
    PIC_DECLSPEC_FUNC(int) open(std::string, int, int);
    PIC_DECLSPEC_FUNC(int) open(const char *, int);
    PIC_DECLSPEC_FUNC(int) open(const char *, int, int);
    PIC_DECLSPEC_FUNC(std::string) lockfile(const std::string &);

    class PIC_DECLSPEC_CLASS lockfile_t
    {
        public:
            class impl_t;

        public:
            lockfile_t(const std::string &name);
            ~lockfile_t();

            bool lock();
            void unlock();

        private:
            std::string name_;
            impl_t *impl_;
    };
};

#endif
