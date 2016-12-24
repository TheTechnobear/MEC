#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#include "push2lib.h"
#include "push1font.h"


namespace Push2API {

static uint16_t VID = 0x2982, PID = 0x1967;


#define VSCALE 5
#define HSCALE 2



// see : https://github.com/Ableton/push-interface/blob/master/doc/AbletonPush2MIDIDisplayInterface.asc#usb-display-interface-access
// //bbbbbggggggrrrrr
#define RGB565(r,g,b)   ((((((int16_t) b & 0x0078)>> 3)   << 5) | (((int16_t) g & 0x00FC) >> 2 ) << 6 ) | (((int16_t) r & 0x00f8) >> 3))   //use top bits of colour input
// #define RGB565(r,g,b) ((((((int16_t) b & 0x001F) << 5) | ((int16_t) g & 0x003F) ) << 6 ) | ((int16_t) r & 0x001f)) // uses lower bits of colour input
// #define RGB565(r,g,b) ((((((int16_t) r & 0x001F) << 5) | ((int16_t) g & 0x003F) ) << 6 ) | ((int16_t) b & 0x001f)) // do as RGB , idea was too then reverse, didnt seem right


#define F_HEIGHT    8
#define F_WIDTH     5
#define CH_COLS  (((LINE/2)  / F_WIDTH) / HSCALE)
#define CH_ROWS  ((HEIGHT/F_HEIGHT) / VSCALE)

// push 1
#define MONOCHROME true
#define MONO_CLR RGB565(255,60,0)

#define HDR_PKT_SZ 0x10
static uint8_t headerPkt[HDR_PKT_SZ] =
{ 0xef, 0xcd, 0xab, 0x89, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};// rev engineered
// { 0xFF, 0xCC, 0xAA, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00 }; //ableton


// Future versions of libusb will use usb_interface instead of interface
// in libusb_config_descriptor => catter for that
#define usb_interface interface



static int perr(char const *format, ...)
{
    va_list args;
    int r;

    va_start (args, format);
    r = vfprintf(stderr, format, args);
    va_end(args);

    return r;
}

#define ERR_EXIT(errcode) do { perr("   %s\n", libusb_strerror((enum libusb_error)errcode)); return -1; } while (0)
#define CALL_CHECK(fcall) do { r=fcall; if (r < 0) ERR_EXIT(r); } while (0);

Push2::Push2() : headerPkt_(headerPkt),handle_(NULL) {
    ;
}

Push2::~Push2() {
    ;
}


void Push2::clearDisplay() {
    memset(dataPkt_, 0, DATA_PKT_SZ);
}

void Push2::clearRow(unsigned row) {
    for (int line = 0; line < F_HEIGHT; line++) {
        for (int vs = 0; vs < VSCALE; vs++) {
            int bl = (((((row * F_HEIGHT) + line )) * VSCALE) + vs ) * (LINE / 2);
            memset(& (dataPkt_[bl]), 0, LINE);
        }
    }
}

void Push2::drawText(unsigned row, unsigned col, const char* str, int ln)
{
    unsigned len = col + ln < CH_COLS ? ln : CH_COLS - col;
    for (int i = 0; i < len; i++) {
        int c = col + i;
        int ch = str[i];
        for (int line = 0; line < F_HEIGHT; line++) {
            int prow  = ch / (GIMP_IMAGE_WIDTH / F_WIDTH) ;
            int pchar = ch %  (GIMP_IMAGE_WIDTH / F_WIDTH);
            int pline = ((prow * F_HEIGHT) + line) * (GIMP_IMAGE_WIDTH * GIMP_IMAGE_BYTES_PER_PIXEL);

            for (int vs = 0; vs < VSCALE; vs++) {
                int bl = (((((row * F_HEIGHT) + line )) * VSCALE) + vs ) * (LINE / 2);
                for (int pix = 0; pix < F_WIDTH ; pix++) {
                    int poffset = (pline) + (((pchar * F_WIDTH) + pix) * GIMP_IMAGE_BYTES_PER_PIXEL);

                    uint16_t clr = 0;
                    unsigned red = GIMP_IMAGE_PIXEL_DATA[poffset];
                    if (MONOCHROME) {
                        if (red) {
                            clr = MONO_CLR;
                        }

                    } else {
                        unsigned green = GIMP_IMAGE_PIXEL_DATA[poffset + 1];
                        unsigned blue = GIMP_IMAGE_PIXEL_DATA[poffset + 2];
                        clr = RGB565(red, blue, green);
                    }


                    for (int hs = 0; hs < HSCALE; hs++) {
                        int bpos = bl + ((((c * F_WIDTH) + pix) * HSCALE) + hs);
                        // dataPkt_[bpos] = ( (clr & 0x00FF) << 8 ) | ( (clr & 0xFF00) >> 8);
                        dataPkt_[bpos] = clr;
                    }
                }
            }
        }
    }
}

void Push2::drawText(unsigned row, unsigned col, const char* str)
{
    Push2::drawText(row, col, str, strlen(str));
}

unsigned CELL_OFFSET[4] = { 3, 27, 51, 75 };
void Push2::p1_drawCell(unsigned row, unsigned cell, const char* str)
{
    if (cell < 4) {
        Push2::drawText(row, CELL_OFFSET[cell], str);
    }
}

// void Push2::abletonShape() {
//     // official ableton 'shaping'
//     // xor with 0xFFE7F3E7
// }


int Push2::render() {
    if (handle_ == NULL) return -1;
    int tfrsize = 0;
    int r = 0;
    CALL_CHECK(libusb_bulk_transfer(handle_, endpointOut_, headerPkt_, HDR_PKT_SZ, &tfrsize, 1000));
    if (tfrsize != HDR_PKT_SZ) { printf( "header packet short %d", tfrsize); }
    CALL_CHECK(libusb_bulk_transfer(handle_, endpointOut_, (unsigned char*) dataPkt_, DATA_PKT_SZ, &tfrsize, 1000));
    if (tfrsize != DATA_PKT_SZ) { printf( "data packet short %d", tfrsize); }

    return 0;
}


int Push2::init() {
    const struct libusb_version* version;
    version = libusb_get_version();
    std::cout <<  "Using libusb " <<  version->major << "." << version->minor << "." <<  version->micro << "." <<  version->nano << std::endl;
    int r = libusb_init(NULL);
    libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_INFO);
    static uint16_t vid = VID, pid = PID;

    std::cout << "Open Push2 :" << std::hex << vid << ":" << pid << std::dec << std::endl;

    handle_ = libusb_open_device_with_vid_pid(NULL, vid, pid);

    if (handle_ == NULL) {
        perr("  Failed.\n");
        return -1;
    }

    CALL_CHECK(libusb_claim_interface(handle_, iface_));

    return r;
}

int Push2::deinit() {
    if (handle_ != NULL) {
        if (iface_ != 0) libusb_release_interface(handle_, iface_);
        libusb_close(handle_);
    }
    libusb_exit(NULL);
    return 0;
}

} // Push2API namespace
