#ifndef PUSH2LIB_H
#define PUSH2LIB_H

#include <stdint.h>
#include <libusb.h>

namespace Push2API {

#define DATA_PKT_SZ (LINE * HEIGHT)
#define LINE    2048
#define HEIGHT  160
#define WIDTH   960

class Push2 {
public:
    Push2();
    virtual ~Push2();
    int init();
    int render();
    int deinit();

    void drawText(unsigned row, unsigned col, const char* str, int ln);
    void drawText(unsigned row, unsigned col, const char* str);
    void p1_drawCell(unsigned row, unsigned cell, const char* str);
    void clearDisplay();
    void clearRow(unsigned row);


private:
    uint8_t  *headerPkt_;
    uint16_t dataPkt_[DATA_PKT_SZ / 2];

    libusb_device_handle *handle_;
    int iface_ = 0;
    int endpointOut_ = 1;
};

}

#endif //PUSH2LIB_H