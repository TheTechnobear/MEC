
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

#include <stdlib.h>
#include <memory>
#include <iostream>
#include <cstdio>

#include <picross/pic_error.h>
#include <picross/pic_log.h>
#include <picross/pic_cfrunloop.h>
#include <picross/pic_usb.h>
#include <picross/pic_atomic.h>
#include <picross/pic_fastalloc.h>
#include <picross/pic_time.h>

#include <CoreServices/CoreServices.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <CoreFoundation/CoreFoundation.h>

#define ISO_IN_URBS        80
#define ISO_IN_URBSIZE     8
#define ISO_IN_FUTURE      50
#define ISO_LOW            40
#define MICROFRAMES_PER_FRAME 8
#define MICROFRAME_INTERVAL (1000/MICROFRAMES_PER_FRAME)

#define ISO_OUT_URBS        80
#define ISO_OUT_FRAMES_PER_URB     4 
#define ISO_OUT_OFFSET       4 /* ms (frames)*/
#define ISO_OUT_FUTURE       64 /* microframes */

#define OUTPIPE_IDLE 0
#define OUTPIPE_BUSY 1
#define OUTPIPE_START 2
#define OUTPIPE_SHUTDOWN 3

#define SUBMIT_OK 0
#define SUBMIT_UNKNOWN_ERROR 1 
#define SUBMIT_NO_BANDWIDTH 2

// top 4 bits of device state is the status, bottom 12 inflight urb count
// run together so we can do atomic changes of both at once.

#define PIPES_STOPPED  0x0000  // not running.
#define PIPES_IDLE     0x1000  // idle state.  pipe_started not called, no urbs in flight, will go to running on poll
#define PIPES_RUNNING  0x2000  // running state.  pipe_started called, urbs in progress
#define PIPES_KILLED   0x3000  // pipe_died called, no urbs in flight
#define PIPES_SLEEP    0x4000

#define PIPES_STATE_MASK 0xf000
#define PIPES_COUNT_MASK 0x0fff

namespace
{
    int find_bulk_pipe(IOUSBInterfaceInterface197 **iface, unsigned name)
    {
        UInt8 ne;
        int wdir = (name>=0x80)?1:0;
        int wnum = name&0x7f;

        (*iface)->GetNumEndpoints(iface,&ne);

        for(unsigned i=0;i<=ne;i++)
        {
            UInt8 dir, num, type, time; UInt16 size;
            (*iface)->GetPipeProperties(iface,i,&dir,&num,&type,&size,&time);

            if(num==wnum && dir==wdir && type==kUSBBulk)
            {
                return (int)i;
            }
        }

        return -1;
    }

    int find_iso_pipe(IOUSBInterfaceInterface197 **iface, unsigned name, unsigned *uframe_per_frame = 0)
    {
        UInt8 ne;
        int wdir = (name>=0x80)?1:0;
        int wnum = name&0x7f;

        (*iface)->GetNumEndpoints(iface,&ne);

        for(unsigned i=0;i<=ne;i++)
        {
            UInt8 dir, num, type, time; UInt16 size;
            (*iface)->GetPipeProperties(iface,i,&dir,&num,&type,&size,&time);

            if(num==wnum && dir==wdir && type==kUSBIsoc)
            {
                if(uframe_per_frame)
                {
                    *uframe_per_frame = 8/(1<<(time-1));
                }

                return (int)i;
            }
        }

        return -1;
    }

    IOUSBInterfaceInterface197 **open_interface(IOUSBDeviceInterface197 **device, unsigned iface)
    {
        IOReturn e;
        UInt8 num_config=0;
        IOUSBConfigurationDescriptorPtr config_desc;
        IOUSBFindInterfaceRequest request;
        io_iterator_t ii;
        IOUSBInterfaceInterface197 **interface;
        io_service_t is;
        SInt32 score;
        IOCFPlugInInterface **plug;
        UInt8 id;

        e = (*device)->USBDeviceOpen(device);
        if(e==kIOReturnExclusiveAccess)
        {
            PIC_THROW("can't open: already opened by someone else");
        }
        PIC_ASSERT(e==kIOReturnSuccess);

        e = (*device)->GetNumberOfConfigurations(device, &num_config);

        if(!num_config)
        {
            (*device)->USBDeviceClose(device);
            PIC_THROW("device has no configurations!");
        }

        if((e = (*device)->GetConfigurationDescriptorPtr(device, 0, &config_desc)))
        {
            (*device)->USBDeviceClose(device);
            PIC_THROW("can't get default config descriptor");
        }

        if((e = (*device)->SetConfiguration(device, config_desc->bConfigurationValue)))
        {
            (*device)->USBDeviceClose(device);
            PIC_THROW("can't set default configuration");
        }

        request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
        request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
        request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
        request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

        if((e = (*device)->CreateInterfaceIterator(device, &request, &ii)))
        {
            (*device)->USBDeviceClose(device);
            PIC_THROW("can't enumerate interfaces");
        }

        while((is = IOIteratorNext(ii)))
        {
            e=IOCreatePlugInInterfaceForService(is, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plug, &score);
            IOObjectRelease(is);
            if(e || !plug) continue;
            (*plug)->QueryInterface(plug,CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID197), (void **)&interface);
            (*plug)->Stop(plug);
            IODestroyPlugInInterface(plug);
            (*interface)->GetInterfaceNumber(interface,&id);

            if(id==iface)
            {
                (*interface)->USBInterfaceOpen(interface);
                IOObjectRelease(ii);
                return interface;
            }

            (*interface)->Release(interface);
        }

