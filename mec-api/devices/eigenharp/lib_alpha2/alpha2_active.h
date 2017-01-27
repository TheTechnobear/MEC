
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

#ifndef __ALPHA2_ACTIVE__
#define __ALPHA2_ACTIVE__

#include <picross/pic_usb.h>
#include <lib_alpha2/alpha2_usb.h>
#include <picross/pic_fastalloc.h>
#include <lib_alpha2/alpha2_exports.h>
#define KBD_KEYS 132

#define KBD_DESENSE (KBD_KEYS+0)
#define KBD_BREATH1 (KBD_KEYS+1)
#define KBD_BREATH2 (KBD_KEYS+2)
#define KBD_STRIP1  (KBD_KEYS+3)
#define KBD_STRIP2  (KBD_KEYS+4)
#define KBD_ACCEL   (KBD_KEYS+5)
#define KBD_SENSORS 6

#define TAU_KBD_KEYS 84 // normal keys(84)  
#define TAU_KEYS_OFFSET 5
#define TAU_MODE_KEYS   8
#define TAU_KBD_DESENSE (TAU_KBD_KEYS+0)
#define TAU_KBD_BREATH1 (TAU_KBD_KEYS+1)
#define TAU_KBD_BREATH2 (TAU_KBD_KEYS+2)
#define TAU_KBD_STRIP1  (TAU_KBD_KEYS+3)
#define TAU_KBD_ACCEL   (TAU_KBD_KEYS+4)

#define AUDIO_RATE_48 1 // 512
#define AUDIO_RATE_96 2 // 256
#define AUDIO_RATE_44 3 // 557

namespace alpha2
{
    class ALPHA2_DECLSPEC_CLASS active_t: virtual public pic::lckobject_t, public pic::pollable_t
    {
        public:
            struct impl_t;

        public:
            struct delegate_t
            {
                virtual ~delegate_t() {}
                virtual void kbd_dead(unsigned reason) {}
                virtual void kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4) {}
                virtual void kbd_mic(unsigned char s,unsigned long long t, const float *samples) {}
                virtual void kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y) {}
                virtual void kbd_keydown(unsigned long long t, const unsigned short *bitmap) {}
                virtual void pedal_down(unsigned long long t, unsigned pedal, unsigned p) {}
                virtual void midi_data(unsigned long long t, const unsigned char *data, unsigned len) {}
            };

            inline static unsigned word2key(unsigned word) { return word*16; }
            inline static unsigned short key2word(unsigned key) { return key/16; }
            inline static unsigned short key2mask(unsigned key) { return (1<<(key%16)); }
            inline static bool keydown(unsigned key, const unsigned short *bitmap) { return (bitmap[key2word(key)]&key2mask(key))!=0; }

        public:
            active_t(pic::usbdevice_t *device, delegate_t *, bool legacy_mode=false);
            ~active_t();

            void start();
            void stop();
            void invalidate();
            bool poll(unsigned long long t);

            void set_raw(bool raw);
            unsigned get_temperature();

            const char *get_name();

            // The following messages use the device's bulk out endpoint.  Multiple
            // messages are buffered up and sent in fixed sized packets.  To flush
            // the buffer and send the last messages, call msg_flush().
            void msg_set_led(unsigned key, unsigned colour);
            void msg_set_calibration(unsigned key, unsigned corner, unsigned short min, unsigned short max, const unsigned short *table);
            void msg_write_calibration();
            void msg_clear_calibration();
            void msg_test_write_lib(unsigned idx, unsigned p, unsigned r, unsigned y);
            void msg_test_finish_lib();
            void msg_test_write_seq(unsigned lib, unsigned key, unsigned time);
            void msg_test_finish_seq();
            void msg_test_start();
            void msg_flush();
            void msg_send_midi(const unsigned char *data, unsigned len);

            // period is 0x01 for 44.1/512, 0x02 for 48/512, 0x03 for 96/512
            void audio_write(const float *stereo, unsigned len, unsigned period);

            void set_tau_mode(bool);

            void mic_type(unsigned);
            void mic_gain(unsigned);
            void mic_pad(bool);
            void mic_enable(bool);
            void mic_suppress(bool);
            void headphone_enable(bool);
            void headphone_limit(bool);
            void headphone_gain(unsigned);
            void set_active_colour(unsigned);
            void loopback_enable(bool);
            void loopback_gain(float);
            void mic_automute(bool);
            void debounce_time(unsigned long);
            void threshold_time(unsigned long long);
            void key_threshold(unsigned);
            void key_noise(unsigned);

            void restart();
            
        private:
            impl_t *_impl;
    };

    struct ALPHA2_DECLSPEC_CLASS printer_t : active_t::delegate_t
    {
        void kbd_dead(unsigned reason);
        void kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4);
        void kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y);
        void kbd_keydown(unsigned long long t, const unsigned short *bitmap);
        void pedal_down(unsigned long long t, unsigned pedal, unsigned p);
        void midi_data(unsigned long long t, const unsigned char *data, unsigned len);
    };
};

#endif
