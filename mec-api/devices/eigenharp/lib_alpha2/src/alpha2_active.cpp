
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

#ifdef DEBUG
#endif
#include <string.h>

#include <lib_alpha2/alpha2_active.h>
#include <lib_alpha2/alpha2_usb.h>
#include <lib_alpha2/alpha2_reg.h>

#include <picross/pic_usb.h>
#include <picross/pic_log.h>
#include <picross/pic_config.h>
#include <picross/pic_time.h>

#include <stdlib.h>
#include <math.h>

#ifndef PI_BIGENDIAN
#define MY_NTOHS(X) (X)
#else
#define MY_NTOHS(X) ((((X)&0xff)<<8)|(((X)>>8)&0xff))
#endif

//#define DEBUG_MESSAGES_IN 
//#define DEBUG_MESSAGES_OUT 

#define SCALE_24 8388607.f
#define SCALE_32 (8388607.f*256.f)
#define MIN_24  -8388607 
#define MAX_24  8388607  
#define HEARTBEAT 100

#define KBD_OFF 0
#define KBD_STARTING 1
#define KBD_STARTED 500

namespace
{
    static void __convert_one_float_to_24bit(unsigned char *out, float f)
    {
        int32_t x;

        if(f<=-1.0f)
            x = MIN_24;
        else if(f>=1.0f)
            x = MAX_24;
        else
            x = (long)((f > 0.0f) ? ((f*SCALE_24) + 0.5f) : ((f*SCALE_24) - 0.5f));

#ifndef PI_BIGENDIAN
        memcpy(out,&x,3);
#else
        const unsigned char *cx = (const unsigned char *)(&x);
        out[0] = cx[3];
        out[1] = cx[2];
        out[2] = cx[1];
#endif
    }

    static float __convert_one_24bit_to_float(const unsigned char *in)
    {
        int32_t x = 0;

        x = ((x<<8)|in[2]);
        x = ((x<<8)|in[1]);
        x = ((x<<8)|in[0]);
        x = (x<<8);

        float y = ((float)x)/SCALE_32;
        return y;
    }

    static void __float_to_24(unsigned char *out, const float *in, unsigned long nsamples, unsigned in_stride, unsigned out_stride)
    {
        out_stride*=3;
        for(unsigned i=0; i<nsamples; ++i)
        {
            __convert_one_float_to_24bit(out,*in);
            in+=in_stride;
            out+=out_stride;
        }
    }
}

//class alpha2::active_t::impl_t;

struct key_in_pipe: pic::usbdevice_t::iso_in_pipe_t
{

    key_in_pipe( alpha2::active_t::impl_t * ); 
    ~key_in_pipe();

    void in_pipe_data(const unsigned char *frame, unsigned size, unsigned long long fnum, unsigned long long htime, unsigned long long ptime);
 
private:

    alpha2::active_t::impl_t *pimpl_;
};

struct pedal_in_pipe: pic::usbdevice_t::iso_in_pipe_t
{

    pedal_in_pipe( alpha2::active_t::impl_t * ); 
    ~pedal_in_pipe();

    void in_pipe_data(const unsigned char *frame, unsigned size, unsigned long long fnum, unsigned long long htime, unsigned long long ptime);
 
private:

    alpha2::active_t::impl_t *pimpl_;
};
    
struct alpha2::active_t::impl_t: pic::usbdevice_t::power_t, virtual public pic::lckobject_t
{
    impl_t(pic::usbdevice_t *device, alpha2::active_t::delegate_t *, bool legacy_mode);
    ~impl_t();

    unsigned decode_keydown(const unsigned short *frame, unsigned length, unsigned long long ts);
    unsigned decode_raw(const unsigned short *frame, unsigned length, unsigned long long ts);
    unsigned decode_processed(const unsigned short *frame, unsigned length, unsigned long long ts);
    unsigned decode_mic(unsigned char seq,const unsigned short *payload, unsigned length, unsigned long long ts);
    unsigned decode_pedal(const unsigned short *payload, unsigned length, unsigned long long ts);
    unsigned decode_i2c(const unsigned char *payload, unsigned paylen, unsigned length, unsigned long long ts);
    unsigned decode_midi(const unsigned char *payload, unsigned paylen, unsigned length, unsigned long long ts);

    unsigned total_keys();

    void pipe_died(unsigned reason);
    void pipe_started();
    void pipe_stopped();

    void start();
    void stop();
    void restart();
    void kbd_start();
    void kbd_stop();
    
    bool poll(unsigned long long t);
    void msg_set_led(unsigned key, unsigned colour);
    void msg_set_led_raw(unsigned key, unsigned colour);
    void msg_send_midi(const unsigned char *data, unsigned len);
    void msg_set_leds();
    void clear_leds();

    unsigned char read_register(unsigned char addr)
    {
        PIC_ASSERT(!legacy_mode_);
        std::string s = device_->control_in(0x80|BCTKBD_USBCOMMAND_IREG_READ_REQTYPE, BCTKBD_USBCOMMAND_IREG_READ_REQ,0,0,64);
        return (unsigned char)s.c_str()[addr];
    }

    void write_register(unsigned char addr,unsigned char value)
    {
        PIC_ASSERT(!legacy_mode_);
        device_->control_out(BCTKBD_USBCOMMAND_IREG_WRITE_REQTYPE, BCTKBD_USBCOMMAND_IREG_WRITE_REQ,addr,value,0,0);
    }