        (*device)->USBDeviceClose(device);
        IOObjectRelease(ii);
        PIC_THROW("can't find interface");
    }

    void close_interface(IOUSBDeviceInterface197 **device, IOUSBInterfaceInterface197 **interface)
    {
        if(interface)
        {
            (*interface)->USBInterfaceClose(interface);
            (*interface)->Release(interface);
        }
        if(device)
        {
            (*device)->USBDeviceClose(device);
        }
    }

    void close_device(IOUSBDeviceInterface197 **device)
    {
        if(device)
        {
            (*device)->Release(device);
        }
    }

    unsigned device_enumerate(unsigned long v, unsigned long p, void (*visitor)(void *, IOUSBDeviceInterface197 **), void *arg)
    {
        CFMutableDictionaryRef  d;
        kern_return_t e;
        io_service_t s;
        io_iterator_t i;
        SInt32 score;
        IOCFPlugInInterface **plug;
        IOUSBDeviceInterface197 **device;
        unsigned count=0;
        mach_port_t port;

        if((e=IOMasterPort(MACH_PORT_NULL,&port)) || !port)
        {
            PIC_THROW("can't create mach port");
        }

        if(!(d = IOServiceMatching(kIOUSBDeviceClassName)))
        {
            mach_port_deallocate(mach_task_self(),port);
            PIC_THROW("can't create matching dict");
        }

        if(v)
        {
            CFDictionarySetValue(d, CFSTR(kUSBVendorName), CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &v));
            CFDictionarySetValue(d, CFSTR(kUSBProductName), CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &p));
        }

        if((e=IOServiceGetMatchingServices(port,d,&i)))
        {
            mach_port_deallocate(mach_task_self(),port);
            PIC_THROW("can't find any devices of class");
        }

        while((s=IOIteratorNext(i)))
        {
            e=IOCreatePlugInInterfaceForService(s, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plug, &score);
            IOObjectRelease(s);
            if(e || !plug) continue;
            (*plug)->QueryInterface(plug,CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID197), (void **)&device);
            (*plug)->Stop(plug);
            IODestroyPlugInInterface(plug);
            if(!device) continue;
            visitor(arg,device);
            (*device)->Release(device);
            count++;
        }

        IOObjectRelease(i);
        mach_port_deallocate(mach_task_self(),port);

        return count;
    }


    bool is_hispeed(IOUSBInterfaceInterface197 **i)
    {
        UInt32 us;
        if((*i)->GetFrameListTime(i,&us)!=kIOReturnSuccess)
        {
            PIC_THROW("cannot determine if this is a USB 2.0 device");
        }

        return us==kUSBHighSpeedMicrosecondsInFrame;
    }

    struct __finderdata
    {
        unsigned long name;
        IOUSBDeviceInterface197 **device;
    };

    static void __finder(void *impl_, IOUSBDeviceInterface197 **device)
    {
        __finderdata *f = (__finderdata *)impl_;
        unsigned int dl;

        (*device)->GetLocationID(device,&dl);

        if(dl==f->name)
        {
            (*device)->AddRef(device);
            f->device=device;
        }
    }

    IOUSBDeviceInterface197 **open_device(const char *name)
    {
        __finderdata f;

        f.name = strtoul(name,0,16);
        f.device=0;

        device_enumerate(0,0,__finder,&f);

        if(!f.device)
        {
            pic::msg() << "can't find device " << name << pic::hurl;
        }

        return f.device;
    }

    struct macosx_usbdevice_t
    {
        macosx_usbdevice_t(const char *, unsigned);
        ~macosx_usbdevice_t();

        void close();

        IOUSBDeviceInterface197 **device;
        IOUSBInterfaceInterface197 **interface;
        bool hispeed;
    };

    struct usburb_t: virtual public pic::lckobject_t
    {
        usburb_t();
        ~usburb_t();

        void *pipe;
        macosx_usbdevice_t *device;
        IOUSBLowLatencyIsocFrame *frame;
        unsigned char *buffer;
        UInt64 fnum;
    };

    struct macosx_usbpipe_t: virtual public pic::lckobject_t
    {
        macosx_usbpipe_t(pic::usbdevice_t::impl_t *impl,int piperef,unsigned size);
        virtual ~macosx_usbpipe_t() {}

        void clear_stall();
        void abort_urbs();

        pic::usbdevice_t::impl_t *impl_;
        int piperef_;
        unsigned size_;
        UInt64 counter_;
    };

    struct macosx_usbpipe_in_t: macosx_usbpipe_t
    {
        macosx_usbpipe_in_t(pic::usbdevice_t::impl_t *impl,pic::usbdevice_t::iso_in_pipe_t *pipe, int ref);
        ~macosx_usbpipe_in_t();

        void start();
        void service();

        void completed(usburb_t *, IOReturn);
        void submit(usburb_t *);
        void init_urb(usburb_t *);

        bool poll(unsigned long long t);

        pic::usbdevice_t::iso_in_pipe_t *pipe;

        usburb_t urbs[ISO_IN_URBS];

        unsigned long urb_queued_, urb_reaped_;
        unsigned micro;
        unsigned requests;

        unsigned long long polled_;
        pic::flipflop_t<unsigned long long> urb_reaped_lbound_;
        unsigned inflight_;
    };

    struct macosx_usbpipe_out_t: macosx_usbpipe_t
    {
        macosx_usbpipe_out_t(pic::usbdevice_t::impl_t *impl,pic::usbdevice_t::iso_out_pipe_t *pipe, int ref, unsigned uframe_per_frame);
        ~macosx_usbpipe_out_t();

        void completed(usburb_t *, IOReturn);
        void submit(usburb_t *);
        void init_urb(usburb_t *);
        void start();

        unsigned char *iso_write_start();
        unsigned char *iso_write_advance(unsigned n);

        pic::usbdevice_t::iso_out_pipe_t *pipe;

        usburb_t urbs[ISO_IN_URBS];
        unsigned write_urb_;
        unsigned write_frame_;
        unsigned uframe_per_frame_;
        unsigned inflight_;
    };

    typedef pic::lcklist_t<macosx_usbpipe_in_t *>::lcktype pipe_list_t;
    typedef pic::flipflop_t<pipe_list_t> pipe_flipflop_t;
};

struct pic::usbenumerator_t::impl_t: public pic::cfrunloop_t
{
    impl_t(unsigned short, unsigned short, const pic::f_string_t &, const pic::f_string_t &);
    ~impl_t();

    void runloop_init();
    void runloop_term();
    void runloop_start();

    static void device_attached(void *, io_iterator_t);

    SInt32 vendor_;
    SInt32 product_;
    pic::f_string_t delegate_;
    pic::f_string_t gone_;
    pic::mutex_t lock_;

    IONotificationPortRef notification_port_;
    CFRunLoopSourceRef notification_source_;

    io_iterator_t iterator_;
    io_object_t notification_;
};

struct pic::usbdevice_t::impl_t: pic::cfrunloop_t, virtual public pic::lckobject_t
{
    impl_t(const char *, unsigned);
    ~impl_t();

    void runloop_init();
    void runloop_term();
    int runloop_cmd(void *c, void *a);
    void start_pipes();
    void stop_pipes();
    void detach();
    void close();

    void power_sleep();
    void power_wakeup();

    void pipe_idle();
    void pipe_sleep();
    void pipe_kill(unsigned reason);
    void pipe_running();

    void inc_inflight();
    void dec_inflight();
    void call_pipe_running();
    void call_pipe_stopped();
    void call_pipe_died(unsigned reason);

    bool poll_pipe(unsigned long long t);

    bool add_bulk_out(bulk_out_pipe_t *);
    bool add_iso_in(iso_in_pipe_t *);
    void set_iso_out(iso_out_pipe_t *);

    void ping(void *a, void *b);
    void abort_urbs(int piperef);
    void abort_urbs();
    void reset_device();

    std::string name_;

    macosx_usbdevice_t device;
    pic::usbdevice_t::power_t *power;

