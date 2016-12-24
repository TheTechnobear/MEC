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

#ifndef __PICO_USB__
#define __PICO_USB__

#ifdef __cplusplus
extern "C" {
#endif

#define BCTPICO_USBVENDOR_LEGACY             0xbeca
#define BCTPICO_USBVENDOR                    0x2139
#define BCTPICO_USBPRODUCT                   0x0101
#define BCTPICO_INTERFACE                    0
#define BCTPICO_SCANSIZE                     192

#define BCTPICO_USBENDPOINT_SENSOR_NAME      0x82
#define BCTPICO_USBENDPOINT_SENSOR_SIZE      (4*BCTPICO_SCANSIZE)

#define BCTPICO_CALTABLE_POINTS              30
#define BCTPICO_CALTABLE_SIZE                (2+BCTPICO_CALTABLE_POINTS)
#define BCTPICO_CALTABLE_MIN                 0
#define BCTPICO_CALTABLE_MAX                 1
#define BCTPICO_CALTABLE_DATA                2

#define TYPE_VENDOR   0x40

#define BCTPICO_USBCOMMAND_SETLED   0xb0
#define BCTPICO_USBCOMMAND_SETMODELED 0xb2
#define BCTPICO_USBCOMMAND_START    0xb1
#define BCTPICO_USBCOMMAND_CALDATA  0xb5
#define BCTPICO_USBCOMMAND_CALWRITE 0xb6
#define BCTPICO_USBCOMMAND_CALREAD  0xb7
#define BCTPICO_USBCOMMAND_CALCLEAR 0xb8
#define BCTPICO_USBCOMMAND_STOP     0xbb
#define BCTPICO_USBCOMMAND_DEBUG    0xc0

#ifdef __cplusplus
}
#endif

#endif
