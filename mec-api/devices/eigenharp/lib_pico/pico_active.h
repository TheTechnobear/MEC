
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

#ifndef __PICO_ACTIVE__
#define __PICO_ACTIVE__

#include <lib_pico/pico_exports.h>
#include <picross/pic_usb.h>
#include <resources/pico_decoder.h>

namespace pico
{
    class PICO_DECLSPEC_CLASS active_t: public pic::pollable_t
    {
        public:
            class impl_t;

        public:
            typedef pico_rawkey_t rawkey_t;
            typedef pico_rawkbd_t rawkbd_t;

            struct delegate_t
            {
                virtual ~delegate_t() {}
                virtual void kbd_dead(unsigned reason) {}
                virtual void kbd_raw(bool resync,const rawkbd_t &) {}
                virtual void kbd_key(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y) {}
                virtual void kbd_strip(unsigned long long t, unsigned s) {}
                virtual void kbd_breath(unsigned long long t, unsigned b) {}
                virtual void kbd_mode(unsigned long long t, unsigned key, unsigned m) {}
            };

        public:
            active_t(const char *name, delegate_t *);
            ~active_t();

            void start();
            void stop();
            bool poll(unsigned long long);
            void set_raw(bool raw);

            const char *get_name();

            std::string debug();

            void set_calibration(unsigned key, unsigned corner, unsigned short min, unsigned short max, const unsigned short *table);
            void write_calibration();
            void clear_calibration();
            bool get_calibration(unsigned key, unsigned corner, unsigned short *min, unsigned short *max, unsigned short *table);
            unsigned get_temperature();
            void load_calibration_from_device();

            void set_led(unsigned key, unsigned colour);

            pic::usbdevice_t *device();

        private:
            impl_t *_impl;
    };
};

#endif
