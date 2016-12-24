
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

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <picross/pic_config.h>
#ifdef PI_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif


#ifndef PI_BIGENDIAN
#define MY_NTOHS(X) (X)
#else
#define MY_NTOHS(X) ((((X)&0xff)<<8)|(((X)>>8)&0xff))
#endif


//#include <pikeyboard/performotron_usb.h>
#include <lib_pico/pico_active.h>
#include <picross/pic_usb.h>
#include <picross/pic_time.h>

#define BCTPICO_USBVENDOR 0x2139
#define BCTPICO_USBPRODUCT 0x0101

struct printer_t: public  pico::active_t::delegate_t
{
    printer_t(): count(0) {}

    void kbd_dead(unsigned reason)
    {
        printf("keyboard disconnected\n");
        exit(0);
    }

    void kbd_key(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y)
    {
       printf("%llu %u %u %d %d\n",t,key,p,r,y);
    }

    void kbd_raw(bool resync,const pico::active_t::rawkbd_t &raw) 
    {
        
        const pico::active_t::rawkey_t *key = raw.keys;

        for( int i = 0; i < 20; i++)
        {
	    unsigned short c0 = MY_NTOHS(key[i].c[0]);
	    unsigned short c1 = MY_NTOHS(key[i].c[1]);
	    unsigned short c2 = MY_NTOHS(key[i].c[2]);
	    unsigned short c3 = MY_NTOHS(key[i].c[3]);
            if(i==0) printf( "key: %02d c1:%04d, c2:%04d, c3:%04d, c4:%04d\n",i,c0,c1,c2,c3); 
        }
    
    }
    unsigned char count;
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
    std::string usbdev;

    if(ac==2)
    {
        usbdev=av[1];
    }
    else
    {
        try
        {
            usbdev = pic::usbenumerator_t::find(BCTPICO_USBVENDOR,BCTPICO_USBPRODUCT);
        }
        catch(...)
        {
            fprintf(stderr,"can't find a keyboard\n");   
            exit(-1);
        }
    }

    printer_t printer;
    pico::active_t loop(usbdev.c_str(), &printer);
    loop.set_raw(true);

    loop.load_calibration_from_device();
    pic_microsleep(1000000);
    loop.start();
    unsigned long long t=0;
    unsigned c=0;
    
    while(1)
    {   
		pic_microsleep(10000);
		t=pic_microtime();
        loop.poll(t);
        unsigned x=(t/1000000)%4;
        if(x!=c)
        {
        	loop.set_led(1,x);
        	c=x;
        }
	}

    return 0;
}
