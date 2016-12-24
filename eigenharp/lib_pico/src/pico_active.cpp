
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

#include <picross/pic_config.h>
#include <picross/pic_log.h>
#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_endian.h>
#include <lib_pico/pico_active.h>
#include <lib_pico/pico_usb.h>
#include <memory>
#include <iomanip>

#define KEYS 18
#define MODEKEYS 4

struct pico::active_t::impl_t: pic::usbdevice_t::iso_in_pipe_t, pic::usbdevice_t::power_t, pic::usbdevice_t, virtual pic::lckobject_t, pic::element_t<>
{
    impl_t(const char *, pico::active_t::delegate_t *);
    ~impl_t();

    void in_pipe_data(const unsigned char *frame, unsigned length, unsigned long long hf, unsigned long long ht,unsigned long long pt);

    void pipe_died(unsigned reason);
    void pipe_started();
    void pipe_stopped();
    void start();
    void stop();
    void set_led(unsigned,unsigned);

    static void decode_cooked(void *ctx,unsigned long long ts,int tp, int id,unsigned a,unsigned p,int r,int y);
    static void decode_raw(void *ctx,int resync,const pico_rawkbd_t *);

    pico::active_t::delegate_t *handler_;
    bool raw_;
    unsigned ledmask_;
    pico_decoder_t decoder_;
    bool resync_;
};

static pic::ilist_t<pico::active_t::impl_t> *kbds__;

static void __shutdown()
{
    //shutdown no longer used, as agent closes correctly
    fprintf(stderr,"shutdown handler\n");

    pico::active_t::impl_t *i = kbds__->head();

    while(i)
    {
        try
        {
            for(unsigned k=0; k<22; ++k)
                i->set_led(k,0);
            i->control_out(TYPE_VENDOR,BCTPICO_USBCOMMAND_STOP,0,0,0,0);
            i->stop();
            i->close();
        }
        catch(...)
        {
        }
        i = kbds__->next(i);
    }
}

pico::active_t::impl_t::impl_t(const char *name, pico::active_t::delegate_t *del): usbdevice_t::iso_in_pipe_t(BCTPICO_USBENDPOINT_SENSOR_NAME,BCTPICO_USBENDPOINT_SENSOR_SIZE), 
    usbdevice_t(name,BCTPICO_INTERFACE), 
    handler_(del), 
    raw_( false ),
    ledmask_(0xFF),
    resync_(false)
{
    pico_decoder_create(&decoder_,PICO_DECODER_PICO);

    if(!kbds__)
    {
        kbds__=new pic::ilist_t<pico::active_t::impl_t>;
        //TODO: remove shutdown handler, now unncessary
        //atexit(__shutdown);
    }

    kbds__->append(this);
    control(0x40,0xd1,0,0);
    add_iso_in(this);
    set_power_delegate(this);
}

pico::active_t::impl_t::~impl_t()
{  
    try
    {
        control_out(TYPE_VENDOR,BCTPICO_USBCOMMAND_STOP,0,0,0,0);
    }
    catch(...)
    {
    }
    stop(); 
    close();
}

void pico::active_t::impl_t::start()
{
    start_pipes();
}

void pico::active_t::impl_t::pipe_started()
{
    control_out(TYPE_VENDOR,BCTPICO_USBCOMMAND_START,0,0,0,0);
    pic::logmsg() << "restoring led mask:" << ledmask_;
    pic_microsleep(5000);
    control(TYPE_VENDOR,BCTPICO_USBCOMMAND_SETMODELED,ledmask_,0);
}

void pico::active_t::impl_t::stop()
{
    stop_pipes();
}

void pico::active_t::impl_t::pipe_stopped()
{
    try
    {
        control_out(TYPE_VENDOR,BCTPICO_USBCOMMAND_STOP,0,0,0,0);
    }
    catch(...)
    {
        pic::logmsg() << "device shutdown failed";
    }
}

