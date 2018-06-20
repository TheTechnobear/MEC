#include <iostream>
#include "push2lib.h"

#include <stdarg.h>
#include <memory.h>

namespace Push2API {

static uint16_t VID = 0x2982, PID = 0x1967;


#define HDR_PKT_SZ 0x10
static uint8_t headerPkt[HDR_PKT_SZ] = {0xFF, 0xCC, 0xAA, 0x88, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


// Future versions of libusb will use usb_interface instead of interface
// in libusb_config_descriptor => cater for that
#define usb_interface interface


static int perr(char const *format, ...) {
    va_list args;
    int r;

    va_start (args, format);
    r = vfprintf(stderr, format, args);
    va_end(args);

    return r;
}

#define ERR_EXIT(errcode) do { perr("   %s\n", libusb_strerror((enum libusb_error)errcode)); return -1; } while (0)
#define CALL_CHECK(fcall) do { r=fcall; if (r < 0) ERR_EXIT(r); } while (0);

#include <cairo.h>

cairo_surface_t *surface;
cairo_t *cr;
unsigned char imgBuf_[DATA_PKT_SZ];


Push2::Push2() : headerPkt_(headerPkt), handle_(NULL) {
    surface = cairo_image_surface_create_for_data(
            (unsigned char *) imgBuf_,
//            (unsigned char*) dataPkt_,
            CAIRO_FORMAT_RGB16_565,
            WIDTH,
            HEIGHT,
            LINE
    );
    cr = cairo_create(surface);
    memset(dataPkt_, 0, DATA_PKT_SZ);
    cairo_select_font_face(cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16.0);
    cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
}

Push2::~Push2() {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);;
}

void Push2::clearDisplay() {
    memset(imgBuf_, 0, DATA_PKT_SZ);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);
//    drawCell8(0,5,"Hello",Push2::Colour(0,0,0xFF));
//    drawCell8(1,5,"Hello",Push2::Colour(0,0,0xFF));
//    drawInvertedCell8(2,5,"Hello",Push2::Colour(0,0,0xFF));
//    drawCell8(3,5,"Hello",Push2::Colour(0,0,0xFF));
//    drawInvertedCell8(3,6,"Hello",Push2::Colour(0xFF,0,0));
//    drawCell8(4,5,"Hello",Push2::Colour(0,0,0xFF));
//    drawCell8(5,5,"Hello",Push2::Colour(0,0,0xFF));
}


void Push2::drawInvertedCell8(unsigned row, unsigned cell, const char *str, const Push2::Colour &clr) {
    cairo_set_source_rgb(cr, clr.blue_, clr.green_, clr.red_); //!!
    cairo_rectangle(cr,
                    (cell * (WIDTH / 8) + 9), (row * 24) + 9,
                    (WIDTH / 8), 24);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0); //!!
    cairo_move_to(cr, (cell * (WIDTH / 8) + 16), row * 24 + 24);
    cairo_show_text(cr, str);
}

void Push2::drawCell8(unsigned row, unsigned cell, const char *str, const Push2::Colour &clr) {
    cairo_set_source_rgb(cr, 0, 0, 0); //!!
    cairo_rectangle(cr,
                    (cell * (WIDTH / 8) + 9), (row * 24) + 9,
                    (WIDTH / 8), 24);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, clr.blue_, clr.green_, clr.red_); //!!
    cairo_move_to(cr, (cell * (WIDTH / 8) + 16), row * 24 + 24);
    cairo_show_text(cr, str);
}


int Push2::render() {
    if (handle_ == NULL) return -1;
    int tfrsize = 0;
    int r = 0;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < LINE; x += 4) {
            int pixelOffset = (y * LINE) + x;
            int destinationOffset = (y * LINE) + x;
            // mask 0xFFE7F3E7
            dataPkt_[destinationOffset] = imgBuf_[pixelOffset] ^ 0xE7;
            dataPkt_[destinationOffset + 1] = imgBuf_[pixelOffset + 1] ^ 0xF3;
            dataPkt_[destinationOffset + 2] = imgBuf_[pixelOffset + 2] ^ 0xE7;
            dataPkt_[destinationOffset + 3] = imgBuf_[pixelOffset + 3] ^ 0xFF;
        }
    }

    CALL_CHECK(libusb_bulk_transfer(handle_, endpointOut_, headerPkt_, HDR_PKT_SZ, &tfrsize, 1000));
    if (tfrsize != HDR_PKT_SZ) { printf("header packet short %d", tfrsize); }
    CALL_CHECK(libusb_bulk_transfer(handle_, endpointOut_, (unsigned char *) dataPkt_, DATA_PKT_SZ, &tfrsize, 1000));
    if (tfrsize != DATA_PKT_SZ) { printf("data packet short %d", tfrsize); }

    return 0;
}


int Push2::init() {
    const struct libusb_version *version;
    version = libusb_get_version();
    std::cout << "Using libusb " << version->major << "." << version->minor << "." << version->micro << "."
              << version->nano << std::endl;
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