    CFRunLoopSourceRef source;

    IONotificationPortRef power_notify;
    CFRunLoopSourceRef power_source;
    io_object_t power_iterator;
    io_connect_t power_port;

    pipe_flipflop_t inpipes_;
    macosx_usbpipe_out_t *outpipe;
    pic_atomic_t cmd_pending;

    unsigned state_;
};

struct pic::usbdevice_t::bulk_out_pipe_t::impl_t
{
    impl_t(pic::usbdevice_t::impl_t *device, pic::usbdevice_t::bulk_out_pipe_t *p): device_(device), pipe_(p)
    {
        p->impl_ = this;

        if((ref_=find_bulk_pipe(device_->device.interface,pipe_->out_pipe_name()))<0)
        {
            pic::msg() << "can't find bulk endpoint " << pipe_->out_pipe_name() << pic::hurl;
        }
    }

    void bulk_write(const void *data, unsigned len, unsigned timeout);

    pic::usbdevice_t::impl_t *device_;
    pic::usbdevice_t::bulk_out_pipe_t *pipe_;
    int ref_;
};

macosx_usbpipe_t::macosx_usbpipe_t(pic::usbdevice_t::impl_t *impl,int piperef,unsigned size): impl_(impl), piperef_(piperef), size_(size), counter_(0)
{
}

void macosx_usbpipe_t::clear_stall()
{
    IOUSBInterfaceInterface197 **intf = impl_->device.interface;
    IOReturn e;

    if((e=(*intf)->ClearPipeStallBothEnds(intf,piperef_)) != kIOReturnSuccess)
    {
        pic::logmsg() << "clear stall failed: 0x" << std::hex << e;
    }
}

void macosx_usbpipe_t::abort_urbs()
{
    impl_->abort_urbs(piperef_);
}

usburb_t::usburb_t(): pipe(0),device(0),frame(0),buffer(0),fnum(0)
{
}

void macosx_usbpipe_in_t::init_urb(usburb_t *u)
{
    IOUSBInterfaceInterface197 **i = impl_->device.interface;
    IOReturn e;

    e=(*i)->LowLatencyCreateBuffer(i,(void **)&u->frame,requests*sizeof(IOUSBLowLatencyIsocFrame),kUSBLowLatencyFrameListBuffer);

    if (e!=kIOReturnSuccess)
    {
        pic::msg() << "LowLatencyCreateBuffer: allocing frame list: " << requests*sizeof(IOUSBLowLatencyIsocFrame) << " 0x" << std::hex << e << pic::hurl;
    }

    e=(*i)->LowLatencyCreateBuffer(i,(void **)&u->buffer,requests*size_,kUSBLowLatencyReadBuffer);

    if (e!=kIOReturnSuccess)
    {
        (*i)->LowLatencyDestroyBuffer(i,u->frame);
        pic::msg() << "LowLatencyCreateBuffer: allocing read buffer: " << requests*size_ << " 0x" << std::hex << e << pic::hurl;
    }

    u->pipe=this;
    u->device=&impl_->device;
}

void macosx_usbpipe_out_t::init_urb(usburb_t *u)
{
    IOUSBInterfaceInterface197 **i = impl_->device.interface;
    IOReturn e;

    e=(*i)->LowLatencyCreateBuffer(i,(void **)&u->frame,ISO_OUT_FRAMES_PER_URB*uframe_per_frame_*sizeof(IOUSBLowLatencyIsocFrame),kUSBLowLatencyFrameListBuffer);

    if (e!=kIOReturnSuccess)
    {
        pic::msg() << "LowLatencyCreateBuffer: allocing frame list: " << ISO_OUT_FRAMES_PER_URB*uframe_per_frame_*sizeof(IOUSBLowLatencyIsocFrame) << " 0x" << std::hex << e << pic::hurl;
    }

    e=(*i)->LowLatencyCreateBuffer(i,(void **)&u->buffer,ISO_OUT_FRAMES_PER_URB*uframe_per_frame_*size_,kUSBLowLatencyWriteBuffer);

    if (e!=kIOReturnSuccess)
    {
        (*i)->LowLatencyDestroyBuffer(i,u->frame);
        pic::msg() << "LowLatencyCreateBuffer: allocing write buffer: " << ISO_OUT_FRAMES_PER_URB*uframe_per_frame_*size_ << " 0x" << std::hex << e << pic::hurl;
    }

    u->pipe=this;
    u->device=&impl_->device;
}

usburb_t::~usburb_t()
{
    if(device)
    {
        IOUSBInterfaceInterface197 **i = device->interface;

        (*i)->LowLatencyDestroyBuffer(i,frame);
        (*i)->LowLatencyDestroyBuffer(i,buffer);
    }
}

static void in_completed__(void *urb_, IOReturn e, void *)
{
    usburb_t *urb = (usburb_t *)urb_;
    macosx_usbpipe_in_t *p = (macosx_usbpipe_in_t *)urb->pipe;
    p->completed(urb,e);
}

static void out_completed__(void *urb_, IOReturn e, void *)
{
    usburb_t *urb = (usburb_t *)urb_;
    macosx_usbpipe_out_t *p = (macosx_usbpipe_out_t *)urb->pipe;
    p->completed(urb,e);
}

void macosx_usbpipe_in_t::submit(usburb_t *u)
{
    IOUSBInterfaceInterface197 **intf = impl_->device.interface;
    IOReturn e;

    if((impl_->state_&PIPES_STATE_MASK) !=PIPES_RUNNING)
    {
        return;
    }

    for(unsigned i=0;i<requests;i++)
    {
        u->frame[i].frStatus=kUSBLowLatencyIsochTransferKey;
        u->frame[i].frReqCount=size_;
        u->frame[i].frActCount=0;
    }

restart:

    u->fnum = counter_;

    if((e=(*intf)->LowLatencyReadIsochPipeAsync(intf,piperef_,u->buffer,u->fnum/micro,requests,1,u->frame,in_completed__,u)) != kIOReturnSuccess)
    {
        if(e==kIOReturnIsoTooOld)
        {
            AbsoluteTime time;
            UInt64 f;
            (*intf)->GetBusFrameNumber(intf,&f,&time);
            f+=ISO_IN_FUTURE;
            counter_ = f*micro;
            pic::logmsg() << "fell behind: frame count resynced to " << counter_;
            goto restart;
        }
        else if(e==kIOReturnNoBandwidth)
        {
            pic::logmsg() << "can't submit read request due to unsufficient bandwidth 0x" << std::hex << e;
            impl_->pipe_kill(PIPE_NO_BANDWIDTH);
            return;
        }

        pic::logmsg() << "can't submit read request 0x" << std::hex << e;
        impl_->pipe_kill(PIPE_UNKNOWN_ERROR);
        return;
    }

    counter_+=requests;
    urb_queued_++;
    inflight_++;
    impl_->inc_inflight();
}

