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

#ifndef __PIC_USB_H__
#define __PIC_USB_H__

#include "pic_exports.h"
#include "pic_functor.h"
#include "pic_error.h"
#include <string>

#define PIC_USB_FRAME_HISTORY 20

#define PIPE_OK 0
#define PIPE_UNKNOWN_ERROR 1
#define PIPE_NO_BANDWIDTH 2
#define PIPE_NOT_RESPONDING 3
#define PIPE_BAD_DRIVER 4

namespace pic
{
    class PIC_DECLSPEC_CLASS usbenumerator_t
    {
        public:
            struct impl_t;

        public:
            usbenumerator_t(unsigned short vendor, unsigned short product, const f_string_t &add);
            usbenumerator_t(unsigned short vendor, unsigned short product, const f_string_t &add, const f_string_t &del);
            ~usbenumerator_t();

            void start();
            void stop();

            int gc_traverse(void *,void *) const;
            int gc_clear();

        public:
            static unsigned enumerate(unsigned short vendor, unsigned short product, const f_string_t &);
            static std::string find(unsigned short vendor, unsigned short product, bool hurl=true);

        private:
            impl_t *impl_;
    };

   class PIC_DECLSPEC_CLASS usbdevice_t
    {
        public:
            struct impl_t;

            struct PIC_DECLSPEC_CLASS iso_in_pipe_t
            {
                iso_in_pipe_t(unsigned name, unsigned size): name_(name), size_(size), fnum_(0),ftime_(0),history_(0),trigger_(0),frame_check_(true) {}
                virtual ~iso_in_pipe_t() {}
                virtual void in_pipe_data(const unsigned char *frame, unsigned size, unsigned long long fnum, unsigned long long htime, unsigned long long ptime) = 0;

                void enable_frame_check(bool e) { frame_check_ = e; }

                unsigned in_pipe_name() { return name_; }
                unsigned in_pipe_size() { return size_; }
                void call_pipe_data(const unsigned char *frame, unsigned size, unsigned long long fnum, unsigned long long htime,unsigned long long ptime);
                void dump_history();

            private:
                unsigned name_;
                unsigned size_;
                unsigned long long fnum_;
                unsigned long long ftime_;
                unsigned long long hist_fnum_[PIC_USB_FRAME_HISTORY];
                unsigned long long hist_ftime_[PIC_USB_FRAME_HISTORY];
                unsigned long history_;
                unsigned trigger_;
                bool frame_check_;
            };

            struct PIC_DECLSPEC_CLASS power_t
            {
                virtual ~power_t() {}
                virtual void pipe_died(unsigned reason) {}
                virtual void pipe_started() {}
                virtual void pipe_stopped() {}
            };

            struct PIC_DECLSPEC_CLASS iso_out_pipe_t
            {
                iso_out_pipe_t(unsigned name, unsigned size): name_(name), size_(size) {}
                unsigned out_pipe_name() { return name_; }
                unsigned out_pipe_size() { return size_; }
            private:
                unsigned name_;
                unsigned size_;
            };

            struct PIC_DECLSPEC_CLASS bulk_out_pipe_t
            {
                bulk_out_pipe_t(unsigned name, unsigned size): name_(name), size_(size) {}
                unsigned out_pipe_name() { return name_; }
                unsigned out_pipe_size() { return size_; }
                void bulk_write(const void *data, unsigned len, unsigned timeout=500);
                void bulk_write(const std::string &data);

                struct impl_t;

            private:
                friend struct impl_t;
                unsigned name_;
                unsigned size_;
                impl_t *impl_;
            };

            class PIC_DECLSPEC_CLASS iso_out_guard_t: public pic::nocopy_t
            {
                public:
                    iso_out_guard_t(usbdevice_t *device);
                    ~iso_out_guard_t();
                    bool isvalid() const { return current_!=0; }
                    void dirty() { dirty_ = true; }
                    unsigned char *current() { return current_; }
                    unsigned char *advance();
                private:
                    usbdevice_t::impl_t *impl_;
                    unsigned char *current_;
#ifndef PI_MACOSX
                    void *guard_;
#endif
                    bool dirty_;
            };

        public:
            usbdevice_t(const char *name, unsigned iface);
            const char *name();
            virtual ~usbdevice_t();

            void start_pipes();
            void stop_pipes();

            // currently, supporting only one client with multiple pipes, possibly
            // a power notification delegate.  detach() clears them all out.
            bool add_iso_in(iso_in_pipe_t *);
            void set_iso_out(iso_out_pipe_t *);
            bool add_bulk_out(bulk_out_pipe_t *);

            void set_power_delegate(power_t *);
            void detach();
            void close();

            bool poll_pipe(unsigned long long t);

            void control(unsigned char type, unsigned char request, unsigned short value, unsigned short index, unsigned timeout=500);
            void control_out(unsigned char type, unsigned char request, unsigned short value, unsigned short index, const void *data, unsigned len, unsigned timeout=500);
            void control_out(unsigned char type, unsigned char request, unsigned short value, unsigned short index, const std::string &s);

            void control_in(unsigned char type, unsigned char request, unsigned short value, unsigned short index, void *data, unsigned len, unsigned timeout=500);
            std::string control_in(unsigned char type, unsigned char request, unsigned short value, unsigned short index, unsigned len);

            impl_t *impl() { return impl_; }

        private:
            impl_t *impl_;
    };

    struct pollable_t
    {
        virtual ~pollable_t() {}
        virtual bool poll(unsigned long long) = 0;
    };

    class PIC_DECLSPEC_CLASS poller_t: public nocopy_t
    {
        public:
            poller_t(pollable_t *pollable);
            ~poller_t();
            void start_polling();
            void stop_polling();

            struct impl_t;
        private:
            impl_t *impl_;
    };

    class PIC_DECLSPEC_CLASS bulk_queue_t
    {
        public:
            bulk_queue_t(unsigned size,usbdevice_t *dev, unsigned name, unsigned timeout, unsigned auto_flush);
            ~bulk_queue_t();
            void write(const void *data, unsigned len);
            void flush();
            void start();
            void stop();
        private:
            struct impl_t;
        private:
            impl_t *impl_;
    };

    PIC_DECLSPEC_FUNC(std::string) usb_string(usbdevice_t *device, unsigned index);
    PIC_DECLSPEC_FUNC(std::string) usb_serial(usbdevice_t *device);
    PIC_DECLSPEC_FUNC(std::string) usb_product(usbdevice_t *device);
};

#endif
