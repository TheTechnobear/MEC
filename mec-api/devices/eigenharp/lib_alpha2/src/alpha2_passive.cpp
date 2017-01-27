
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

#include <lib_alpha2/alpha2_passive.h>
#include <lib_alpha2/alpha2_active.h>
#include <lib_alpha2/alpha2_usb.h>
#include <picross/pic_log.h>
#include <picross/pic_thread.h>
#include <picross/pic_ref.h>
#include <picross/pic_flipflop.h>
#include <picross/pic_time.h>
#include <string.h>
#include <vector>

namespace 
{
    struct blob_t
    {
        unsigned short corners[KBD_KEYS+KBD_SENSORS][4];
    };

    struct data_t: pic::counted_t
    {
        blob_t blob;
    };

    typedef pic::ref_t<data_t> dataref_t;
    typedef std::vector<dataref_t> datalist_t;

    struct calibration_row_t
    {
        unsigned key,corner;
        unsigned short min,max;
        unsigned short points[BCTKBD_CALTABLE_POINTS];
    };
}

struct alpha2::passive_t::impl_t: public alpha2::active_t::delegate_t
{
    impl_t(const char *name,unsigned decim);
    bool wait();
    void kbd_raw(unsigned long long t, unsigned k, unsigned c1, unsigned c2, unsigned c3, unsigned c4);
    void kbd_dead(unsigned reason){ is_kbd_died = true; }
    pic::usbdevice_t device_;
    active_t loop_;
    pic::poller_t poller_;
    calibration_row_t cal_row_;
    pic::gate_t gate;
    unsigned decim;
    unsigned long keycount,scancount;
    unsigned start;
    bool insync;
    bool is_kbd_died;
    pic::flipflop_t<bool> collecting;

    data_t decimated_;
    dataref_t current_;
    datalist_t data_;
};

void alpha2::passive_t::impl_t::kbd_raw(unsigned long long t, unsigned k, unsigned c1, unsigned c2, unsigned c3, unsigned c4)
{
    //pic::logmsg() << "raw " << k;

    PIC_ASSERT(k<(KBD_KEYS+KBD_SENSORS));

    current_->blob.corners[k][0]=c1;
    current_->blob.corners[k][1]=c2;
    current_->blob.corners[k][2]=c3;
    current_->blob.corners[k][3]=c4;

    if((scancount%decim) == 0)
    {
        if(gate.isopen())
        {
            insync = false;
        }
    }

    keycount++;
    if(keycount==1)
    {
        start=k;
        return;
    }

    if(k!=start)
    {
        return;
    }

    if((scancount%decim) == 0)
    {
        memcpy(&decimated_.blob,&current_->blob,sizeof(blob_t));
        gate.open();
    }

    {
        pic::flipflop_t<bool>::guard_t g(collecting);
        if(g.value())
        {
            data_.push_back(current_);
            current_ = pic::ref(new data_t);
        }
    }

    scancount++;
}

alpha2::passive_t::impl_t::impl_t(const char *name,unsigned d): device_(name,0), loop_(&device_,this,false),poller_(&loop_),decim(d),keycount(0),scancount(0),insync(true),is_kbd_died(false),collecting(false),current_(pic::ref(new data_t))
{
    memset(&cal_row_,0,sizeof(cal_row_));
}

bool alpha2::passive_t::impl_t::wait()
{
    gate.shut();
    gate.untimedpass();
    return insync;
}

const char *alpha2::passive_t::get_name()
{
    return impl_->loop_.get_name();
}

alpha2::passive_t::passive_t(const char *name,unsigned decim)
{
    impl_=new impl_t(name,decim);
}

alpha2::passive_t::~passive_t()
{
    delete impl_;
}

unsigned short alpha2::passive_t::get_rawkey(unsigned key, unsigned corner)
{
    PIC_ASSERT(key<(KBD_KEYS+KBD_SENSORS));
    PIC_ASSERT(corner<4);

    return impl_->decimated_.blob.corners[key][corner];
}

void alpha2::passive_t::set_ledcolour(unsigned key, unsigned colour)
{
    impl_->loop_.msg_set_led(key,colour);
    impl_->loop_.msg_flush();
}

void alpha2::passive_t::control(unsigned char t, unsigned char r, unsigned short v, unsigned short i)
{
    impl_->device_.control(t,r,v,i);
}

void alpha2::passive_t::control_out(unsigned char t, unsigned char r, unsigned short v, unsigned short i, const std::string &s)
{
    impl_->device_.control_out(t,r,v,i,s);
}

std::string alpha2::passive_t::control_in(unsigned char t, unsigned char r, unsigned short v, unsigned short i, unsigned l)
{
    return impl_->device_.control_in(t,r,v,i,l);
}

void alpha2::passive_t::start_calibration_row(unsigned key, unsigned corner)
{
    impl_->cal_row_.key=key;
    impl_->cal_row_.corner=corner;
}

void alpha2::passive_t::set_calibration_range(unsigned short min, unsigned short max)
{
    impl_->cal_row_.min = min;
    impl_->cal_row_.max = max;
}

void alpha2::passive_t::set_calibration_point(unsigned point, unsigned short value)
{
    impl_->cal_row_.points[point] = value;
}

void alpha2::passive_t::write_calibration_row()
{
    impl_->loop_.msg_set_calibration(impl_->cal_row_.key,impl_->cal_row_.corner,impl_->cal_row_.min,impl_->cal_row_.max,impl_->cal_row_.points);
}

void alpha2::passive_t::commit_calibration()
{
    impl_->loop_.msg_write_calibration();
}

void alpha2::passive_t::clear_calibration()
{
    impl_->loop_.msg_clear_calibration();
}

unsigned alpha2::passive_t::get_temperature()
{
    return impl_->loop_.get_temperature();
}

bool alpha2::passive_t::wait()
{
    return impl_->wait();
}

bool alpha2::passive_t::is_kbd_died()
{
    return impl_->is_kbd_died;
}
unsigned long alpha2::passive_t::count()
{
    return impl_->keycount;
}

unsigned alpha2::passive_t::collect_count()
{
    return impl_->data_.size();
}

unsigned short alpha2::passive_t::get_collected_key(unsigned scan, unsigned key, unsigned corner)
{
    PIC_ASSERT(key<(KBD_KEYS+KBD_SENSORS));
    PIC_ASSERT(corner<4);
    return impl_->data_[scan]->blob.corners[key][corner];
}

void alpha2::passive_t::sync()
{
    impl_->insync=true;
}

void alpha2::passive_t::start()
{
    impl_->loop_.set_raw(true);
    pic_microsleep(250000);
    impl_->loop_.start();
}

void alpha2::passive_t::start_polling()
{
    impl_->poller_.start_polling();
}

void alpha2::passive_t::stop()
{
    impl_->loop_.stop();
}

void alpha2::passive_t::stop_polling()
{
    impl_->poller_.stop_polling();
}

void alpha2::passive_t::start_collecting()
{
    pic::logmsg() << "start collecting";
    PIC_ASSERT(!impl_->collecting.current());
    impl_->data_.clear();
    impl_->data_.reserve(1000);
    impl_->collecting.set(true);
}

void alpha2::passive_t::stop_collecting()
{
    impl_->collecting.set(false);
    pic::logmsg() << "stop collecting " << impl_->data_.size();
}

void alpha2::passive_t::set_active_colour(unsigned c)
{
    impl_->loop_.set_active_colour(c);
}