void macosx_usbpipe_in_t::service()
{
    unsigned long reaped = urb_reaped_;

    while((impl_->state_&PIPES_STATE_MASK)==PIPES_RUNNING && urb_queued_<reaped+ISO_IN_URBS)
    {
        usburb_t *urb = &urbs[urb_queued_%ISO_IN_URBS];
        submit(urb);
    }

    if(inflight_==1)
    {
        pic::logmsg() << "pipe starvation";
        impl_->pipe_idle();
    }
}

void pic::usbdevice_t::impl_t::abort_urbs(int pipe)
{
    IOUSBInterfaceInterface197 **intf = device.interface;
    (*intf)->AbortPipe(intf,pipe);
}

void macosx_usbpipe_in_t::start()
{
    pic::logmsg() << "in pipe starting...";
    urb_queued_=0;
    urb_reaped_=0;
    counter_=0;
    pic::logmsg() << "in pipe started.";
}

void macosx_usbpipe_out_t::start()
{
    pic::logmsg() << "out pipe starting...";
    counter_=0;

    for(unsigned i=0;i<ISO_OUT_URBS;i++)
    {
        submit(&urbs[i]);
    }

    pic::logmsg() << "out pipe started.";
}

bool pic::usbdevice_t::poll_pipe(unsigned long long time)
{
    return impl_->poll_pipe(time);
}

void pic::usbdevice_t::impl_t::abort_urbs()
{
    pipe_flipflop_t::guard_t g(inpipes_);
    pipe_list_t::const_iterator i;

    for(i=g.value().begin(); i!=g.value().end(); i++)
    {
        (*i)->abort_urbs();
    }

    if(outpipe)
    {
        outpipe->abort_urbs();
    }
}

bool pic::usbdevice_t::impl_t::poll_pipe(unsigned long long time)
{
    pipe_running();

    pipe_flipflop_t::guard_t g(inpipes_);
    pipe_list_t::const_iterator i;
    bool skipped = false;

    for(i=g.value().begin(); i!=g.value().end(); i++)
    {
        if((*i)->poll(time))
        {
            skipped=true;
        }
    }

    return skipped;
}

bool macosx_usbpipe_in_t::poll(unsigned long long t)
{
    pic::flipflop_t<unsigned long long>::guard_t reaped_guard(urb_reaped_lbound_);

restart:
    unsigned long queued = urb_queued_;
    unsigned long long reaped_lbound = reaped_guard.value();

    if((impl_->state_&PIPES_STATE_MASK)!=PIPES_RUNNING)
    {
        urb_reaped_ = urb_queued_;
        return false;
    }

    if(urb_reaped_<reaped_lbound)
    {
        urb_reaped_ = reaped_lbound;
    }

    if(urb_reaped_>=queued)
    {
        return false;
    }

    usburb_t *urb = &urbs[urb_reaped_%ISO_IN_URBS];

    if(polled_>=urb->fnum+requests)
    {
        pic::logmsg() << "delayed urb";
        urb_reaped_++;
        goto restart;
    }

    if(urb->fnum>polled_)
    {
        pic::logmsg() << "skipped urb";
        polled_ = urb->fnum;
        return true;
    }

    unsigned i = polled_-urb->fnum;
    unsigned char *b = urb->buffer+i*size_;
    unsigned long long f0 = UnsignedWideToUInt64(AbsoluteToNanoseconds(urb->frame[0].frTimeStamp))/1000ULL;

    for(; i<requests; ++i,b+=size_,++polled_)
    {
        IOReturn s = urb->frame[i].frStatus;

        if(s==kUSBLowLatencyIsochTransferKey)
        {
            return false;
        }

        if(s!=kIOReturnSuccess && s!=kIOReturnUnderrun && s!=kIOReturnOverrun)
        {
            if(s==kIOReturnNotResponding)
            {
                pic::logmsg() << "fatal error in poll, killing pipe";
                impl_->pipe_kill(PIPE_NOT_RESPONDING);
            }

            polled_ = urb->fnum+requests;
            urb_reaped_++;

            pic::logmsg() << "skip aborted frames";
            return true;
        }

        unsigned long long ft = f0+(i*MICROFRAME_INTERVAL);

        if(t!=0ULL && ft>t)
        {
            return false;
        }

        if(urb->frame[i].frActCount>0)
        {
            pic::flipflop_t<unsigned long long>::unguard_t ug(reaped_guard);
            pipe->call_pipe_data(b,urb->frame[i].frActCount,urb->fnum+i,ft,t);
        }

        if(reaped_guard.value()!=reaped_lbound)
        {
            goto restart;
        }
    }

    urb_reaped_++;
    goto restart;
}

void macosx_usbpipe_in_t::completed(usburb_t *urb, IOReturn result)
{
    --inflight_;
    impl_->dec_inflight();

    if((impl_->state_&PIPES_STATE_MASK)==PIPES_RUNNING)
    {
        switch(result)
        {
            case kIOReturnSuccess:
            case kIOReturnUnderrun:
            case kIOReturnOverrun:
            case kIOReturnAborted:
                break;

            case kIOReturnIsoTooOld:
            case kIOUSBNotSent1Err:
            case kIOUSBNotSent2Err:
            case kIOReturnNoBandwidth:
                pic::logmsg() << "urb ignored with error 0x" << std::hex << result;
                break;

            case kIOUSBWrongPIDErr:
                pic::logmsg() << "pipe stall 0x" << std::hex << result;
                impl_->pipe_idle();
                break;

            default:
                pic::logmsg() << "urb completed with error 0x" << std::hex << result;
                impl_->pipe_kill(PIPE_UNKNOWN_ERROR);
                break;
        }
    }

    service();
}

macosx_usbpipe_in_t::macosx_usbpipe_in_t(pic::usbdevice_t::impl_t *impl,pic::usbdevice_t::iso_in_pipe_t *p,int r): 
    macosx_usbpipe_t(impl,r,p->in_pipe_size()),
    pipe(p), urb_queued_(0), urb_reaped_(0), polled_(0), urb_reaped_lbound_(0ULL), inflight_(0)
{
    micro = impl_->device.hispeed?MICROFRAMES_PER_FRAME:1;
    requests = micro*ISO_IN_URBSIZE;

    for(unsigned i=0;i<ISO_IN_URBS;i++)
    {
        init_urb(&urbs[i]);
    }
}

