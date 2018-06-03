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
    struct Colour {
        float red_;
        float green_;
        float blue_;

        Colour(unsigned r, unsigned g, unsigned b) {
            red_ = r / 256.0f;
            green_ = g / 256.0f;
            blue_ = b / 256.0f;
        }
        Colour(const Colour& c) : red_(c.red_), green_(c.green_), blue_(c.blue_){
            ;
        }
    };


    Push2();
    virtual ~Push2();
    int init();
    int render();
    int deinit();


    void clearDisplay();
    void drawCell8(unsigned row, unsigned cell, const char *str, const Colour& clr);
    void drawInvertedCell8(unsigned row, unsigned cell, const char *str, const Colour& clr);



private:
    uint8_t *headerPkt_;
    unsigned char dataPkt_[DATA_PKT_SZ];

    libusb_device_handle *handle_;
    int iface_ = 0;
    int endpointOut_ = 1;
};

}

#endif //PUSH2LIB_H