pico::active_t::active_t(const char *name, pico::active_t::delegate_t *handler)
{
    _impl = new impl_t(name,handler);
}

pico::active_t::~active_t()
{
    delete _impl;
}

void pico::active_t::start()
{
    _impl->start();
}

void pico::active_t::stop()
{
    _impl->stop();
}

bool pico::active_t::poll(unsigned long long t)
{
    bool skipped = _impl->poll_pipe(t);

    if(skipped)
    {
        _impl->resync_ = true;
    }

    return skipped;
}

const char *pico::active_t::get_name()
{
    return _impl->name();
}

void pico::active_t::set_led(unsigned key, unsigned colour)
{
    _impl->set_led(key,colour);
}
/*
std::string dec2bin(unsigned n)
{
    const size_t size = sizeof(n) * 8;
    char result[size];

    unsigned index = size;
    do {
        result[--index] = '0' + (n & 1);
    } while (n >>= 1);

    return std::string(result + index, result + size);
}
*/
void pico::active_t::impl_t::set_led(unsigned key, unsigned colour)
{
    if( key < 18 )
    {
        unsigned green = (colour&1);
        unsigned red = (colour&2);
        unsigned mask = (green<<2) + (red<<4);
        control(TYPE_VENDOR,BCTPICO_USBCOMMAND_SETLED,mask,key);
    }
    else
    {
       unsigned mode = key % 17;
       unsigned shift = (mode-1)*2;
       unsigned kmask = ~(3<<shift);
       unsigned c = colour;
       if(colour==0) c=3;
       else if(colour==3) c=0;
       ledmask_ &= kmask;
       ledmask_ |= (c<<shift);
       
       // sending the mode led data three times as a workaround for some weirdness in the firmware
       // this is a stop-gap fix until we can improve this at the firmware level, see issue #518
       control(TYPE_VENDOR,BCTPICO_USBCOMMAND_SETMODELED,ledmask_,key);
       control(TYPE_VENDOR,BCTPICO_USBCOMMAND_SETMODELED,ledmask_,key);
       control(TYPE_VENDOR,BCTPICO_USBCOMMAND_SETMODELED,ledmask_,key);
    }
}

void pico::active_t::impl_t::decode_cooked(void *self,unsigned long long ts,int tp, int id,unsigned a,unsigned p,int r,int y)
{
    impl_t *impl = (impl_t *)self;

    switch(tp)
    {
        case PICO_DECODER_KEY:
            impl->handler_->kbd_key(ts,id,a,p,r,y);
            break;
        case PICO_DECODER_BREATH:
            impl->handler_->kbd_breath(ts,p);
            break;
        case PICO_DECODER_STRIP:
            impl->handler_->kbd_strip(ts,p);
            break;
        case PICO_DECODER_MODE:
            impl->handler_->kbd_mode(ts,KEYS+id,p);
            break;
    }
}

void pico::active_t::impl_t::decode_raw(void *self, int resync,const pico_rawkbd_t *rawkeys)
{
    impl_t *impl = (impl_t *)self;
    impl->handler_->kbd_raw(resync,*rawkeys);
}

void pico::active_t::impl_t::in_pipe_data(const unsigned char *frame, unsigned length, unsigned long long hf, unsigned long long ht, unsigned long long pt)
{
#if 0
    unsigned short* f=(unsigned short*) frame;
    unsigned a1=(unsigned) (*(frame));
    unsigned a2=(unsigned) (*(frame+1));
    unsigned a3=(unsigned) (*(frame+2));
    unsigned a4=(unsigned) (*(frame+3));
    unsigned v=0,c=0;
    for(c=0;c<length && v==0;c++)
    {
      v=(int) (*(frame+c));
    }
    printf("%d  : %04x %04x %04x %04x ... %x %x %x %x... %x %x\n",length, *(f), *(f+6), *(f+12), *(f+18),a1,a2,a3,a4,c,v);
#endif
    if(raw_)
    {
        pico_decoder_raw(&decoder_,resync_?1:0,frame,length,ht,decode_raw,this);
    }
    else
    {
        pico_decoder_cooked(&decoder_,resync_?1:0,frame,length,ht,decode_cooked,this);
    }

    resync_=false;
}