macosx_usbpipe_out_t::macosx_usbpipe_out_t(pic::usbdevice_t::impl_t *impl,pic::usbdevice_t::iso_out_pipe_t *p,int r,unsigned uframe_per_frame): 
    macosx_usbpipe_t(impl,r,p->out_pipe_size()),
    pipe(p), write_urb_(0), write_frame_(0), uframe_per_frame_(uframe_per_frame), inflight_(0)
{
    PIC_ASSERT(impl_->device.hispeed);
    pic::logmsg() << "output pipe " << r << " uframe_per_frame " << uframe_per_frame;

    for(unsigned i=0;i<ISO_OUT_URBS;i++)
    {
        init_urb(&urbs[i]);
    }
}

macosx_usbpipe_out_t::~macosx_usbpipe_out_t()
{
}

void macosx_usbpipe_out_t::completed(usburb_t *u, IOReturn result)
{
    --inflight_;
    impl_->dec_inflight();

    if((impl_->state_&PIPES_STATE_MASK)==PIPES_RUNNING)
    {
      switch(result)
      {
          case kIOReturnSuccess:
          case kIOReturnUnderrun:
          case kIOReturnOverrun:
          case kIOReturnAborted:
              break;

          case kIOReturnIsoTooOld:
          case kIOReturnNoBandwidth:
              pic::logmsg() << "urb ignored with error 0x" << std::hex << result;
              break;

          case kIOUSBWrongPIDErr:
              pic::logmsg() << "pipe stall 0x" << std::hex << result;
              impl_->pipe_idle();
              break;

          default:
              pic::logmsg() << "urb completed with error 0x" << std::hex << result;
	            impl_->pipe_kill(PIPE_OK);
              break;
      }
    }
    
    submit(u);
}

void macosx_usbpipe_out_t::submit(usburb_t *u)
{
    IOUSBInterfaceInterface197 **intf = impl_->device.interface;
    IOReturn e;

    if((impl_->state_&PIPES_STATE_MASK) != PIPES_RUNNING)
    {
        return;
    }

    for(unsigned i=0;i<ISO_OUT_FRAMES_PER_URB*uframe_per_frame_;i++)
    {
        u->frame[i].frStatus=kUSBLowLatencyIsochTransferKey;
        u->frame[i].frActCount=0;
        u->frame[i].frTimeStamp.lo=0;
        u->frame[i].frTimeStamp.hi=0;

        if((i%uframe_per_frame_)==0)
        {
            //pic::printmsg() << "complete clear " << u->index << ":" << i;
            u->frame[i].frReqCount=size_;
            memset(u->buffer+(i/uframe_per_frame_)*size_,0,size_);
        }
        else
        {
            u->frame[i].frReqCount=0;
        }
    }


restart:

    u->fnum = counter_;

    if((e=(*intf)->LowLatencyWriteIsochPipeAsync(intf,piperef_,u->buffer,u->fnum/uframe_per_frame_,ISO_OUT_FRAMES_PER_URB*uframe_per_frame_,1,u->frame,out_completed__,u)) != kIOReturnSuccess)
    {
        if(e==kIOReturnIsoTooOld)
        {
            AbsoluteTime time;
            UInt64 f;
            (*intf)->GetBusFrameNumber(intf,&f,&time);
            f+=ISO_OUT_FUTURE;
            counter_ = f*uframe_per_frame_;
            pic::logmsg() << "Fell behind: output frame count resynced to " << counter_;
            goto restart;
        }

        if(e==kIOReturnNoBandwidth)
        {
            pic::logmsg() << "can't submit write request due to unsufficient bandwidth 0x" << std::hex << e;
            impl_->pipe_kill(PIPE_NO_BANDWIDTH);
            return;
        }

        pic::logmsg() << "can't submit write request 0x" << std::hex << e << " 0x" << std::hex << kIOReturnIsoTooOld;
        impl_->pipe_kill(PIPE_UNKNOWN_ERROR);
        return;
    }

    counter_+=ISO_OUT_FRAMES_PER_URB*uframe_per_frame_;
    ++inflight_;
    impl_->inc_inflight();
}

macosx_usbpipe_in_t::~macosx_usbpipe_in_t()
{
}

macosx_usbdevice_t::macosx_usbdevice_t(const char *name, unsigned iface)
{
    device=open_device(name);

    try
    {
        interface=open_interface(device,iface);
        hispeed=is_hispeed(interface);
        if(hispeed)
        {
            pic::logmsg() << "high speed device detected";
        }
        else
        {
            pic::logmsg() << "full speed device detected";
        }
    }
    catch(...)
    {
        close_device(device);
        throw;
    }

}

macosx_usbdevice_t::~macosx_usbdevice_t()
{
    close();
}

void macosx_usbdevice_t::close()
{
    close_interface(device,interface);
    close_device(device);
    device = 0;
    interface = 0;
}

pic::usbdevice_t::impl_t::impl_t(const char *name, unsigned iface): pic::cfrunloop_t(0), name_(name), device(name,iface), power(0), power_port(MACH_PORT_NULL),outpipe(0),cmd_pending(0),state_(PIPES_STOPPED)
{
}

void pic::usbdevice_t::impl_t::set_iso_out(iso_out_pipe_t *p )
{
    int piperef;
    unsigned uframe_per_frame;

    if((piperef=find_iso_pipe(device.interface,p->out_pipe_name(),&uframe_per_frame))<0)
    {
        pic::logmsg() << "can't find endpoint " << p->out_pipe_name();
        return;
    }
    
   try 
    {
        outpipe = new macosx_usbpipe_out_t(this,p,piperef,uframe_per_frame);
    }
    catch( std::exception &e )
    {
        pic::logmsg() << " cannot add pipe" << e.what();    
    }
    catch(...)
    {
        pic::logmsg() << "cannot add pipe: unknown error";
    }

}

bool pic::usbdevice_t::impl_t::add_bulk_out(bulk_out_pipe_t *pipe)
{
    new pic::usbdevice_t::bulk_out_pipe_t::impl_t(this,pipe);
    return true;
}

bool pic::usbdevice_t::impl_t::add_iso_in(iso_in_pipe_t *pipe)
{
    //PIC_ASSERT(!isrunning());
    int piperef;

    if((piperef=find_iso_pipe(device.interface,pipe->in_pipe_name()))<0)
    {
        pic::logmsg() << "can't find endpoint " << pipe->in_pipe_name();
        return false;
    }

    
    macosx_usbpipe_in_t *p = new macosx_usbpipe_in_t(this,pipe,piperef);
    inpipes_.alternate().push_back(p);
    inpipes_.exchange();
    return true;
}

pic::usbdevice_t::impl_t::~impl_t()
{
    close();
}

void pic::usbdevice_t::impl_t::close()
{
    stop_pipes();
    detach();
    device.close();
}


void pic::usbdevice_t::impl_t::call_pipe_died(unsigned reason)
{
    abort_urbs();

    if(power)
    {
        try
        {
            power->pipe_died(reason);
        }
        CATCHLOG()
    }
}