    unsigned char config_wait(unsigned char reg,unsigned char mask)
    {
        unsigned ms = 0;
        for(;;)
        {
            unsigned hc = read_register(reg);
            if(hc&mask)
            {
                return hc;
            }
            pic_microsleep(10000ULL);
            if(++ms>1000)
            {
                PIC_THROW("timed out waiting for config register");
            }
        }
        return 0;
    }

    unsigned char hp_config_wait()  { return config_wait(A2_REG_HP_CONFIG,0x20); }
    void hp_config_done(unsigned char hc)  { write_register(A2_REG_HP_CONFIG,hc|0x10); }
    unsigned char mic_config_wait() { return config_wait(A2_REG_MIC_CONFIG,0x20); }
    void mic_config_done(unsigned char mc) { write_register(A2_REG_MIC_CONFIG,mc|0x10); }

    void raw_mode(bool e)
    {
        raw_mode_ = e;

        if(kbd_state_ != KBD_STARTED)
        {
            return;
        }

        if(!legacy_mode_)
        {
            unsigned char hc = read_register(A2_REG_MODE);

            if(e)
            {
                hc |= 0x01;
            }
            else
            {
                hc &= ~0x01;
            }

            write_register(A2_REG_MODE,hc);
        }
        else
        {
            if(raw_mode_)
            {
                device_->control(BCTKBD_USBCOMMAND_SETRAW_REQTYPE, BCTKBD_USBCOMMAND_SETRAW_REQ,0,0);
            }
        }

        pic::logmsg() << "raw mode enable: " << e;
    }

