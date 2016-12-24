
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

#include <lib_pico/pico_passive.h>
#include <lib_pico/pico_active.h>
#include <lib_pico/pico_usb.h>
#include <picross/pic_thread.h>
#include <string.h>

struct calibration_row_t
{
    unsigned key,sensor;
    unsigned short min,max;
    unsigned short points[BCTPICO_CALTABLE_POINTS];
};

struct data_t: pic::counted_t
{
    pico::active_t::rawkbd_t kbd;
};

typedef pic::ref_t<data_t> dataref_t;
typedef std::vector<dataref_t> datalist_t;

struct pico::passive_t::impl_t : pico::active_t::delegate_t
{
    impl_t(const char *name, unsigned decim);
    void kbd_raw(bool resync,const pico::active_t::rawkbd_t &);
    void kbd_dead(unsigned reason);

    active_t::rawkbd_t data_;
    active_t loop_;
    pic::poller_t poller_;
    pic::gate_t gate_;
    unsigned decim_;
    unsigned scancount_;
    bool insync_;
    bool is_pipe_died;
    calibration_row_t cal_row_;

    pic::flipflop_t<bool> collecting;
    datalist_t datalist_;
};

void pico::passive_t::impl_t::kbd_dead(unsigned reason)
{
    is_pipe_died = true;

}

void pico::passive_t::impl_t::kbd_raw(bool resync,const pico::active_t::rawkbd_t &keys)
{
    if(++scancount_ == decim_)
    {
        if(gate_.isopen())
        {
            insync_ = false;
        }
        data_ = keys;
        gate_.open();
        scancount_ = 0;
    }

    {
        pic::flipflop_t<bool>::guard_t g(collecting);
        if(g.value())
        {
            dataref_t current = pic::ref(new data_t);
            memcpy(&current->kbd,&keys,sizeof(active_t::rawkbd_t));
            datalist_.push_back(current);
        }
    }
}

pico::passive_t::impl_t::impl_t(const char *name, unsigned decim): loop_(name,this), poller_(&loop_), decim_(decim), scancount_(0), insync_(true),is_pipe_died( false ), collecting(false)
{
    loop_.set_raw(true);
}

pico::passive_t::passive_t(const char *name, unsigned decim)
{
    impl_=new impl_t(name,decim);
}

pico::passive_t::~passive_t()
{
    delete impl_;
}

unsigned short pico::passive_t::get_rawkey(unsigned key, unsigned sensor)
{
    return impl_->data_.keys[key].c[sensor];
}

unsigned short pico::passive_t::get_button(unsigned key)
{
    return impl_->data_.buttons[key];
}

unsigned short pico::passive_t::get_breath()
{
    return impl_->data_.breath[0];
}

unsigned short pico::passive_t::get_strip()
{
    return impl_->data_.strip;
}

void pico::passive_t::set_ledcolour(unsigned key, unsigned colour)
{
    impl_->loop_.set_led(key,colour);
}

bool pico::passive_t::is_kbd_died()
{
    return impl_->is_pipe_died;
}

bool pico::passive_t::wait()
{
    impl_->gate_.shut();
    impl_->gate_.untimedpass();
    return impl_->insync_;
}

void pico::passive_t::sync()
{
    impl_->insync_ = true;
}

std::string pico::passive_t::debug()
{
    return impl_->loop_.debug();
}

void pico::passive_t::start_calibration_row(unsigned key, unsigned corner)
{
    impl_->cal_row_.key=key;
    impl_->cal_row_.sensor=corner;
}

void pico::passive_t::set_calibration_range(unsigned short min, unsigned short max)
{
    impl_->cal_row_.min = min;
    impl_->cal_row_.max = max;
}

void pico::passive_t::set_calibration_point(unsigned point, unsigned short value)
{
    impl_->cal_row_.points[point] = value;
}

void pico::passive_t::write_calibration_row()
{
    impl_->loop_.set_calibration(impl_->cal_row_.key,impl_->cal_row_.sensor,impl_->cal_row_.min,impl_->cal_row_.max,impl_->cal_row_.points);
}

void pico::passive_t::commit_calibration()
{
    impl_->loop_.write_calibration();
}

void pico::passive_t::clear_calibration()
{
    impl_->loop_.clear_calibration();
}

void pico::passive_t::read_calibration_row()
{
    impl_->loop_.get_calibration(impl_->cal_row_.key,impl_->cal_row_.sensor,&impl_->cal_row_.min,&impl_->cal_row_.max,impl_->cal_row_.points);
}

unsigned short pico::passive_t::get_calibration_min()
{
    return impl_->cal_row_.min;
}

unsigned short pico::passive_t::get_calibration_max()
{
    return impl_->cal_row_.max;
}

unsigned short pico::passive_t::get_calibration_point(unsigned point)
{
    return impl_->cal_row_.points[point];
}

void pico::passive_t::start()
{
    impl_->loop_.start();
    impl_->poller_.start_polling();
}

void pico::passive_t::stop()
{
    impl_->poller_.stop_polling();
    impl_->loop_.stop();
}

unsigned pico::passive_t::get_temperature()
{
    return impl_->loop_.get_temperature();
}

void pico::passive_t::start_collecting()
{
    PIC_ASSERT(!impl_->collecting.current());
    impl_->datalist_.clear();
    impl_->datalist_.reserve(1000);
    impl_->collecting.set(true);
}

void pico::passive_t::stop_collecting()
{
    impl_->collecting.set(false);
}

unsigned pico::passive_t::collect_count()
{
    return impl_->datalist_.size();
}

unsigned short pico::passive_t::get_collected_key(unsigned scan, unsigned key, unsigned corner)
{
    return impl_->datalist_[scan]->kbd.keys[key].c[corner];
}