void pic::usbdevice_t::impl_t::call_pipe_stopped()
{
    abort_urbs();

    if(power)
    {
        try
        {
            power->pipe_stopped();
        }
        CATCHLOG()
    }
}

void pic::usbdevice_t::impl_t::call_pipe_running()
{
    pic::logmsg() << "pipe_running 0x" << std::hex << state_ << " woke pipe";

    {
        pipe_flipflop_t::guard_t g(inpipes_);
        pipe_list_t::const_iterator i;

        for(i=g.value().begin(); i!=g.value().end(); i++)
        {
            (*i)->start();
        }
    }

    if(outpipe)
    {
        outpipe->start();
    }

    cmd(0,0);
}

int pic::usbdevice_t::impl_t::runloop_cmd(void *c, void *a)
{
    pic::logmsg() << "runloop starting pipe";

    if(power)
    {
        try
        {
            power->pipe_started();
        }
        CATCHLOG()
    }

    pipe_flipflop_t::guard_t g(inpipes_);
    pipe_list_t::const_iterator i;

    for(i=g.value().begin(); i!=g.value().end(); i++)
    {
        (*i)->counter_=0;
        (*i)->service();
    }

    pic::logmsg() << "runloop started pipe";

    return 0;
}

void pic::usbdevice_t::impl_t::pipe_running()
{
    for(;;)
    {
        unsigned o = state_;
        unsigned os = o&PIPES_STATE_MASK;
        unsigned c = o&PIPES_COUNT_MASK;
        unsigned ns = os;

        switch(os)
        {
            case PIPES_RUNNING: return;
            case PIPES_SLEEP: return;
            case PIPES_KILLED: return;
            case PIPES_STOPPED: return;
            case PIPES_IDLE: ns=PIPES_RUNNING; break;
        }

        unsigned n = ns|c;

        if(pic_atomiccas(&state_,o,n))
        {
            call_pipe_running();
            return;
        }
    }
}

void pic::usbdevice_t::impl_t::pipe_sleep()
{
    for(;;)
    {
        unsigned o = state_;
        unsigned os = o&PIPES_STATE_MASK;
        unsigned c = o&PIPES_COUNT_MASK;
        unsigned ns = os;

        switch(os)
        {
            case PIPES_RUNNING: ns=PIPES_SLEEP; break;
            case PIPES_SLEEP: return;
            case PIPES_KILLED: return;
            case PIPES_STOPPED: return;
            case PIPES_IDLE: ns=PIPES_SLEEP; break;
        }

        unsigned n = ns|c;

        if(pic_atomiccas(&state_,o,n))
        {
            if(os!=PIPES_SLEEP && os!=PIPES_IDLE)
            {
                call_pipe_stopped();
            }

            return;
        }
    }
}

void pic::usbdevice_t::impl_t::pipe_idle()
{
    for(;;)
    {
        unsigned o = state_;
        unsigned os = o&PIPES_STATE_MASK;
        unsigned c = o&PIPES_COUNT_MASK;
        unsigned ns = os;

        switch(os)
        {
            case PIPES_RUNNING: ns=PIPES_IDLE; break;
            case PIPES_SLEEP: return;
            case PIPES_KILLED: return;
            case PIPES_STOPPED: return;
            case PIPES_IDLE: return;
        }

        unsigned n = ns|c;

        if(pic_atomiccas(&state_,o,n))
        {
            if(os!=PIPES_SLEEP && os!=PIPES_IDLE)
            {
                call_pipe_stopped();
            }

            return;
        }
    }
}

void pic::usbdevice_t::impl_t::pipe_kill(unsigned reason)
{
    for(;;)
    {
        unsigned o = state_;
        unsigned os = o&PIPES_STATE_MASK;
        unsigned c = o&PIPES_COUNT_MASK;
        unsigned ns = os;

        switch(os)
        {
            case PIPES_RUNNING: ns=PIPES_KILLED; break;
            case PIPES_SLEEP: ns=PIPES_KILLED; break;
            case PIPES_KILLED: return;
            case PIPES_STOPPED: ns=PIPES_KILLED; break;
            case PIPES_IDLE: ns=PIPES_KILLED; break;
        }

        unsigned n = ns|c;

        if(pic_atomiccas(&state_,o,n))
        {
            call_pipe_died(reason);
            return;
        }
    }
}

void pic::usbdevice_t::impl_t::inc_inflight()
{
    for(;;)
    {
        unsigned o = state_;
        unsigned n = o+1;

        if(pic_atomiccas(&state_,o,n))
        {
            return;
        }
    }
}

void pic::usbdevice_t::impl_t::dec_inflight()
{
    for(;;)
    {
        unsigned o = state_;
        unsigned c = o&PIPES_COUNT_MASK;
        unsigned s = o&PIPES_STATE_MASK;

        if(!c)
        {
            return;
        }

        c--;

        unsigned n = s|c;

        if(pic_atomiccas(&state_,o,n))
        {
            return;
        }
    }
}

void pic::usbdevice_t::impl_t::start_pipes()
{
    if(state_ == PIPES_STOPPED)
    {
        run();
        state_ = PIPES_IDLE;
    }
}

void pic::usbdevice_t::impl_t::stop_pipes()
{
    pipe_kill(PIPE_OK);
    pic::logmsg() << "usb shutdown starting";

    for(;;)
    {
        if(state_ == PIPES_KILLED)
        {
            break;
        }

        pic_microsleep(1000000);
        pic::logmsg() << "usb shutdown continues: 0x" << std::hex << state_;
    }

    stop();

    state_ = PIPES_STOPPED;
    pic::logmsg() << "usb shutdown complete";
}

void pic::usbdevice_t::impl_t::power_sleep()
{
    pic::logmsg() << "usb power sleep";
    pipe_sleep();
}

void pic::usbdevice_t::impl_t::reset_device()
{
    IOReturn e;

    if((e=(*device.device)->ResetDevice(device.device)) != kIOReturnSuccess)
    {
        pic::logmsg() << "can't reset device: 0x" << std::hex << e;
    }
    else
    {
        pic::logmsg() << "reset device";
    }
}

void pic::usbdevice_t::impl_t::power_wakeup()
{
    if(state_ == PIPES_SLEEP)
    {
        reset_device();
        state_ = PIPES_IDLE;
    }
}

static void __power_callback(void *i_, io_service_t s, natural_t mt, void *arg)
{
    pic::usbdevice_t::impl_t *i = (pic::usbdevice_t::impl_t *)i_;

    switch(mt)
    {
        case kIOMessageSystemWillSleep:
            pic::logmsg() << "power callback SystemWillSleep, " << (long)arg;
            i->power_sleep();
            IOAllowPowerChange(i->power_port, (long)arg);
            break;

        case kIOMessageCanSystemSleep:
            pic::logmsg() << "power callback CanSystemSleep, " << (long)arg;
            IOAllowPowerChange(i->power_port, (long)arg);
            break;

        case kIOMessageSystemHasPoweredOn:
            pic::logmsg() << "power callback SystemHasPoweredOn, " << (long)arg;
            i->power_wakeup();
            break;

        default:
            break;
    }
}

