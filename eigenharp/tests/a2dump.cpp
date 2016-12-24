
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

#ifndef PI_WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif


#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <lib_alpha2/alpha2_usb.h>
#include <lib_alpha2/alpha2_active.h>
#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_log.h>

struct printer_t: public  alpha2::active_t::delegate_t
{
    printer_t(int k): loop_(0), count_(0), key_(k)
    {
        memset(curmap_,0,sizeof(curmap_));
        memset(pedal_,0,sizeof(pedal_));
        memset(lights_,0,sizeof(lights_));
    }

    void kbd_dead(unsigned reason)
    {
        pic::logmsg() << "kbd disconnected";
        exit(0);
    }

    void kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y)
    {
        if(key_>0)
        {
            if((int)key!=key_)
            {
                return;
            }
        }
        else
        {
            if(!count_)
            {
                return;
            }

            count_--;
        }

        pic::logmsg() << "cooked: " << t << ' ' << key << ' ' << p << ' ' << r << ' ' << y;
    }

    void kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4)
    {
        pic::logmsg() << "raw: " << key << ' ' << std::hex << c1 << ' ' << c2 << ' ' << c3 << ' ' << c4;
    }

    void kbd_keydown(unsigned long long t, const unsigned short *bitmap)
    {
        for(unsigned k=0; k<KBD_KEYS+KBD_SENSORS+1; ++k)
        {
            bool a = alpha2::active_t::keydown(k,bitmap);

            if(k<KBD_KEYS)
            {
                if(a)
                {
                    if(!lights_[k])
                    {
                        loop_->msg_set_led(k,2);
                    }

                    lights_[k]=1000;
                }
                else
                {
                    if(lights_[k])
                    {
                        lights_[k]--;
                        if(!lights_[k])
                        {
                            loop_->msg_set_led(k,0);
                        }
                    }
                }
                
            }

            loop_->msg_flush();
        }

        if(memcmp(bitmap,curmap_,18)==0)
            return;

        memcpy(curmap_,bitmap,18);

        pic::msg_t m = pic::msg();
        for(unsigned k=0; k<KBD_KEYS+KBD_SENSORS+1; ++k)
        {
            bool a = alpha2::active_t::keydown(k,bitmap);

            m << (a?"X":"-");

            if(k<KBD_KEYS)
            {
                if(a)
                {
                    if(!lights_[k])
                    {
                        loop_->msg_set_led(k,2);
                        pic::logmsg() << "turning on " << k;
                    }

                    lights_[k]=200;
                }
                else
                {
                    if(lights_[k])
                    {
                        lights_[k]--;
                        if(!lights_[k])
                        {
                            loop_->msg_set_led(k,0);
                            pic::logmsg() << "turning off " << k;
                        }
                    }
                }
                
            }
        }
        m << pic::log;

        count_ = 10;
    }

    void kbd_mic(unsigned char s,unsigned long long t, const float *data)
    {
        //for(unsigned i=0;i<16;i++) { printf("%f\n",data[i]); }
        if(s!=seq_)
        {
            pic::logmsg() << "seq: " << (int)s << ' ' << (int)seq_;
        }

        seq_=(s+1)&0xff;
    }

    void midi_data(unsigned long long t, const unsigned char *data, unsigned len)
    {
    }

    void pedal_down(unsigned long long t, unsigned pedal, unsigned value)
    {
        /*
        if(value != pedal_[pedal-1])
        {
            pic::logmsg() << "pedal: " << pedal << ' ' << value;
            pedal_[pedal-1] = value;
        }
        */
    }

    alpha2::active_t *loop_;
    unsigned pedal_[4];
    unsigned short curmap_[9];
    unsigned count_;
    int key_;
    unsigned char seq_;
    unsigned lights_[KBD_KEYS];
};

static volatile int keepRunning = 1;
void intHandler(int dummy) {
    std::cerr  << "int handler called";
    if(keepRunning==0) {
        sleep(1);
        exit(-1);
    }
    keepRunning = 0;
}

int main(int ac, char **av)
{
    signal(SIGINT, intHandler);

    std::string usbdev;
    int key = -1;

    try
    {
        usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,BCTKBD_USBPRODUCT).c_str();
    }
    catch(...)
    {
        fprintf(stderr,"can't find a keyboard\n");   
        exit(-1);
    }

    if(ac==2)
    {
        key=atoi(av[1]);
        fprintf(stderr,"using key %d\n",key);
    }

    pic_init_time();
    pic_set_foreground(true);

    {
    	try 
    	{
        printer_t printer(key);
        pic::usbdevice_t device(usbdev.c_str(),0);
        alpha2::active_t loop(&device, &printer);
        printer.loop_ = &loop;
        loop.start();
        //loop.set_raw(true);

        while(keepRunning)
        {
            loop.poll(pic_microtime());
            loop.msg_flush();
            pic_microsleep(5000);
        }
		
    	}
    	catch(...){
            fprintf(stderr,"exception during poll ");   
    	}

    }

    return 0;
}