void pico::active_t::impl_t::pipe_died(unsigned reason)
{
    pic::logmsg() << "pico::active pipe died";
    pipe_stopped();
    handler_->kbd_dead(reason);
}

void pico::active_t::load_calibration_from_device()
{
    pic::logmsg() << "loading calibration from device";

    unsigned short min,max,row[BCTPICO_CALTABLE_POINTS+2];
    row[0] = 0;
    row[BCTPICO_CALTABLE_POINTS+1] = 4095;

    try
    {
        for(unsigned k=0; k<KEYS; ++k)
        {
            for(unsigned c=0; c<4; ++c)
            {
                if(get_calibration(k,c,&min,&max,row+1))
                {
                    pico_decoder_cal(&_impl->decoder_,k,c,min,max,BCTPICO_CALTABLE_POINTS+2,row);
                }
                else
                {
                    pic::logmsg() << "warning: no data for key " << k << " corner " << c;
                }
            }
        }

        pic::logmsg() << "loading calibration done";
    }
    catch(pic::error &e)
    {
        pic::logmsg() << "error reading calibration data: " << e.what();
    }
}

std::string pico::active_t::debug()
{
    //return _impl->control_in(0x80|0x40,0xc0,0,0,6);
    return "";
}

static void __write16(unsigned char *raw, unsigned i, unsigned short val)
{
    raw[i*2] = (val>>8)&0x00ff;
    raw[i*2+1] = (val>>0)&0x00ff;
}

void pico::active_t::set_calibration(unsigned key, unsigned corner, unsigned short min, unsigned short max, const unsigned short *table)
{
    unsigned char raw[2*BCTPICO_CALTABLE_SIZE];

    __write16(raw,BCTPICO_CALTABLE_MIN,min);
    __write16(raw,BCTPICO_CALTABLE_MAX,max);

    for(unsigned i=0;i<BCTPICO_CALTABLE_POINTS;i++)
    {
        __write16(raw,BCTPICO_CALTABLE_DATA+i,table[i]);
    }

    _impl->control_out(TYPE_VENDOR,BCTPICO_USBCOMMAND_CALDATA,corner,key,raw,2*BCTPICO_CALTABLE_SIZE);
}

void pico::active_t::write_calibration()
{
    _impl->control(TYPE_VENDOR,BCTPICO_USBCOMMAND_CALWRITE,0,0);
}

void pico::active_t::clear_calibration()
{
    _impl->control(TYPE_VENDOR,BCTPICO_USBCOMMAND_CALCLEAR,0,0);
}

unsigned pico::active_t::get_temperature()
{
    return debug()[0];
}

bool pico::active_t::get_calibration(unsigned key, unsigned corner, unsigned short *min, unsigned short *max, unsigned short *table)
{
    unsigned short data[BCTPICO_CALTABLE_SIZE];

    _impl->control_in(0x80|TYPE_VENDOR,BCTPICO_USBCOMMAND_CALREAD,corner,key,data,2*BCTPICO_CALTABLE_SIZE);

    *min = pic_ntohs(data[0]);
    *max = pic_ntohs(data[1]);

    for(unsigned i=0; i<BCTPICO_CALTABLE_POINTS; ++i)
    {
        table[i] = pic_ntohs(data[i+2]);
    }

    if((data[0]&0x8080) == 0x8080)
        return false;

    return true;
}

pic::usbdevice_t *pico::active_t::device()
{
    return _impl;
}

void pico::active_t::set_raw(bool r)
{
    _impl->raw_=r;
}
