
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


#include <picross/pic_usb.h>
#include <picross/pic_log.h>
#include <picross/pic_time.h>
#include <picross/pic_safeq.h>
#include <cstring>

struct finder_t: virtual pic::tracked_t
{
    finder_t(): found_(false) {}
    void found(const std::string &device) { device_=device; found_=true; }
    std::string device_;
    bool found_;
};


std::string pic::usbenumerator_t::find(unsigned short vendor, unsigned short product, bool hurl)
{
    finder_t f;
    enumerate(vendor,product,pic::f_string_t::method(&f,&finder_t::found));

    if(f.found_)
        return f.device_;

    if(hurl)
        pic::hurlmsg() << "usb enumerator timed out";

    return "";
}

void pic::usbdevice_t::iso_in_pipe_t::dump_history()
{
    unsigned l = std::min(history_,(unsigned long)PIC_USB_FRAME_HISTORY);

    unsigned long xf = hist_fnum_[(history_-l)%PIC_USB_FRAME_HISTORY];
    unsigned long long xt = hist_ftime_[(history_-l)%PIC_USB_FRAME_HISTORY];
    unsigned long f;
    unsigned long long t;

    for(unsigned i=0;i<l;i++)
    {
        unsigned k = (history_-l+i)%PIC_USB_FRAME_HISTORY;
        f = hist_fnum_[k];
        t = hist_ftime_[k];

        int fd = (f>xf)?(f-xf):(-(int)(xf-f));
        int td = (t>xt)?(t-xt):(-(int)(xt-t));

        pic::logmsg() << "frame: " << (f/8) << ":" << (f%8) << " time: " << t << " fdelta: " << fd << " tdelta: " << td;

        xf = f;
        xt = t;
    }
}

void pic::usbdevice_t::iso_in_pipe_t::call_pipe_data(const unsigned char *frame, unsigned size, unsigned long long fnum, unsigned long long ftime,unsigned long long ptime)
{
    hist_fnum_[history_%PIC_USB_FRAME_HISTORY] = fnum;
    hist_ftime_[history_%PIC_USB_FRAME_HISTORY] = ftime;
    history_++;

    if(trigger_>0)
    {
        if(--trigger_==0)
        {
            dump_history();
        }
    }

    if(fnum&&ftime)
    {
        if(fnum <= fnum_)
        {
            pic::logmsg() << "pipe " << name_ << " out of order frame " << fnum << " (received after " << fnum_ << ")";

            if(!trigger_)
                trigger_ = PIC_USB_FRAME_HISTORY/2;

            fnum_=fnum;
            ftime_=ftime;
            return;
        }

        if(ftime <= ftime_)
        {
#if 0
            pic::logmsg() << "pipe " << name_ << " out of order time " << ftime << " (received after " << ftime_ << ")";

            if(!trigger_)
                trigger_ = PIC_USB_FRAME_HISTORY/2;

#endif
            
            ftime = ftime_+1;
        }

        fnum_=fnum;
        ftime_=ftime;
    }

    in_pipe_data(frame,size,fnum,ftime,ptime);
}

struct pic::poller_t::impl_t:  pic::thread_t
{
    impl_t(pic::pollable_t *pipe): pipe_(pipe), quit_(0)
    {
    }

    void thread_main()
    {
#ifdef DEBUG_DATA_ATOMICITY
        std::cout << "Started USB poller thread with ID " << pic_current_threadid() << std::endl;
#endif
 
        while(!quit_)
        {
            pipe_->poll(0);
            pic_microsleep(10000);
        }
    }

    pic::pollable_t *pipe_;
    pic_atomic_t quit_;
};

pic::poller_t::poller_t(pic::pollable_t *dev): impl_(new impl_t(dev))
{
}

pic::poller_t::~poller_t()
{
    delete impl_;
}

void pic::poller_t::start_polling()
{
    impl_->run();
}

void pic::poller_t::stop_polling()
{
    impl_->quit_ = 1;
    impl_->wait();
}

