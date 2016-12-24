#include "pico_decoder.h"
// NULL implementaton!

void pico_decoder_create(pico_decoder_t *decoder, int tp)
{
    ;
}
void pico_decoder_destroy(pico_decoder_t *decoder)
{
    ;
}
void pico_decoder_raw(pico_decoder_t *decoder, int resync, const unsigned char *usb, unsigned usblen, unsigned long long time, void (*raw)(void *ctx, int resync, const pico_rawkbd_t *), void *ctx)
{

}
void pico_decoder_cooked(pico_decoder_t *decoder, int resync, const unsigned char *usb, unsigned usblen, unsigned long long time, void (*cooked)(void *ctx, unsigned long long ts, int tp, int id, unsigned a, unsigned p, int r, int y), void *ctx)
{

}

void pico_decoder_cal(pico_decoder_t *decoder, unsigned k,unsigned c, unsigned short min, unsigned short max, unsigned size, unsigned short *points)
{

}