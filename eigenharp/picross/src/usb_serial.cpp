
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

#include <picross/pic_log.h>
#include <picross/pic_usb.h>

std::string pic::usb_string(pic::usbdevice_t *device, unsigned index)
{
    unsigned char dscr_str[256];
    char str[128];

    if(index==0)
    {
        return std::string();
    }

    device->control_in(0x80,6,0x0300|index,0,dscr_str,256);

    unsigned slen = dscr_str[0]/2;

    if(slen>sizeof(str))
    {
        PIC_THROW("serial# buffer too small");
    }

    for(unsigned i=0;i<=slen;i++)
    {
        str[i] = dscr_str[2+i*2];
    }

    return std::string(str);
}

std::string pic::usb_serial(pic::usbdevice_t *device)
{
    unsigned char dscr_dev[18];
    device->control_in(0x80,6,0x0100,0,dscr_dev,18);
    return usb_string(device, dscr_dev[16]);
}

std::string pic::usb_product(pic::usbdevice_t *device)
{
    unsigned char dscr_dev[18];
    device->control_in(0x80,6,0x0100,0,dscr_dev,18);
    return usb_string(device, dscr_dev[15]);
}
