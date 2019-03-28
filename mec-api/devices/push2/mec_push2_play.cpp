#include "mec_push2_play.h"

#include <mec_log.h>
#include <mec_msg_queue.h>

#define PAD_NOTE_ON_CLR (int8_t) 127
#define PAD_NOTE_OFF_CLR (int8_t) 0
#define PAD_NOTE_ROOT_CLR (int8_t) 41
#define PAD_NOTE_IN_KEY_CLR (int8_t) 3

namespace mec {
static const struct Scales {
    char name[20];
    uint16_t value;
} scales[] = {
        {"Chromatic",      0b111111111111},
        {"Major",          0b101011010101},
        {"Harmonic Minor", 0b101101011001},
        {"Melodic Minor",  0b101101010101},
        {"Whole",          0b101010101010},
        {"Octatonic",      0b101101101101}
};

#if 0
const int scales_len = sizeof(scales) / sizeof(scales[0]);

static const char tonics[12][3] = {
        "C ",
        "C#",
        "D",
        "D#",
        "E ",
        "F ",
        "F#",
        "G",
        "G#",
        "A ",
        "A#",
        "B"
};

static const char scaleModes[2][15] = {
        "Chromatic",
        "In Key"
};

#endif

uint8_t NumNotesInScale(int16_t scale) {
    int s = scale;
    uint8_t n = 0;
    for (int i = 0; i < 12; i++) {
        if (s & 0x01) {
            n++;
        }
        s = s >> 1;
    }
    return n;
}

P2_PlayMode::P2_PlayMode(mec::Push2 &parent, const std::shared_ptr<Push2API::Push2> &api)
        : parent_(parent),
          push2Api_(api),
          scaleIdx_(1),
          octave_(5),
          chromatic_(true),
          scale_(scales[scaleIdx_].value),
          numNotesInScale_(0),
          tonic_(0),
          rowOffset_(5) {
    numNotesInScale_ = NumNotesInScale(scale_);
}

void P2_PlayMode::activate() {
    updatePadColours();
}

void P2_PlayMode::updatePadColours() {
    for (int8_t r = 0; r < 8; r++) {
        for (int8_t c = 0; c < 8; c++) {
            unsigned clr = determinePadScaleColour(r, c);
            parent_.sendNoteOn(0, P2_NOTE_PAD_START + (r * 8) + c, clr);
        }
    }
}
unsigned P2_PlayMode::determinePadNote(int8_t r, int8_t c) {
    //TODO : extend to using scales, and octaves

    // octave midi starts at -2, so 5 = C3
    unsigned note;
    if(chromatic_) {
        note = (octave_ * 12)  + (r * rowOffset_) + c + tonic_;
    } else {
        // needs testing
        note = (octave_ * 12)  + (r * rowOffset_) + ((c / numNotesInScale_) * 12 ) + tonic_;
        int nc = c;
        int mask = 0b100000000000;
        while(nc>0) {
            if(scale_& mask) {
                nc--;
            }
            note++;
        }

    }
    return note;
}

unsigned P2_PlayMode::determinePadScaleColour(int8_t r, int8_t c) {
    auto note_s = (r * rowOffset_) + c;
    if (chromatic_) {
        int i = note_s %  12;
        int v = (scale_ & (1 << ( 11 - i)));

        return i == 0 ? PAD_NOTE_ROOT_CLR : (v > 0 ? PAD_NOTE_IN_KEY_CLR : PAD_NOTE_OFF_CLR);
    } else {
        return (note_s % numNotesInScale_) == 0 ? PAD_NOTE_ROOT_CLR : PAD_NOTE_IN_KEY_CLR;
    }
}


void P2_PlayMode::processNoteOn(unsigned n, unsigned v) {
    if (n >= P2_NOTE_PAD_START && n <= P2_NOTE_PAD_END) {
        // TODO: use voice, or make single ch midi
        int padn = n - P2_NOTE_PAD_START;
        int r = padn / 8;
        int c = padn % 8;

        MecMsg msg;
        msg.type_ = MecMsg::TOUCH_ON;
        msg.data_.touch_.touchId_ = 1;
        msg.data_.touch_.note_ = determinePadNote(r,c);
        msg.data_.touch_.x_ = 0;
        msg.data_.touch_.y_ = 0;
        msg.data_.touch_.z_ = float(v) / 127.0f;
        parent_.queueMecMsg(msg);

        parent_.sendNoteOn(0, n, PAD_NOTE_ON_CLR);
    }
}

void P2_PlayMode::processNoteOff(unsigned n, unsigned v) {
    if (n >= P2_NOTE_PAD_START && n <= P2_NOTE_PAD_END) {
        // TODO: use voice, or make single ch midi
        int padn = n - P2_NOTE_PAD_START;
        int r = padn / 8;
        int c = padn % 8;

        MecMsg msg;
        msg.type_ = MecMsg::TOUCH_OFF;
        msg.data_.touch_.touchId_ = 1;
        msg.data_.touch_.note_ = determinePadNote(r,c);
        msg.data_.touch_.x_ = 0;
        msg.data_.touch_.y_ = 0;
        msg.data_.touch_.z_ = float(v) / 127.0f;
        parent_.queueMecMsg(msg);


        unsigned clr = determinePadScaleColour(r, c);
        parent_.sendNoteOn(0, n, clr);

    }
}

void P2_PlayMode::processCC(unsigned n, unsigned v) {
    if(n == P2_ACCENT_CC) {
        parent_.sendCC(0, P2_ACCENT_CC,v > 0 ? 0x7f : 0x10);
        MecMsg msg;
        msg.type_ = MecMsg::CONTROL;
        msg.data_.control_.controlId_ = 69; // default AUX CC
        msg.data_.control_.value_ = float(v) / 127.0f;
        parent_.queueMecMsg(msg);
    }
}

}