void pic::usbdevice_t::control_out(unsigned char type, unsigned char request, unsigned short value, unsigned short index, const std::string &s)
{
    control_out(type,request,value,index,(const void *)s.c_str(),s.size(),10000);
}

std::string pic::usbdevice_t::control_in(unsigned char type, unsigned char request, unsigned short value, unsigned short index, unsigned len)
{
    PIC_ASSERT(len<1024);
    unsigned char b[1024];
    control_in(type,request,value,index,(void *)b,len);
    return std::string((char *)b,len);
}

void pic::usbdevice_t::bulk_out_pipe_t::bulk_write(const std::string &data)
{
    bulk_write((const void *)data.c_str(),data.size(),500);
}

struct pic::bulk_queue_t::impl_t: pic::safe_worker_t, pic::usbdevice_t::bulk_out_pipe_t
{
    impl_t(unsigned size,usbdevice_t *dev, unsigned name, unsigned timeout, unsigned auto_flush);
    ~impl_t();
    void write(const void *data, unsigned len);
    void flush();
    static void writer__(void *self_, void *buffer_, void *size_, void *);
    bool ping();

    unsigned size_;
    mutex_t lock_;
    usbdevice_t *dev_;
    unsigned name_;
    unsigned timeout_;
    unsigned char *buffer_;
    unsigned count_;
};

pic::bulk_queue_t::bulk_queue_t(unsigned size,usbdevice_t *dev, unsigned name, unsigned timeout, unsigned auto_flush)
{
    impl_ = new impl_t(size,dev,name,timeout,auto_flush);
}

pic::bulk_queue_t::~bulk_queue_t()
{
    delete impl_;
}

void pic::bulk_queue_t::write(const void *data, unsigned len)
{
    mutex_t::guard_t g(impl_->lock_);
    impl_->write(data,len);
}

void pic::bulk_queue_t::flush()
{
    mutex_t::guard_t g(impl_->lock_);
    impl_->flush();
}

void pic::bulk_queue_t::start()
{
    impl_->run();
}

void pic::bulk_queue_t::stop()
{
    impl_->quit();
}

pic::bulk_queue_t::impl_t::impl_t(unsigned size,usbdevice_t *dev, unsigned name, unsigned timeout,unsigned auto_flush): pic::safe_worker_t(auto_flush,0), bulk_out_pipe_t(name,size), size_(size), dev_(dev), name_(name), timeout_(timeout), count_(0)
{
    PIC_ASSERT(dev->add_bulk_out(this));
    buffer_ = (unsigned char *)nb_malloc(PIC_ALLOC_NB,size_);
    PIC_ASSERT(buffer_);
    memset(buffer_,0,size_);
}

bool pic::bulk_queue_t::impl_t::ping()
{
    mutex_t::guard_t g(lock_);
    flush();
    return false;
}

pic::bulk_queue_t::impl_t::~impl_t()
{
    pic::nb_free(buffer_);
}

void pic::bulk_queue_t::impl_t::write(const void *data, unsigned len)
{
    if(buffer_)
    {
        if((count_+len) > size_)
        {
            flush();
        }

        memcpy(buffer_+count_,data,len);
        count_ += len;
    }
}

void pic::bulk_queue_t::impl_t::writer__(void *self_, void *buffer_, void *size_, void *)
{
    impl_t *self = (impl_t *)self_;
    unsigned size = (unsigned)(intptr_t)size_;
    try
    {
        //pic::logmsg() << "bulk write ep=" << self->name_ << " len=" << size;
        pic_microsleep(1000);
        self->bulk_write(buffer_,size,self->timeout_);
    }
    CATCHLOG()
    nb_free(buffer_);
}

void pic::bulk_queue_t::impl_t::flush()
{
    if(buffer_)
    {
        if(count_>0)
        {
            add(writer__,this,buffer_,(void *)(long) count_,0);
            buffer_ = (unsigned char *)nb_malloc(PIC_ALLOC_NB,size_);
            count_ = 0;
            if(buffer_)
                memset(buffer_,0,size_);
        }
    }
}
