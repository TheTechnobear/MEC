
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


#include <picross/pic_time.h>
#include <picross/pic_usb.h>

#define INTERVAL 40000

int main(int ac, char **av)
{
    const char *usbdev = 0;

    if(ac==2)
    {
        usbdev=av[1];
    }
    else
    {
        try
        {
            usbdev = pic::usbenumerator_t::find(0x2139,0x0104).c_str();
        }
        catch(...)
        {
            fprintf(stderr,"can't find a keyboard\n");   
            exit(-1);
        }
    }

    printf("found device %s\n",usbdev);

    pic::usbdevice_t::iso_out_pipe_t pipe(6,128);
    pic::usbdevice_t loop(usbdev,0);
	loop.set_iso_out(&pipe);

    loop.control_out(0x40,0xb1,0,0,0,0);

    unsigned char data[128];
    memset(data,0xff,128);

    while(1)
    {
        pic_microsleep(INTERVAL);
        pic::usbdevice_t::iso_out_guard_t g(&loop);
        if(g.isvalid())
            memcpy(g.current(),data,128);
    }

    return 0;
}