void pic::usbdevice_t::impl_t::runloop_init()
{
    IOReturn e;

    if((e=(*device.interface)->CreateInterfaceAsyncEventSource(device.interface,&source)) != kIOReturnSuccess)
    {
        pic::msg() << "can't create event source for interface: " << e << pic::hurl;
    }

    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);

    if(power)
    {
        power_port = IORegisterForSystemPower(this, &power_notify, __power_callback, &power_iterator);

        if(power_port != MACH_PORT_NULL)
        {
            power_source = IONotificationPortGetRunLoopSource(power_notify);
            CFRunLoopAddSource(CFRunLoopGetCurrent(), power_source, kCFRunLoopDefaultMode);
            pic::msg() << "registered for power status" << pic::log;
        }
        else
        {
            pic::msg() << "unable to register for power status" << pic::log;
        }
    }
}

void pic::usbdevice_t::impl_t::runloop_term()
{
    pic::logmsg() << "terminating runloop";
    if(power_port != MACH_PORT_NULL)
    {
        IODeregisterForSystemPower(&power_iterator);
        IOServiceClose(power_port);
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), power_source, kCFRunLoopDefaultMode);
    }

    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);
}

void pic::usbdevice_t::control_in(unsigned char req_type, unsigned char req, unsigned short value, unsigned short index, void *buffer, unsigned buflen, unsigned timeout)
{
    IOUSBDevRequestTO ctrl;
    IOReturn e;

    ctrl.bmRequestType = req_type;
    ctrl.bRequest = req;
    ctrl.wValue = value;
    ctrl.wIndex = index;
    ctrl.wLength = buflen;
    ctrl.pData = buffer;
    ctrl.noDataTimeout = 0;
    ctrl.completionTimeout = timeout;

    if((e=(*impl_->device.device)->DeviceRequestTO(impl_->device.device,&ctrl)))
    {
        pic::msg() << "control_in err=0x" << std::hex << e << " req " << (int)req_type << ":" << (int)req << pic::hurl;
    }
}

void pic::usbdevice_t::control_out(unsigned char req_type, unsigned char req, unsigned short value, unsigned short index, const void *buffer, unsigned buflen, unsigned timeout)
{
    IOUSBDevRequestTO ctrl;
    IOReturn e;

    ctrl.bmRequestType = req_type;
    ctrl.bRequest = req;
    ctrl.wValue = value;
    ctrl.wIndex = index;
    ctrl.wLength = buflen;
    ctrl.pData = (void *)buffer;
    ctrl.noDataTimeout = 0;
    ctrl.completionTimeout = timeout;

    if((e=(*impl_->device.device)->DeviceRequestTO(impl_->device.device,&ctrl)))
    {
        pic::msg() << "control_out err=0x" << std::hex << e << " req " << (int)req_type << ":" << (int)req << pic::hurl;
    }
}

void pic::usbdevice_t::control(unsigned char req_type, unsigned char req, unsigned short value, unsigned short index, unsigned timeout)
{
    IOUSBDevRequestTO ctrl;
    IOReturn e;

    ctrl.bmRequestType = req_type;
    ctrl.bRequest = req;
    ctrl.wValue = value;
    ctrl.wIndex = index;
    ctrl.wLength = 0;
    ctrl.pData = 0;
    ctrl.noDataTimeout = 0;
    ctrl.completionTimeout = timeout;

    if((e=(*impl_->device.device)->DeviceRequestTO(impl_->device.device,&ctrl)))
    {
        pic::msg() << "control err=0x" << std::hex << e << " req " << (int)req_type << ":" << (int)req << pic::hurl;
    }
}

void pic::usbdevice_t::bulk_out_pipe_t::impl_t::bulk_write(const void *data, unsigned len, unsigned timeout)
{
    IOReturn e;

    e = (*device_->device.interface)->WritePipeTO(device_->device.interface,ref_,(void *)data,len,0,timeout);

    if(e!=0)
    {
        pic::msg() << "write pipe err=0x" << std::hex << e << pic::hurl;
    }
}

void pic::usbdevice_t::bulk_out_pipe_t::bulk_write(const void *data, unsigned len, unsigned timeout)
{
    impl_->bulk_write(data,len,timeout);
}

pic::usbdevice_t::usbdevice_t(const char *name, unsigned iface)
{
    impl_ = new pic::usbdevice_t::impl_t(name,iface);
}

const char *pic::usbdevice_t::name()
{
    return impl_->name_.c_str();
}

void pic::usbdevice_t::start_pipes()
{
    impl_->start_pipes();
}

void pic::usbdevice_t::stop_pipes()
{
    impl_->stop_pipes();
}

void pic::usbdevice_t::close()
{
    impl_->close();
}

pic::usbdevice_t::~usbdevice_t()
{
    delete impl_;
}

bool pic::usbdevice_t::add_bulk_out(bulk_out_pipe_t *pipe)
{
    return impl_->add_bulk_out(pipe);
}

bool pic::usbdevice_t::add_iso_in(iso_in_pipe_t *pipe)
{
    return impl_->add_iso_in(pipe);
}

void pic::usbdevice_t::set_iso_out(iso_out_pipe_t *p)
{
    impl_->set_iso_out(p);
}

void pic::usbdevice_t::set_power_delegate(power_t *p)
{
    impl_->power = p;
}

void pic::usbdevice_t::impl_t::detach()
{
    pic::logmsg() << "detaching client";

    while(inpipes_.alternate().size()>0)
    {
        macosx_usbpipe_in_t *p = inpipes_.alternate().front();
        inpipes_.alternate().pop_front();
        inpipes_.exchange();
        delete p;
    }

    if(outpipe)
    {
        delete outpipe;
        outpipe = 0;
    }

    power=0;

    pic::logmsg() << "done detaching client";
}

void pic::usbdevice_t::detach()
{
    impl_->detach();
}

unsigned char *macosx_usbpipe_out_t::iso_write_start()
{
    // find current write position in buffer
    unsigned last_status = kUSBLowLatencyIsochTransferKey;
    for(;;)
    {
        unsigned status = urbs[write_urb_].frame[write_frame_].frStatus;

        if(status!=0 && status!=kUSBLowLatencyIsochTransferKey)
        {
            return 0;
        }

        if(last_status==0 && status==kUSBLowLatencyIsochTransferKey)
            break;

        last_status=status;
        iso_write_advance(1);
    }

    return iso_write_advance(ISO_OUT_OFFSET);
}

