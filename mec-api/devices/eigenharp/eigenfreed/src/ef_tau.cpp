#include <eigenfreed/eigenfreed.h>

#include "eigenfreed_impl.h"

#include <picross/pic_config.h>


#include <lib_alpha2/alpha2_usb.h>

#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>

// INFO ONLY from alpha2_active.h
// #define TAU_KBD_KEYS 84 // normal keys(84)
// #define TAU_KEYS_OFFSET 5
// #define TAU_MODE_KEYS   8
// #define TAU_KBD_DESENSE (TAU_KBD_KEYS+0)
// #define TAU_KBD_BREATH1 (TAU_KBD_KEYS+1)
// #define TAU_KBD_BREATH2 (TAU_KBD_KEYS+2)
// #define TAU_KBD_STRIP1  (TAU_KBD_KEYS+3)
// #define TAU_KBD_ACCEL   (TAU_KBD_KEYS+4)


namespace EigenApi
{

static const unsigned int TAU_COLUMNCOUNT = 7;
static const unsigned int TAU_COLUMNS[TAU_COLUMNCOUNT] = {16, 16, 20, 20, 12, 4, 4};
static const unsigned int TAU_COURSEKEYS = 92;


void EF_Tau::fireTauKeyEvent(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y)
{
    const int MAIN_KEYBASE = TAU_KBD_KEYS; //CHECK
    unsigned course = key >= MAIN_KEYBASE;
    if (key > TAU_KBD_KEYS) key = key - TAU_KEYS_OFFSET; // mode keys
    parent_.fireKeyEvent(t, course, key , a, p, r, y);
}

void EF_Tau::kbd_dead(unsigned reason)
{
    if (!parent_.stopping()) parent_.restartKeyboard();
}

void EF_Tau::kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y)
{
    // pic::logmsg() << "kbd_key" << key << " p " << p << " r " << r << " y " << y;

    unsigned w = alpha2::active_t::key2word(key);
    unsigned short m = alpha2::active_t::key2mask(key);
    bool a = false;

    if (!(parent_.skpMap()[w]&m)) {
        parent_.curMap()[w] |= m;
        a = true;
    }

    if (key < TAU_KBD_KEYS || key >= (TAU_KBD_KEYS + TAU_KEYS_OFFSET)) {
    // pic::logmsg() << "kbd_key fire" << key << " p " << p << " r " << r << " y " << y;
        int rr = ( r - 2048) * 2;
        int ry = ( y - 2048 ) * 2;
        if (key >= (TAU_KBD_KEYS + 5)) {
            // mode key
            p *= 4095;
        }

        fireTauKeyEvent(t, key, a, p, rr, ry);
        return;
    }

    //TAU_KEYS_OFFSET = 5
    //TAU_MODE_KEYS = 8

    switch (key) {
    
    case TAU_KBD_BREATH1 : {
        parent_.fireBreathEvent(t, p);
        break;
    }
    case TAU_KBD_STRIP1 : {
        parent_.fireStripEvent(t, 1, p);
        break;
    }
    // case TAU_KBD_DESENSE : break;
    // case TAU_KBD_BREATH2 : break;
    // case TAU_KBD_ACCEL   : break;
    default: ;
    }
}

void EF_Tau::kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4)
{
}

void EF_Tau::kbd_keydown(unsigned long long t, const unsigned short *newmap)
{
    // char buf[121];
    // for(int i=0;i < 120;i++) {
    //     buf[i] = alpha2::active_t::keydown(i,bitmap) ? '1' :  '0';
    // }
    // buf[120]=0;
    // pic::logmsg() << buf;
    for (unsigned w = 0; w < 9; w++)
    {
        parent_.skpMap()[w] &= newmap[w];

        if (parent_.curMap()[w] == newmap[w]) continue;

        unsigned keybase = alpha2::active_t::word2key(w);

        for (unsigned k = 0; k < 16; k++)
        {
            unsigned short mask = alpha2::active_t::key2mask(k);

            // if was on, but no longer on
            if ( (parent_.curMap()[w]&mask) && !(newmap[w]&mask) )
            {
                fireTauKeyEvent(t, keybase + k, 0, 0, 0, 0);
            }
        }

        parent_.curMap()[w] = newmap[w];
    }
}

void EF_Tau::kbd_mic(unsigned char s, unsigned long long t, const float *data)
{
}

void EF_Tau::midi_data(unsigned long long t, const unsigned char *data, unsigned len)
{
}

void EF_Tau::pedal_down(unsigned long long t, unsigned pedal, unsigned value)
{
    parent_.firePedalEvent(t, pedal, value);
}



} // namespace EigenApi

