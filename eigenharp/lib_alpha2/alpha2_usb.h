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

#ifndef __ALPHA2_USB__
#define __ALPHA2_USB__

#include <picross/pic_stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BCTKBD_USBVENDOR_LEGACY             0xbeca
#define BCTKBD_USBVENDOR                    0x2139
#define BCTKBD_USBPRODUCT                   0x0104
#define BCTKBD_INTERFACE                    0

#define BCTKBD_MSGTYPE_NULL          1
#define BCTKBD_MSGTYPE_KEYDOWN       3
#define BCTKBD_MSGTYPE_RAW           4
#define BCTKBD_MSGTYPE_PROCESSED     5
#define BCTKBD_MSGTYPE_MIC           7
#define BCTKBD_MSGTYPE_PEDAL         8
#define BCTKBD_MSGTYPE_I2C           11
#define BCTKBD_MSGTYPE_MIDI          10

#define BCTKBD_HEADER1_SIZE          2
#define BCTKBD_HEADER2_SIZE          1
#define BCTKBD_HEADER3_SIZE          1
#define BCTKBD_PAYLOAD_KEYDOWN       9
#define BCTKBD_PAYLOAD_PROCESSED     4
#define BCTKBD_PAYLOAD_RAW           5
#define BCTKBD_PAYLOAD_MIC           24
#define BCTKBD_PAYLOAD_PEDAL         3
#define BCTKBD_MSGSIZE_KEYDOWN       (BCTKBD_HEADER1_SIZE+BCTKBD_PAYLOAD_KEYDOWN)
#define BCTKBD_MSGSIZE_PROCESSED     (BCTKBD_HEADER1_SIZE+BCTKBD_PAYLOAD_PROCESSED)
#define BCTKBD_MSGSIZE_RAW           (BCTKBD_HEADER1_SIZE+BCTKBD_PAYLOAD_RAW)
#define BCTKBD_MSGSIZE_MIC           (BCTKBD_HEADER2_SIZE+BCTKBD_PAYLOAD_MIC)
#define BCTKBD_MSGSIZE_PEDAL         (BCTKBD_HEADER2_SIZE+BCTKBD_PAYLOAD_PEDAL)

struct pik_msg1_t
{
    uint8_t  type;
    uint8_t  frame;
    uint16_t timestamp;
    uint16_t payload[0];
};

struct pik_msg2_t
{
    uint8_t  type;
    uint8_t  frame;
    uint16_t payload[0];
};

struct pik_msg3_t
{
    uint8_t  type;
    uint8_t  pad;
    uint16_t payload[0];
};

struct pik_msg4_t
{
    uint8_t  type;
    uint8_t  len;
    uint8_t  payload[0];
};

#define BCTKBD_MSG_RAW_KEY           0
#define BCTKBD_MSG_RAW_I0            1
#define BCTKBD_MSG_RAW_I1            2
#define BCTKBD_MSG_RAW_I2            3
#define BCTKBD_MSG_RAW_I3            4

#define BCTKBD_MSG_PROCESSED_KEY     0
#define BCTKBD_MSG_PROCESSED_P       1
#define BCTKBD_MSG_PROCESSED_R       2
#define BCTKBD_MSG_PROCESSED_Y       3

#define BCTKBD_CALTABLE_POINTS              30
#define BCTKBD_CALTABLE_SIZE                (2+BCTKBD_CALTABLE_POINTS)
#define BCTKBD_CALTABLE_MIN                 0
#define BCTKBD_CALTABLE_MAX                 1
#define BCTKBD_CALTABLE_DATA                2

#define BCTKBD_USBENDPOINT_ISO_IN_NAME       0x82
#define BCTKBD_BULKOUT_LED_EP                4
#define BCTKBD_USBENDPOINT_ISO_OUT_NAME      0x06
#define BCTP_USBENDPOINT_ISO_IN_NAME         0x88
#define BCTP_BULKOUT_MIDI_EP                 1

#define BCTKBD_USBENDPOINT_ISO_IN_SIZE       512
#define BCTKBD_BULKOUT_LED_EP_SIZE           512
#define BCTP_BULKOUT_MIDI_EP_SIZE            64
#define BCTP_USBENDPOINT_ISO_IN_SIZE         512
#define BCTKBD_USBENDPOINT_ISO_OUT_SIZE      512

#define BCTKBD_USBCOMMAND_START_REQTYPE      0x40
#define BCTKBD_USBCOMMAND_START_REQ          0xb1
#define BCTP_USBCOMMAND_START_REQ            0xc3
#define BCTP_USBCOMMAND_STOP_REQ             0xc4
#define BCTP_USBCOMMAND_START_MIDI_REQ       0xcc
#define BCTP_USBCOMMAND_STOP_MIDI_REQ        0xcd
#define BCTP_USBCOMMAND_START_I2C_REQ        0xce
#define BCTP_USBCOMMAND_STOP_I2C_REQ         0xcf

#define BCTKBD_USBCOMMAND_CALDATA_REQTYPE    0x40
#define BCTKBD_USBCOMMAND_CALDATA_REQ        0xb5

#define BCTKBD_USBCOMMAND_CALWRITE_REQTYPE   0x40
#define BCTKBD_USBCOMMAND_CALWRITE_REQ       0xb6

#define BCTKBD_USBCOMMAND_CALCLEAR_REQTYPE   0x40
#define BCTKBD_USBCOMMAND_CALCLEAR_REQ       0xb8

#define BCTKBD_USBCOMMAND_STOP_REQTYPE       0x40
#define BCTKBD_USBCOMMAND_STOP_REQ           0xbb

#define BCTKBD_USBCOMMAND_TEMP_REQTYPE       0x40
#define BCTKBD_USBCOMMAND_TEMP_REQ           0xc0

#define BCTKBD_USBCOMMAND_IREG_READ_REQTYPE  0x40
#define BCTKBD_USBCOMMAND_IREG_READ_REQ      0xc6

#define BCTKBD_USBCOMMAND_IREG_WRITE_REQTYPE 0x40
#define BCTKBD_USBCOMMAND_IREG_WRITE_REQ     0xc5

#define BCTKBD_USBCOMMAND_SETRAW_REQTYPE     0x40
#define BCTKBD_USBCOMMAND_SETRAW_REQ         0xb3

#define BCTKBD_USBCOMMAND_SETCOOKED_REQTYPE  0x40
#define BCTKBD_USBCOMMAND_SETCOOKED_REQ      0xb4

#define BCTKBD_TEST_MSGSIZE_LIB 4
#define BCTKBD_TEST_MSGTYPE_LIB 0x40

#define BCTKBD_TEST_MSGSIZE_SEQ 2
#define BCTKBD_TEST_MSGTYPE_SEQ 0x80

#define BCTKBD_TEST_MSGSIZE_START 2
#define BCTKBD_TEST_MSGTYPE_START 0xd0

#define BCTKBD_SETLED_MSGSIZE 6
#define BCTKBD_SETLED_MSGTYPE 0x15
#define BCTKBD_MIDI_MSGTYPE 0x1a

#define BCTKBD_AUDIO_PKT_FRAMES (8)      /* 8 frames of stereo */
#define BCTKBD_AUDIO_PKT_SIZE   (2+8*2*3) /* in bytes: 2 byte header, 8 frames, stereo, 24 bit */

#ifdef __cplusplus
}
#endif

#endif
