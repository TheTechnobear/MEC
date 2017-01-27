
/*
 Copyright 2009 Eigenlabs Ltd.  http://www.eigenlabs.com
*/

#ifndef __PICO_DECODER__
#define __PICO_DECODER__

#ifdef _WIN32
  #ifdef BUILDING_PICO_DECODER
    #define PICO_DECODER_DECLSPEC_FUNC(rt) rt __declspec(dllexport)
    #define PICO_DECODER_DECLSPEC_CLASS __declspec(dllexport)
  #else
    #define PICO_DECODER_DECLSPEC_FUNC(rt) rt __declspec(dllimport)
    #define PICO_DECODER_DECLSPEC_CLASS __declspec(dllimport)
  #endif
#else
  #if __GNUC__ >= 4
    #ifdef BUILDING_PICO_DECODER
      #define PICO_DECODER_DECLSPEC_FUNC(rt) rt __attribute__ ((visibility("default")))
      #define PICO_DECODER_DECLSPEC_CLASS __attribute__ ((visibility("default")))
    #else
      #define PICO_DECODER_DECLSPEC_FUNC(rt) rt  __attribute__ ((visibility("default")))
      #define PICO_DECODER_DECLSPEC_CLASS  __attribute__ ((visibility("default")))
    #endif
  #else
    #define PICO_DECODER_DECLSPEC_FUNC(rt) rt
    #define PICO_DECODER_DECLSPEC_CLASS
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PICO_DECODER_SIZE 16384

typedef struct _pico_decoder
{
    unsigned char data[PICO_DECODER_SIZE];
} pico_decoder_t;

typedef struct _pico_rawkey
{
    unsigned short c[4];
    unsigned long long time;
} pico_rawkey_t;

typedef struct _pico_rawkbd
{
    pico_rawkey_t keys[22];
    unsigned short breath[16];
    unsigned short strip;
    unsigned short buttons[4];
} pico_rawkbd_t;

PICO_DECODER_DECLSPEC_FUNC(void) pico_decoder_create(pico_decoder_t *decoder, int tp);
PICO_DECODER_DECLSPEC_FUNC(void) pico_decoder_destroy(pico_decoder_t *decoder);
PICO_DECODER_DECLSPEC_FUNC(void) pico_decoder_raw(pico_decoder_t *decoder, int resync, const unsigned char *usb, unsigned usblen, unsigned long long time, void (*raw)(void *ctx, int resync, const pico_rawkbd_t *), void *ctx);
PICO_DECODER_DECLSPEC_FUNC(void) pico_decoder_cooked(pico_decoder_t *decoder, int resync, const unsigned char *usb, unsigned usblen, unsigned long long time, void (*cooked)(void *ctx, unsigned long long ts, int tp, int id, unsigned a, unsigned p, int r, int y), void *ctx);
PICO_DECODER_DECLSPEC_FUNC(void) pico_decoder_cal(pico_decoder_t *decoder, unsigned k,unsigned c, unsigned short min, unsigned short max, unsigned size, unsigned short *points);

#define PICO_DECODER_KEY 0
#define PICO_DECODER_BREATH 1
#define PICO_DECODER_STRIP 2
#define PICO_DECODER_MODE 3

#define PICO_DECODER_PICO 0
#define PICO_DECODER_MICRO 1

#ifdef __cplusplus
}
#endif

#endif
