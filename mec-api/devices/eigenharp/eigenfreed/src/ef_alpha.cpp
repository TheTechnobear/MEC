#include <eigenfreed/eigenfreed.h>

#include "eigenfreed_impl.h"

#include <picross/pic_config.h>


#include <lib_alpha2/alpha2_usb.h>

#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>

// INFO ONLY from alpha2_active.h
// #define KBD_KEYS 132
// #define KBD_DESENSE (KBD_KEYS+0)
// #define KBD_BREATH1 (KBD_KEYS+1)
// #define KBD_BREATH2 (KBD_KEYS+2)
// #define KBD_STRIP1  (KBD_KEYS+3)
// #define KBD_STRIP2  (KBD_KEYS+4)
// #define KBD_ACCEL   (KBD_KEYS+5)
// #define KBD_SENSORS 6


namespace EigenApi
{



   
void EF_Alpha::fireAlphaKeyEvent(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y)
{
    const int MAIN_KEYBASE = 120;
    unsigned course = key >= MAIN_KEYBASE;
    parent_.fireKeyEvent(t, course, key - (course * MAIN_KEYBASE), a, p, r, y);
}

void EF_Alpha::kbd_dead(unsigned reason)
{
	if(!parent_.stopping()) parent_.restartKeyboard();
}

void EF_Alpha::kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y)
{
    // pic::logmsg() << "kbd_key" << key << " p " << p << " r " << r << " y " << y;

     unsigned w = alpha2::active_t::key2word(key);
     unsigned short m = alpha2::active_t::key2mask(key);
     bool a = false;

     if(!(parent_.skpMap()[w]&m))
     {
         parent_.curMap()[w]|=m;
         a = true;
     }

    if(key < KBD_KEYS) {
        // pic::logmsg() << "kbd_key fire" << key << " p " << p << " r " << r << " y " << y;
        int rr = ( r - 2048) * 2;
        int ry = ( y - 2048 ) * 2;
        fireAlphaKeyEvent(t,key,a,p,rr,ry);
        return;
    }
    
    switch(key) {
        case KBD_BREATH1 : {
            parent_.fireBreathEvent(t, p);
            break;
        }
        case KBD_STRIP1 : {
            parent_.fireStripEvent(t, 1, p);
            break;
        }
        case KBD_STRIP2 : {
            parent_.fireStripEvent(t, 2, p);
            break;
        }
       // case KBD_DESENSE : break;
       // case KBD_BREATH2 : break; 
       // case KBD_ACCEL   : break;
        default: ;
    }
}

void EF_Alpha::kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4)
{
}

void EF_Alpha::kbd_keydown(unsigned long long t, const unsigned short *newmap)
{   
    // char buf[121];
    // for(int i=0;i < 120;i++) {
    //     buf[i] = alpha2::active_t::keydown(i,bitmap) ? '1' :  '0';
    // }
    // buf[120]=0;
    // pic::logmsg() << buf;
    for(unsigned w=0; w<9; w++)
    {
        parent_.skpMap()[w] &= newmap[w];

        if(parent_.curMap()[w] == newmap[w]) continue;

        unsigned keybase = alpha2::active_t::word2key(w);

        for(unsigned k=0; k<16; k++)
        {
            unsigned short mask = alpha2::active_t::key2mask(k);

            // if was on, but no longer on
            if( (parent_.curMap()[w]&mask) && !(newmap[w]&mask) )
            {
                fireAlphaKeyEvent(t,keybase+k,0,0,0,0);
            }
        }

        parent_.curMap()[w] = newmap[w];
    }    
}

void EF_Alpha::kbd_mic(unsigned char s,unsigned long long t, const float *data)
{
}

void EF_Alpha::midi_data(unsigned long long t, const unsigned char *data, unsigned len)
{
}

void EF_Alpha::pedal_down(unsigned long long t, unsigned pedal, unsigned value)
{
    parent_.firePedalEvent(t, pedal, value);
}
    
    
    
} // namespace EigenApi