unsigned char *macosx_usbpipe_out_t::iso_write_advance(unsigned n)
{
    unsigned f=(write_urb_*ISO_OUT_FRAMES_PER_URB*uframe_per_frame_)+write_frame_+n*uframe_per_frame_;
    write_urb_=(f/(ISO_OUT_FRAMES_PER_URB*uframe_per_frame_))%ISO_OUT_URBS;
    write_frame_=f%(ISO_OUT_FRAMES_PER_URB*uframe_per_frame_);
    return urbs[write_urb_].buffer+(write_frame_/uframe_per_frame_)*size_;
}

pic::usbdevice_t::iso_out_guard_t::iso_out_guard_t(usbdevice_t *d): impl_(d->impl_), current_(0)
{
    if(!impl_->outpipe)
    {
        return;
    }

    current_ = impl_->outpipe->iso_write_start();
}

pic::usbdevice_t::iso_out_guard_t::~iso_out_guard_t()
{
}

unsigned char *pic::usbdevice_t::iso_out_guard_t::advance()
{
    PIC_ASSERT(impl_->outpipe);
    current_=impl_->outpipe->iso_write_advance(1);
    return current_;
}

pic::usbenumerator_t::impl_t::impl_t(unsigned short v, unsigned short p, const pic::f_string_t &d, const pic::f_string_t &g): pic::cfrunloop_t(0), vendor_(v), product_(p)
{
    delegate_ = d;
    gone_ = g;
}

pic::usbenumerator_t::impl_t::~impl_t()
{
    stop();
}

void pic::usbenumerator_t::impl_t::runloop_init()
{
    mach_port_t master_port;
    CFMutableDictionaryRef matching_dict;
    kern_return_t e;

    if((e=IOMasterPort(MACH_PORT_NULL,&master_port)))
    {
        pic::msg() << "can't create IO master port: 0x" << std::hex << e << pic::hurl;
    }

    matching_dict = IOServiceMatching(kIOUSBDeviceClassName);
    if(!matching_dict)
    {
        mach_port_deallocate(mach_task_self(), master_port);
        pic::msg() << "can't create USB matching dict" << pic::hurl;
        return;
    }

    CFDictionarySetValue(matching_dict, CFSTR(kUSBVendorName), CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vendor_));
    CFDictionarySetValue(matching_dict, CFSTR(kUSBProductName), CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &product_));

    notification_port_ = IONotificationPortCreate(master_port);
    notification_source_ = IONotificationPortGetRunLoopSource(notification_port_);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), notification_source_, kCFRunLoopDefaultMode);

    if((e=IOServiceAddMatchingNotification(notification_port_, kIOFirstMatchNotification, matching_dict, device_attached, this, &iterator_)))
    {
        mach_port_deallocate(mach_task_self(), master_port);
        pic::msg() << "can't create IO iterator: 0x" << std::hex << e << pic::hurl;
    }

}

void pic::usbenumerator_t::impl_t::runloop_start()
{
    device_attached(this,iterator_);
}


void pic::usbenumerator_t::impl_t::runloop_term()
{
    IOObjectRelease(iterator_);
}

namespace
{
    struct notify_context_t
    {
        notify_context_t(pic::usbenumerator_t::impl_t *i, const char *n): impl_(i),name_(n)
        {
        }

        static void notify(void *n_, io_service_t s, natural_t t, void *a)
        {
            notify_context_t *n = (notify_context_t *)n_;
            if(t==kIOMessageServiceIsTerminated)
            {
                try
                {
                    pic::mutex_t::guard_t g(n->impl_->lock_);
                    n->impl_->gone_(n->name_.c_str());
                    delete n;
                }
                CATCHLOG()
            }
        }

        pic::usbenumerator_t::impl_t *impl_;
        std::string name_;
        io_object_t obj_;
    };
}

void pic::usbenumerator_t::impl_t::device_attached(void *impl_, io_iterator_t i)
{
    pic::usbenumerator_t::impl_t *impl = (pic::usbenumerator_t::impl_t *)impl_;
    io_service_t s;
    kern_return_t e;
    SInt32 score;
    IOCFPlugInInterface **plug;
    IOUSBDeviceInterface **device;
    char name[10];
    unsigned int location;

    while((s=IOIteratorNext(i)))
    {
        e=IOCreatePlugInInterfaceForService(s, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plug, &score);
        if(e || !plug)
        {
            IOObjectRelease(s);
            continue;
        }

        (*plug)->QueryInterface(plug,CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID197), (void **)&device);
        (*plug)->Stop(plug);
        IODestroyPlugInInterface(plug);
        if(!device)
        {
            IOObjectRelease(s);
            continue;
        }
        (*device)->GetLocationID(device,&location);
        sprintf(name,"%08x",location);

        notify_context_t *n = new notify_context_t(impl,name);
        e=IOServiceAddInterestNotification(impl->notification_port_,s,kIOGeneralInterest,&notify_context_t::notify,n,&(n->obj_));

        if(e)
        {
            pic::logmsg() << "cannot register for notifications: " << e;
            delete n;
        }

        IOObjectRelease(s);

        try
        {
            pic::mutex_t::guard_t g(impl->lock_);
            impl->delegate_(name);
        }
        CATCHLOG()

        (*device)->Release(device);
    }
}

static void _enumerate_visitor_(void *callback_, IOUSBDeviceInterface197 **device)
{
    char buffer[10];
    unsigned int name;

    (*device)->GetLocationID(device,&name);
    sprintf(buffer,"%08x",name);

    try
    {
        (*(pic::f_string_t *)callback_)(buffer);
    }
    CATCHLOG()
}

unsigned pic::usbenumerator_t::enumerate(unsigned short vend, unsigned short prod, const pic::f_string_t &callback)
{
    return device_enumerate(vend,prod,_enumerate_visitor_, (void *)&callback);
}

pic::usbenumerator_t::usbenumerator_t(unsigned short v, unsigned short p, const pic::f_string_t &d)
{
    impl_ = new impl_t(v,p,d,pic::f_string_t());
}

pic::usbenumerator_t::usbenumerator_t(unsigned short v, unsigned short p, const pic::f_string_t &d, const pic::f_string_t &g)
{
    impl_ = new impl_t(v,p,d,g);
}

pic::usbenumerator_t::~usbenumerator_t()
{
    delete impl_;
}

void pic::usbenumerator_t::start()
{
    impl_->run();
}

void pic::usbenumerator_t::stop()
{
    impl_->stop();
}

int pic::usbenumerator_t::gc_traverse(void *v, void *a) const
{
    return impl_->delegate_.gc_traverse(v,a);
}

int pic::usbenumerator_t::gc_clear()
{
    pic::mutex_t::guard_t g(impl_->lock_);
    return impl_->delegate_.gc_clear();
}