    void loopback_enable(bool e)
    {
        loop_enable_ = e;

        if(legacy_mode_ || tau_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char hc = read_register(A2_REG_HP_CONFIG);

        if(e)
        {
            hc |= 0x04;
        }
        else
        {
            hc &= ~0x04;
        }

        write_register(A2_REG_HP_CONFIG,hc);

        pic::logmsg() << "loopback enable: " << e;
    }

    void mic_automute(bool e)
    {
        mic_automute_ = e;

        if(legacy_mode_ || tau_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char mc = mic_config_wait();

        if(e)
        {
            mc |= 0x80;
        }
        else
        {
            mc &= ~0x80;
        }

        mic_config_done(mc);
    }

    void loopback_gain(float f)
    {
        loop_gain_ = f;

        if(legacy_mode_ || tau_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned d = 0,n = 0;

        /*
        if(f>=0.01)
        {
            d = 255U/((unsigned)ceilf(f));
            n = (unsigned)(f*((float)d));
        }
        */

        pic::logmsg() << "loop gain " << f << " -> " << n << "/" << d;
        write_register(A2_REG_LT_GAIN_MULT,n);
        write_register(A2_REG_LT_GAIN_DIV,d);
    }

    void headphone_enable(bool e)
    {
        hp_enable_ = e;

        if(legacy_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char hc = hp_config_wait();

        if(e)
        {
            hc &= ~0x01;
        }
        else
        {
            hc |= 0x01;
        }

        hp_config_done(hc);
        pic::logmsg() << "headphone enable: " << e;
    }

    void headphone_limit(bool e)
    {
        hp_limit_ = e;

        if(legacy_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char hc = hp_config_wait();

        if(e)
        {
            hc |= 0x02;
        }
        else
        {
            hc &= ~0x02;
        }

        hp_config_done(hc);
        pic::logmsg() << "headphone limit: " << e;
    }

    void headphone_gain(unsigned g)
    {
        PIC_ASSERT(g<128);

        hp_gain_ = g;

        if(legacy_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char hc = hp_config_wait();
        write_register(A2_REG_HP_GAIN,127-g);
        hp_config_done(hc);
        pic::logmsg() << "headphone gain: " << g;
    }

    void mic_gain(unsigned g)
    {
        mic_gain_ = g;

        if(legacy_mode_ || tau_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char mc = mic_config_wait();
        write_register(A2_REG_MIC_GAIN,g);
        mic_config_done(mc);
        pic::logmsg() << "mic gain: " << g;
    }

    void mic_type(unsigned t)
    {
        mic_type_ = t;

        if(legacy_mode_ || tau_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char mc = mic_config_wait();
        unsigned char g = read_register(A2_REG_MIC_GAIN);

        mic_gain(0); // just in case
        pic_microsleep(250000ULL);

        switch(t)
        {
            case 0: // dynamic
                mc &= ~0x06; // clear phantom and bias 
                break;
            case 1: // electret
                mc &= ~0x04; // clear phantom
                mc |= 0x02;  // enable bias
                break;
            case 2: // condenser
                mc &= ~0x02; // clear bias
                mc |= 0x04;  // enable phantom
                break;
        }

        mic_config_done(mc);
        mic_gain(g);
    }

    void mic_pad(bool p)
    {
        mic_pad_ = p;

        if(legacy_mode_ || tau_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char mc = mic_config_wait();
        unsigned char g = read_register(A2_REG_MIC_GAIN);

        mic_gain(0); // just in case
        pic_microsleep(250000ULL);

        if(p)
        {
            mc |= 0x01;
        }
        else
        {
            mc &= ~0x01;
        }

        mic_config_done(mc);
        mic_gain(g);
    }

    void mic_enable(bool e)
    {
        mic_enable_ = e;

        if(legacy_mode_ || tau_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned char mc = mic_config_wait();

        if(e)
        {
            mc &= ~0x40; // un mute
            mc |= 0x08;  // LED on
        }
        else
        {
            mc |= 0x40;  // mute
            mc &= ~0x08; // LED off
        }

        mic_config_done(mc);
    }

    void debounce_time(unsigned long us) // RA_TIMEOUT
    {
        ra_timeout_ = us;

        if(legacy_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned long v = std::min(us/500,63UL);
        write_register(A2_REG_RA_TIMEOUT,v);
        pic::logmsg() << "debounce time " << us;
    }

    void threshold_time(unsigned long us) // TF_TIMEOUT
    {
        tf_timeout_ = us;

        if(legacy_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned long v = std::min(us/50,255UL);
        write_register(A2_REG_TF_TIMEOUT,v);
        pic::logmsg() << "threshold filter time " << us;
    }

    void key_threshold(unsigned us)
    {
        key_thresh_ = us;

        if(legacy_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned v = std::min(us,63U);
        write_register(A2_REG_KA_THRESH,v);
        pic::logmsg() << "key threshold " << us;
    }

    void key_noise(unsigned us)
    {
        key_noise_ = us;

        if(legacy_mode_ || kbd_state_ != KBD_STARTED)
        {
            return;
        }

        unsigned v = std::min(us,15U);
        write_register(A2_REG_KA_NOISE,v);
        pic::logmsg() << "key noise " << us;
    }

    pic::usbdevice_t *device_;
    alpha2::active_t::delegate_t *handler_;
    key_in_pipe *pkey_in_pipe_;
    pedal_in_pipe *ppedal_in_pipe_;
    pic::usbdevice_t::iso_out_pipe_t out_pipe_;
    pic::bulk_queue_t led_pipe_;
    //pic::bulk_queue_t midi_pipe_;
    bool keydown_recv_;
    unsigned heartbeat_;
    unsigned char ledstates_[132];
    bool noleds_;
    bool legacy_mode_;
    bool mic_suppressed_;
    unsigned active_colour_;
    bool tau_mode_;

    bool mic_pad_,mic_enable_,hp_enable_,loop_enable_,mic_automute_,hp_limit_,raw_mode_;
    unsigned mic_type_,mic_gain_,hp_gain_;
    float loop_gain_;
    unsigned long long ra_timeout_, tf_timeout_;
    unsigned key_noise_;
    unsigned key_thresh_;

    unsigned kbd_state_;
};

key_in_pipe::key_in_pipe( alpha2::active_t::impl_t *pimpl ):
    pic::usbdevice_t::iso_in_pipe_t(BCTKBD_USBENDPOINT_ISO_IN_NAME,BCTKBD_USBENDPOINT_ISO_IN_SIZE),
    pimpl_( pimpl )
{
}

key_in_pipe::~key_in_pipe()
{

}

pedal_in_pipe::pedal_in_pipe( alpha2::active_t::impl_t *pimpl ):
    pic::usbdevice_t::iso_in_pipe_t(BCTP_USBENDPOINT_ISO_IN_NAME,BCTP_USBENDPOINT_ISO_IN_SIZE),
    pimpl_( pimpl )
{
}

pedal_in_pipe::~pedal_in_pipe()
{
}

alpha2::active_t::impl_t::impl_t(pic::usbdevice_t *device, alpha2::active_t::delegate_t *del, bool legacy_mode): 
    device_(device),
    handler_(del),
    out_pipe_(BCTKBD_USBENDPOINT_ISO_OUT_NAME,BCTKBD_USBENDPOINT_ISO_OUT_SIZE),
    led_pipe_(/*BCTKBD_BULKOUT_LED_EP_SIZE*/36,device,BCTKBD_BULKOUT_LED_EP,500,0),
    //midi_pipe_(BCTP_BULKOUT_MIDI_EP_SIZE,device,BCTP_BULKOUT_MIDI_EP,500,1),
    keydown_recv_(false),
    heartbeat_(0),
    legacy_mode_(legacy_mode),
    mic_suppressed_(false),
    active_colour_(0x03),
    tau_mode_(false),
    mic_pad_(true), mic_enable_(false), hp_enable_(false), loop_enable_(false), mic_automute_(false), hp_limit_(true),raw_mode_(false),
    mic_type_(1), mic_gain_(0x15), hp_gain_(0x46), ra_timeout_(20000), tf_timeout_(5000), key_noise_(8), key_thresh_(25),
    kbd_state_(KBD_OFF)
{
    device_->set_power_delegate(this);
    pkey_in_pipe_ = new key_in_pipe( this );
    device_->add_iso_in( pkey_in_pipe_ );
    pkey_in_pipe_->enable_frame_check(true);

    noleds_ = (getenv("PI_NOLEDS")!=0);

    if(!noleds_)
    {
        ppedal_in_pipe_ = new pedal_in_pipe( this );
        device_->add_iso_in( ppedal_in_pipe_ );
    }
    
    memset(ledstates_,0,sizeof(ledstates_));
    device_->set_iso_out(&out_pipe_);

    if(legacy_mode_)
    {
        pic::logmsg() << "device is legacy mode, no audio or configuration registers available";
    }

    device_->control_out(BCTKBD_USBCOMMAND_STOP_REQTYPE,BCTKBD_USBCOMMAND_STOP_REQ,0,0,0,0);
    device_->control_out(BCTKBD_USBCOMMAND_STOP_REQTYPE,BCTP_USBCOMMAND_STOP_REQ,0,0,0,0); 
}

alpha2::active_t::impl_t::~impl_t()
{
    stop();
    device_->detach();
    delete pkey_in_pipe_;
    
    if(getenv("PI_NOLEDS")==0)
    {
        delete ppedal_in_pipe_;
    }
}

void pedal_in_pipe::in_pipe_data(const unsigned char *frame, unsigned length, unsigned long long hf, unsigned long long ht,unsigned long long pt)
{   
#ifdef DEBUG_MESSAGES_IN
    if(length>0)
    {
        pic::msg_t msg;

        msg << "\n- iso IN " << length << " bytes ";
        for(unsigned o=0; o<length; ++o)
        {
            msg << std::hex << ' ' << (int)(frame[o]);
        }

        pic::log(msg);
    }
#endif

    unsigned l = length/2;
    unsigned short o;
    const unsigned short *p = (const unsigned short *)frame;

    while(l>BCTKBD_HEADER3_SIZE)
    {
        pik_msg2_t *m2 = (pik_msg2_t *)p;
        pik_msg3_t *m3 = (pik_msg3_t *)p;
        pik_msg4_t *m4 = (pik_msg4_t *)p;

        switch(m3->type)
        {
            case 0: return;
            case BCTKBD_MSGTYPE_PEDAL: o=pimpl_->decode_pedal(m2->payload,l,ht); break;
            case BCTKBD_MSGTYPE_MIDI: o=pimpl_->decode_midi(m4->payload,m4->len,l,ht); break;
            case BCTKBD_MSGTYPE_I2C: o=pimpl_->decode_i2c(m4->payload,m4->len,l,ht); break;
            default: pic::logmsg() << "x invalid usb message type " << (unsigned)m3->type; return;
        }

        //pic::logmsg() << "type " << (int)(m3->type) << " consumed " << o;

        if(o==0)
        {
            return;
        }

        p+=o;
        l-=o;
        ht+=10;
    }
}

unsigned alpha2::active_t::impl_t::decode_midi(const unsigned char *payload, unsigned paylen, unsigned length, unsigned long long ts)
{
    unsigned l2 = 1+(paylen+1)/2;

    if(length<l2)
    {
        return 0;
    }

    handler_->midi_data(ts,payload,paylen);
    return l2;
}

unsigned alpha2::active_t::impl_t::decode_i2c(const unsigned char *payload, unsigned paylen, unsigned length, unsigned long long ts)
{
    unsigned l2 = 1+(paylen+1)/2;

    if(length<l2)
    {
        return 0;
    }

    return l2;
}

unsigned alpha2::active_t::impl_t::decode_pedal(const unsigned short *payload, unsigned length, unsigned long long ts)
{
    if(length < BCTKBD_MSGSIZE_PEDAL)
    {
        return 0;
    }

    unsigned short pedal1 = 0;
    unsigned short pedal2 = 0;
    unsigned short switch1 = 0;
    unsigned short switch2 = 0;
    
    pedal1 = payload[1] & 0x0fff;
    pedal2 = payload[0] & 0x0fff;
    switch1 = ((payload[2]&0x01)!=0)?4095:0;
    switch2 = ((payload[2]&0x02)!=0)?4095:0;
  
    handler_->pedal_down(ts,1,pedal1);
    handler_->pedal_down(ts,2,pedal2);
    handler_->pedal_down(ts,3,switch1);
    handler_->pedal_down(ts,4,switch2);

    return BCTKBD_MSGSIZE_PEDAL;
}

void key_in_pipe::in_pipe_data(const unsigned char *frame, unsigned length, unsigned long long hf, unsigned long long ht,unsigned long long pt)
{
#ifdef DEBUG_MESSAGES_IN

    if(length>0)
    {
        printf("\n- iso IN %d bytes ---------\n", length);
        for(unsigned o=0; o<length; ++o)
        {
            if((o%32)==0)
                printf("\n");
            printf("%02x ",frame[o]);
        }
        printf("\n-----------\n");
    }

#endif

    unsigned l=length/2;
    unsigned short o;
    const unsigned short *p = (const unsigned short *)frame;


    while(l>BCTKBD_HEADER2_SIZE)
    {
        pik_msg1_t *m = (pik_msg1_t *)p;
        pik_msg2_t *m2 = (pik_msg2_t *)p;

        //pic::logmsg() << "message type " << (int)(m->type);

        switch(m->type)
        {
            case 0: case BCTKBD_MSGTYPE_NULL:          return;
            case BCTKBD_MSGTYPE_KEYDOWN:       o= pimpl_->decode_keydown(m->payload,l,ht); break;
            case BCTKBD_MSGTYPE_RAW:           o= pimpl_->decode_raw(m->payload,l,ht); break;
            case BCTKBD_MSGTYPE_PROCESSED:     o= pimpl_->decode_processed(m->payload,l,ht); break;
            case BCTKBD_MSGTYPE_MIC:           o= pimpl_->decode_mic(m2->frame,m2->payload,l,ht); break;

            default: pic::logmsg() << "x invalid usb message type " << (unsigned)m->type; return;
        }

        if(o==0)
        {
            return;
        }

        p+=o;
        l-=o;
        ht+=10;
    }
}

alpha2::active_t::active_t(pic::usbdevice_t *device, alpha2::active_t::delegate_t *handler, bool legacy_mode)
{
    _impl = new impl_t(device,handler, legacy_mode);
}

alpha2::active_t::~active_t()
{
    if(_impl)
        delete _impl;
}

void alpha2::active_t::impl_t::start()
{
    pic::logmsg() << "starting pipes";
    device_->start_pipes();
    led_pipe_.start();
    //midi_pipe_.start();
}

void alpha2::active_t::impl_t::kbd_start()
{
    device_->control_out(BCTKBD_USBCOMMAND_START_REQTYPE,BCTP_USBCOMMAND_START_REQ,0,0,0,0);
    device_->control_out(BCTKBD_USBCOMMAND_START_REQTYPE,BCTKBD_USBCOMMAND_START_REQ,0,0,0,0); 
    //device_->control_out(BCTKBD_USBCOMMAND_START_REQTYPE,BCTP_USBCOMMAND_START_MIDI_REQ,0,0,0,0);

    if(!legacy_mode_)
    {
        pic_microsleep(10000); // allow comms link to reinitialise
        write_register(A2_REG_MODE,0x18); // 512 byte mode, start data transmission
    }

    kbd_state_ = KBD_STARTING;
    heartbeat_ = 0;
}

void alpha2::active_t::impl_t::kbd_stop()
{
    kbd_state_ = KBD_OFF;

    if(!legacy_mode_)
    {
        write_register(A2_REG_MODE,0x10); // 512 byte mode, stop data transmission
    }

    //device_->control_out(BCTKBD_USBCOMMAND_STOP_REQTYPE,BCTP_USBCOMMAND_STOP_MIDI_REQ,0,0,0,0); 
    device_->control_out(BCTKBD_USBCOMMAND_STOP_REQTYPE,BCTP_USBCOMMAND_STOP_REQ,0,0,0,0); 
    device_->control_out(BCTKBD_USBCOMMAND_STOP_REQTYPE,BCTKBD_USBCOMMAND_STOP_REQ,0,0,0,0);
}

void alpha2::active_t::start()
{
    _impl->start();
}

void alpha2::active_t::impl_t::stop()
{
    clear_leds();
    //pic::logmsg() << " alpha2::active_t::impl_t::stop IN";
    led_pipe_.stop();
    //midi_pipe_.stop();
    device_->stop_pipes();
}

void alpha2::active_t::stop()
{
    _impl->stop();
}

void alpha2::active_t::invalidate()
{
    if(_impl)
    {
        delete _impl;
        _impl=0;
    }
}

bool alpha2::active_t::impl_t::poll(unsigned long long t)
{
    bool v;
	keydown_recv_=false;
    v = device_->poll_pipe(t);

    if(kbd_state_ == KBD_OFF) return v;

    if(kbd_state_ != KBD_STARTED)
    {
        if(kbd_state_ >= KBD_STARTING)
        {
            kbd_state_++;
        }

        if(kbd_state_ == KBD_STARTED)
        {
            bool mg = mic_gain_;

            pic::logmsg() << "keyboard startup phase 2";
            kbd_state_ = KBD_STARTED;
            msg_set_leds();


            raw_mode(raw_mode_);
            loopback_enable(loop_enable_);
            mic_automute(mic_automute_);
            loopback_gain(loop_gain_);
            headphone_enable(hp_enable_);
            headphone_limit(hp_limit_);
            headphone_gain(hp_gain_);
            mic_type(mic_type_);
            mic_pad(mic_pad_);
            mic_enable(mic_enable_);
            mic_gain(mg);
            debounce_time(ra_timeout_);
            threshold_time(tf_timeout_);
            key_threshold(key_thresh_);
            key_noise(key_noise_);
        }
    }

    if(!v && !raw_mode_)
    {
        if(keydown_recv_)
        {
            heartbeat_=0;
        }
        else
        {
            heartbeat_++;
            if(heartbeat_==500)
            {
                pic::logmsg() << "input data stopped, keyboard shutting down";
                handler_->kbd_dead(PIPE_NOT_RESPONDING);
                //stop();
            }
        }
    }

    return v;
}

bool alpha2::active_t::poll(unsigned long long t)
{
    return _impl->poll(t);
}

void alpha2::active_t::impl_t::msg_set_leds()
{
    pic::logmsg() << "refreshing lights";

    for(unsigned k=0;k<total_keys();k++)
    {
        msg_set_led_raw(k,ledstates_[k]);
    }
    led_pipe_.flush();
}

void alpha2::active_t::impl_t::clear_leds()
{
    pic::logmsg() << "clearing lights";

    for(unsigned k=0;k<total_keys();k++)
    {
        msg_set_led_raw(k,0);
    }
    led_pipe_.flush();
}

void alpha2::active_t::impl_t::msg_send_midi(const unsigned char *data, unsigned len)
{
    if(kbd_state_ == KBD_STARTED)
    {
        unsigned char msg[10];

        while(len>0)
        {
            unsigned l2 = std::min(len,8U);
            unsigned l3 = l2;
            if((l3&1)!=0) l3++;

            memset(msg,0,sizeof(msg));

            msg[0] = BCTKBD_MIDI_MSGTYPE;
            msg[1] = l2;
            memcpy(&msg[2],data,l2);

            try
            {
                //midi_pipe_.write(msg,l3+2);
                //midi_pipe_.flush();
            }
            CATCHLOG()

            len -= l2;
            data += l2;
        }
    }
}

unsigned alpha2::active_t::impl_t::total_keys()
{
    if(tau_mode_)
    {
        return TAU_KBD_KEYS+TAU_MODE_KEYS+TAU_KEYS_OFFSET;
    }
    else
    {
        return KBD_KEYS;
    }
}

void alpha2::active_t::impl_t::msg_set_led(unsigned key, unsigned colour)
{
    if(tau_mode_ && key>=TAU_KBD_KEYS)
    {
        key+=TAU_KEYS_OFFSET;
    }

    ledstates_[key]=colour&0xff;

    if(kbd_state_ == KBD_STARTED)
    {
        msg_set_led_raw(key, colour);
    }
}

void alpha2::active_t::impl_t::msg_set_led_raw(unsigned key, unsigned colour)
{
    bool g = (colour&1)!=0;
    bool r = (colour&2)!=0;
    unsigned char msg[BCTKBD_SETLED_MSGSIZE*2];
    msg[0] = BCTKBD_SETLED_MSGTYPE;
    msg[1] = 0x00;
    msg[2] = key;
    msg[3] = colour | active_colour_;
    memset(msg+4,g?0xff:0x00,4);
    memset(msg+8,r?0xff:0x00,4);

    try
    {
        led_pipe_.write(msg,BCTKBD_SETLED_MSGSIZE*2);
    }
    CATCHLOG()
}

void alpha2::active_t::msg_send_midi(const unsigned char *data, unsigned len)
{
    _impl->msg_send_midi(data,len);
}

void alpha2::active_t::msg_set_led(unsigned key, unsigned colour)
{
    _impl->msg_set_led(key,colour);
}

const char *alpha2::active_t::get_name()
{
    return _impl->device_->name();
}

static void __write16(unsigned char *raw, unsigned i, unsigned short val)
{
    raw[i*2] = (val>>8)&0x00ff;
    raw[i*2+1] = (val>>0)&0x00ff;
}

void alpha2::active_t::msg_set_calibration(unsigned key, unsigned corner, unsigned short min, unsigned short max, const unsigned short *table)
{
    unsigned char raw[2*BCTKBD_CALTABLE_SIZE];

    __write16(raw,BCTKBD_CALTABLE_MIN,min);
    __write16(raw,BCTKBD_CALTABLE_MAX,max);

    for(unsigned i=0;i<BCTKBD_CALTABLE_POINTS;i++)
    {
        __write16(raw,BCTKBD_CALTABLE_DATA+i,table[i]);
    }

    _impl->device_->control_out(BCTKBD_USBCOMMAND_CALDATA_REQTYPE,BCTKBD_USBCOMMAND_CALDATA_REQ,corner,key,raw,64);
}

void alpha2::active_t::msg_write_calibration()
{
    _impl->device_->control(BCTKBD_USBCOMMAND_CALWRITE_REQTYPE,BCTKBD_USBCOMMAND_CALWRITE_REQ,0,0);
}

void alpha2::active_t::msg_clear_calibration()
{
    _impl->device_->control(BCTKBD_USBCOMMAND_CALCLEAR_REQTYPE,BCTKBD_USBCOMMAND_CALCLEAR_REQ,0,0);
}

unsigned alpha2::active_t::get_temperature()
{
    unsigned char msg[16];
    _impl->device_->control_in(0x80|BCTKBD_USBCOMMAND_TEMP_REQTYPE,BCTKBD_USBCOMMAND_TEMP_REQ,0,0,msg,16);
    return msg[0];
}

/*
static unsigned long long adjust_time(unsigned long long hf, unsigned long long ht, unsigned short thi, unsigned short tlo)
{
    unsigned short offset = (hf-thi)&0x7ff;
    return tlo + (ht-1000*offset);
}
*/

unsigned alpha2::active_t::impl_t::decode_keydown(const unsigned short *payload, unsigned length, unsigned long long ts)
{
    if(length<BCTKBD_MSGSIZE_KEYDOWN)
    {
        return 0;
    }

    keydown_recv_ = true;

#ifdef PI_BIGENDIAN
    unsigned short bitmap[BCTKBD_PAYLOAD_KEYDOWN];

    for(unsigned i=0;i<BCTKBD_PAYLOAD_KEYDOWN;i++)
    {
        bitmap[i] = MY_NTOHS(payload[i]);
    }

    handler_->kbd_keydown(ts, bitmap);
#else
    handler_->kbd_keydown(ts, payload);
#endif
    return BCTKBD_MSGSIZE_KEYDOWN;
}

unsigned alpha2::active_t::impl_t::decode_raw(const unsigned short *payload, unsigned length, unsigned long long ts)
{
    if(length<BCTKBD_MSGSIZE_RAW)
    {
        return 0;
    }

    unsigned short key = MY_NTOHS(payload[BCTKBD_MSG_RAW_KEY]);
    unsigned short c0 = MY_NTOHS(payload[BCTKBD_MSG_RAW_I0]);
    unsigned short c1 = MY_NTOHS(payload[BCTKBD_MSG_RAW_I1]);
    unsigned short c2 = MY_NTOHS(payload[BCTKBD_MSG_RAW_I2]);
    unsigned short c3 = MY_NTOHS(payload[BCTKBD_MSG_RAW_I3]);

    handler_->kbd_raw(ts, key, c0, c1, c2, c3);

    return BCTKBD_MSGSIZE_RAW;
}

unsigned alpha2::active_t::impl_t::decode_mic(unsigned char seq, const unsigned short *payload, unsigned length, unsigned long long ts)
{
    if(legacy_mode_)
    {
        return length;
    }

    if(length<BCTKBD_MSGSIZE_MIC)
    {
        return 0;
    }

    if(mic_suppressed_)
    {
        return BCTKBD_MSGSIZE_MIC;
    }

    float out[16];
    unsigned char *payloadc = (unsigned char *)payload;

    for(unsigned i=0;i<16;i++)
    {
        out[i] = __convert_one_24bit_to_float(payloadc);
        payloadc += 3;
    }

    handler_->kbd_mic(seq,ts,out);
    return BCTKBD_MSGSIZE_MIC;
}

unsigned alpha2::active_t::impl_t::decode_processed(const unsigned short *payload, unsigned length, unsigned long long ts)
{
    if(length<BCTKBD_MSGSIZE_PROCESSED)
    {
        return 0;
    }

    unsigned short key = MY_NTOHS(payload[BCTKBD_MSG_PROCESSED_KEY]);
    unsigned short pressure = MY_NTOHS(payload[BCTKBD_MSG_PROCESSED_P]);
    unsigned short roll = MY_NTOHS(payload[BCTKBD_MSG_PROCESSED_R]);
    unsigned short yaw = MY_NTOHS(payload[BCTKBD_MSG_PROCESSED_Y]);

    handler_->kbd_key(ts, key, pressure, roll, yaw);
    return BCTKBD_MSGSIZE_PROCESSED;
}

void alpha2::active_t::impl_t::pipe_died(unsigned reason)
{
    pipe_stopped();
    handler_->kbd_dead(reason);
}

void alpha2::active_t::impl_t::pipe_stopped()
{
    try
    {
        pic::logmsg() << "keyboard shutdown";
        kbd_stop();
    }
    catch(...)
    {
        pic::logmsg() << "device shutdown command failed";
    }

}

void alpha2::active_t::impl_t::pipe_started()
{
    pic::logmsg() << "keyboard startup";
    kbd_start();
}

void alpha2::active_t::impl_t::restart()
{
    pic::logmsg() << "starting up keyboard";

    try
    {
        kbd_start();
    }
    catch(...)
    {
        pic::logmsg() << "device startup failed";
    }

    pic::logmsg() << "started up keyboard";
}

void alpha2::active_t::msg_test_write_lib(unsigned idx, unsigned p, unsigned r, unsigned y)
{
    unsigned char msg[BCTKBD_TEST_MSGSIZE_LIB*2];
    msg[0] = idx;
    msg[1] = BCTKBD_TEST_MSGTYPE_LIB;
    msg[2] = p&0xff;
    msg[3] = (p>>8)&0xff;
    msg[4] = r&0xff;
    msg[5] = (r>>8)&0xff;
    msg[6] = y&0xff;
    msg[7] = (y>>8)&0xff;
    _impl->led_pipe_.write(msg,BCTKBD_TEST_MSGSIZE_LIB*2);
}

void alpha2::active_t::msg_test_finish_lib()
{
    unsigned char msg[BCTKBD_TEST_MSGSIZE_LIB*2];
    memset(msg,0xff,BCTKBD_TEST_MSGSIZE_LIB*2);
    msg[1] = BCTKBD_TEST_MSGTYPE_LIB|0x3f;
    _impl->led_pipe_.write(msg,BCTKBD_TEST_MSGSIZE_LIB*2);
}

void alpha2::active_t::msg_test_write_seq(unsigned lib, unsigned key, unsigned time)
{
    unsigned char msg[BCTKBD_TEST_MSGSIZE_SEQ*2];
    msg[0] = key;
    msg[1] = BCTKBD_TEST_MSGTYPE_SEQ|lib;
    msg[2] = time&0xff;
    msg[3] = (time>>8)&0xff;
    _impl->led_pipe_.write(msg,BCTKBD_TEST_MSGSIZE_SEQ*2);
}

void alpha2::active_t::msg_test_finish_seq()
{
    unsigned char msg[BCTKBD_TEST_MSGSIZE_SEQ*2];
    memset(msg,0xff,BCTKBD_TEST_MSGSIZE_SEQ*2);
    msg[1] = BCTKBD_TEST_MSGTYPE_SEQ|0x3f;
    _impl->led_pipe_.write(msg,BCTKBD_TEST_MSGSIZE_START*2);
}

void alpha2::active_t::msg_test_start()
{
    unsigned char msg[BCTKBD_TEST_MSGSIZE_START*2];
    memset(msg,0x00,BCTKBD_TEST_MSGSIZE_START*2);
    msg[1] = BCTKBD_TEST_MSGTYPE_START;
    _impl->led_pipe_.write(msg,BCTKBD_TEST_MSGSIZE_START*2);
}

void alpha2::printer_t::kbd_dead(unsigned reason)
{
    pic::printmsg() << "(dead)";
}

void alpha2::printer_t::kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4)
{
    pic::printmsg() << "(raw) t:" << t << " k:" << key <<  " c:" << c1 << "," << c2 << "," << c3 << "," << c4;
}

void alpha2::printer_t::kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y)
{
    pic::printmsg() << "(cooked) t:" << t << " k:" << key <<  " p:" << p << " r:" << r << " y:" << y;
}

void alpha2::printer_t::kbd_keydown(unsigned long long t, const unsigned short *bitmap)
{
    pic::msg_t m(pic::printmsg());
    m << "(bitmap) t:" << t << " map:";
    for(unsigned i=0; i<KBD_KEYS; i++)
    {
        m << (active_t::keydown(i,bitmap)?"X":"-");
    }
}

void alpha2::printer_t::midi_data(unsigned long long t, const unsigned char *data, unsigned len)
{
    printf("\n-  alpha2::printer_t::midi_data IN---------\n");
}

void alpha2::printer_t::pedal_down(unsigned long long t, unsigned pedal, unsigned p)
{
    printf("\n-  alpha2::printer_t::pedal_down IN---------\n");
}

void alpha2::active_t::set_tau_mode(bool tau_mode)
{
    _impl->tau_mode_ = tau_mode;
}

void alpha2::active_t::audio_write(const float *stereo, unsigned len, unsigned period)
{
    pic::usbdevice_t::iso_out_guard_t guard(_impl->device_);

    if(!guard.isvalid())
    {
        return;
    }

    unsigned pkts = len/BCTKBD_AUDIO_PKT_FRAMES;
    unsigned char *frame = guard.current();
    unsigned index = 0;

    if(!frame)
    {
        return;
    }

    if(period>0)
    {
        // start audio header
        guard.dirty();
        frame[index++] = 0x09;
        frame[index++] = period;
    }

    for(unsigned i=0; i<pkts; i++)
    {
        guard.dirty();
        frame[index+0] = 0x06;
        frame[index+1] = 2*3*BCTKBD_AUDIO_PKT_FRAMES;
        __float_to_24(&frame[index+2],stereo,2*BCTKBD_AUDIO_PKT_FRAMES,1,1);
        
        index+=BCTKBD_AUDIO_PKT_SIZE;
        stereo+=(2*BCTKBD_AUDIO_PKT_FRAMES);

        if((index+BCTKBD_AUDIO_PKT_SIZE)>BCTKBD_USBENDPOINT_ISO_OUT_SIZE)
        {
            frame = guard.advance();

            if(!frame)
            {
                return;
            }

            index=0;
        }
    }

    unsigned remain = len%BCTKBD_AUDIO_PKT_FRAMES;
    if(remain>0)
    {
        guard.dirty();
        frame[index+0] = 0x06;
        frame[index+1] = 2*3*remain;
        __float_to_24(&frame[index+2],stereo,2*remain,1,1);
    }
}

void alpha2::active_t::msg_flush()
{
    _impl->led_pipe_.flush();
    //_impl->midi_pipe_.flush();
}

void alpha2::active_t::mic_type(unsigned t)
{
    pic::logmsg() << "mic type " << t;
    _impl->mic_type(t);
}

void alpha2::active_t::mic_gain(unsigned g)
{
    pic::logmsg() <<  "mic gain " << g;
    _impl->mic_gain(g);
}

void alpha2::active_t::mic_pad(bool p)
{
    pic::logmsg() << "mic pad " << p;
    _impl->mic_pad(p);
}

void alpha2::active_t::mic_automute(bool e)
{
    pic::logmsg() << "mic automute en " << e;
    _impl->mic_automute(e);
}

void alpha2::active_t::mic_enable(bool e)
{
    pic::logmsg() << "mic en " << e;
    _impl->mic_enable(e);
}

void alpha2::active_t::loopback_gain(float f)
{
    pic::logmsg() << "loopback gain " << f;
    _impl->loopback_gain(f);
}

void alpha2::active_t::loopback_enable(bool e)
{
    pic::logmsg() << "loopback enable " << e;
    _impl->loopback_enable(e);
}

void alpha2::active_t::headphone_limit(bool e)
{
    pic::logmsg() << "headphone limit " << e;
    _impl->headphone_limit(e);
}

void alpha2::active_t::headphone_enable(bool e)
{
    pic::logmsg() << "headphone enable " << e;
    _impl->headphone_enable(e);
}

void alpha2::active_t::headphone_gain(unsigned g)
{
    pic::logmsg() << "headphone gain " << g;
    _impl->headphone_gain(g);
}

void alpha2::active_t::mic_suppress(bool s)
{
    pic::logmsg() << "mic suppress " << s;
    _impl->mic_suppressed_ = s;
}

void alpha2::active_t::set_active_colour(unsigned c)
{
    _impl->active_colour_ = c;
}

void alpha2::active_t::set_raw(bool raw)
{
    pic::logmsg() << "raw mode " << raw;
    _impl->raw_mode(raw);
}

void alpha2::active_t::restart()
{
    pic::logmsg() << "restart keyboard";
    _impl->restart();
}

void alpha2::active_t::debounce_time(unsigned long us)
{
    _impl->debounce_time(us);
}

void alpha2::active_t::threshold_time(unsigned long long us)
{
    _impl->threshold_time(us);
}

void alpha2::active_t::key_threshold(unsigned v)
{
    _impl->key_threshold(v);
}

void alpha2::active_t::key_noise(unsigned v)
{
    _impl->key_noise(v);
}
