#ifndef PUSH2LIB_H
#define PUSH2LIB_H

#include <stdint.h>
#include <libusb.h>

namespace Push2API {

#define DATA_PKT_SZ (LINE * HEIGHT)
#define LINE    2048
#define HEIGHT  160
#define WIDTH   960

#define MONO_CLR RGB565(255,60,0)

#define RGB565(r, g, b)   ((((((int16_t) b & 0x0078)>> 3)   << 5) | (((int16_t) g & 0x00FC) >> 2 ) << 6 ) | (((int16_t) r & 0x00f8) >> 3))   //use top bits of colour input

class Push2 {
public:
    Push2();
    virtual ~Push2();
    int init();
    int render();
    int deinit();


    void clearDisplay();
    void clearRow(unsigned row, unsigned vscale);

    static const unsigned P1_VSCALE = 5;
    static const unsigned P1_HSCALE = 2;
    void drawText(unsigned row, unsigned col, const char *str, int ln, unsigned vscale, unsigned hscale, int16_t clr);
    void drawText(unsigned row, unsigned col, const char *str, unsigned vscale, unsigned hscale, int16_t clr);
    void drawCell8(unsigned row, unsigned cell, const char *str, unsigned vscale, unsigned hscale, int16_t clr);

    void p1_drawCell8(unsigned row, unsigned cell, const char *str);
    void p1_drawCell4(unsigned row, unsigned cell, const char *str);


private:
    uint8_t *headerPkt_;
    uint16_t dataPkt_[DATA_PKT_SZ / 2];

    libusb_device_handle *handle_;
    int iface_ = 0;
    int endpointOut_ = 1;
};

}

#endif //PUSH2LIB_H