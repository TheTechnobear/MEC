
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

#ifndef __PICO_PASSIVE__
#define __PICO_PASSIVE__
#include <lib_pico/pico_exports.h>
#include <string>

namespace pico
{
    class PICO_DECLSPEC_CLASS passive_t
    {
        public:
            passive_t(const char *name, unsigned decim);
            ~passive_t();

            unsigned short get_rawkey(unsigned key, unsigned sensor);
            unsigned short get_breath();
            unsigned short get_strip();
            unsigned short get_button(unsigned key);

            void set_ledcolour(unsigned key, unsigned colour);

            std::string debug();

            bool wait();
            bool is_kbd_died();
            void sync();

            void start_calibration_row(unsigned,unsigned);
            void set_calibration_range(unsigned short,unsigned short);
            void set_calibration_point(unsigned,unsigned short);
            void write_calibration_row();
            void commit_calibration();
            void clear_calibration();
            unsigned get_temperature();

            void read_calibration_row();
            unsigned short get_calibration_min();
            unsigned short get_calibration_max();
            unsigned short get_calibration_point(unsigned point);

            void start();
            void stop();

            void start_collecting();
            void stop_collecting();
            unsigned collect_count();
            unsigned short get_collected_key(unsigned scan, unsigned key, unsigned corner);

            class impl_t;
        private:
            impl_t *impl_;
    };
};

#endif
