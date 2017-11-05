#include "mec_push2.h"

#include "mec_log.h"
#include "../mec_voice.h"
#include "mec_push2_param.h"


////////////////////////////////////////////////

#define PAD_NOTE_ON_CLR (int8_t) 127
#define PAD_NOTE_OFF_CLR (int8_t) 0
#define PAD_NOTE_ROOT_CLR (int8_t) 41
#define PAD_NOTE_IN_KEY_CLR (int8_t) 3

enum PushModes {
    P2_Param
};


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

Push2::Push2(ICallback &cb) :
        MidiDevice(cb),
        scaleIdx_(1),
        octave_(2),
        chromatic_(true),
        scale_(scales[scaleIdx_].value),
        numNotesInScale_(0),
        tonic_(0),
        rowOffset_(5) {
    numNotesInScale_ = NumNotesInScale(scale_);
    PaUtil_InitializeRingBuffer(&midiQueue_, sizeof(MidiMsg), MAX_N_MIDI_MSGS, msgData_);
}

Push2::~Push2() {
    deinit();
}

void Push2InCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
    Push2 *self = static_cast<Push2 *>(userData);
    self->midiCallback(deltatime, message);
}

void Push2::changeMode(unsigned mode) {
    currentMode_ = mode;
    auto m = modes_[mode];
    if (m != nullptr) m->activate();
}

void Push2::addMode(unsigned mode, std::shared_ptr<P2_Mode> handler) {
    modes_[mode] = handler;
}


RtMidiIn::RtMidiCallback Push2::getMidiCallback() {

    return Push2InCallback;
}


bool Push2::init(void *arg) {
    if (MidiDevice::init(arg)) {

        Preferences prefs(arg);

        // push2 api setup
        push2Api_.reset(new Push2API::Push2());
        push2Api_->init();

        push2Api_->clearDisplay();

        // Kontrol setup
        model_ = Kontrol::KontrolModel::model();
        addMode(P2_Param, std::make_shared<P2_ParamMode>(*this, push2Api_));
        changeMode(P2_Param);

        active_ = true;
        updatePadColours();
        LOG_0("Push2::init - complete");

        return active_;
    }
    return false;
}

bool Push2::process() {
    push2Api_->render(); // TODO maybe this should be moved off to separate slow thread

    while (PaUtil_GetRingBufferReadAvailable(&midiQueue_)) {
        MidiMsg msg;
        PaUtil_ReadRingBuffer(&midiQueue_, &msg, 1);
        processMidi(msg);
    }

    return MidiDevice::process();
}

void Push2::deinit() {
    LOG_0("Push2::deinit");

    if (push2Api_)push2Api_->deinit();
    push2Api_.reset();
    MidiDevice::deinit();
}

bool Push2::midiCallback(double, std::vector<unsigned char> *message) {
    unsigned int n = message->size();
    if (n > 3) LOG_0("midiCallback unexpect midi size" << n);

    MidiMsg m;

    m.data[0] = message->at(0);
    if (n > 1) m.data[1] = message->at(1);
    if (n > 2) m.data[2] = message->at(2);

    // LOG_0("midi: s " << std::hex << m.status_ << " "<< m.data[1] << " " << m.data[2]);
    PaUtil_WriteRingBuffer(&midiQueue_, (void *) &m, 1);
    return true;
}

bool Push2::processMidi(const MidiMsg &midimsg) {
    //TODO... this can be rationalise with mec_mididevice.cpp
    //be careful : push2 we want to process midi message in main thread
    //since we are going to be handling some of the messages for OLED etc
    int ch = midimsg.data[0] & 0x0F;
    int type = midimsg.data[0] & 0xF0;
    MecMsg msg;
    switch (type) {
        case 0x90: { // note on
            if (midimsg.data[1] >= P2_NOTE_PAD_START && midimsg.data[1] <= P2_NOTE_PAD_END) {
                // TODO: use voice, or make single ch midi
                if (midimsg.data[2] > 0)
                    msg.type_ = MecMsg::TOUCH_ON;
                else
                    msg.type_ = MecMsg::TOUCH_OFF;

                msg.data_.touch_.touchId_ = ch;
                msg.data_.touch_.note_ = (float) midimsg.data[1];
                msg.data_.touch_.x_ = 0;
                msg.data_.touch_.y_ = 0;
                msg.data_.touch_.z_ = float(midimsg.data[2]) / 127.0f;
                queue_.addToQueue(msg);
            } else {
                if (midimsg.data[2] > 0) {
                    processNoteOn(midimsg.data[1], midimsg.data[2]);
                } else {
                    processNoteOff(midimsg.data[1], 100);
                }
            }
            break;
        }

        case 0x80: { // note off
            if (midimsg.data[1] >= P2_NOTE_PAD_START && midimsg.data[1] <= P2_NOTE_PAD_END) {
                // TODO: use voice, or make single ch midi
                msg.type_ = MecMsg::TOUCH_OFF;
                msg.data_.touch_.touchId_ = ch;
                msg.data_.touch_.note_ = (float) midimsg.data[1];
                msg.data_.touch_.x_ = 0;
                msg.data_.touch_.y_ = 0;
                msg.data_.touch_.z_ = float(midimsg.data[2]) / 127.0f;
                queue_.addToQueue(msg);
            } else {
                processNoteOff(midimsg.data[1], midimsg.data[2]);
            }
            break;
        }
        case 0xA0: { // poly pressure
            // TODO: use voice, or make single ch midi
            msg.type_ = MecMsg::TOUCH_CONTINUE;
            msg.data_.touch_.touchId_ = ch;
            msg.data_.touch_.note_ = (float) midimsg.data[1];
            msg.data_.touch_.x_ = 0;
            msg.data_.touch_.y_ = 0;
            msg.data_.touch_.z_ = float(midimsg.data[2]) / 127.0f;
            queue_.addToQueue(msg);
        }
        case 0xB0: { // CC
            processCC(midimsg.data[1], midimsg.data[2]);
            break;
        }
        case 0xD0: { // channel pressure
            // float v = float(midimsg.data[1]) / 127.0f;
            // msg.type_ = MecMsg::CONTROL;
            // msg.data_.control_.controlId_ = type;
            // msg.data_.control_.value_ = v;
            // queue_.addToQueue(msg);
            break;
        }
        case 0xE0: { // pitchbend
            // float pb = (float) ((midimsg.data[2] << 7) + midimsg.data[1]);
            // float v = (pb / 8192.0f) - 1.0f;  // -1.0 to 1.0
            // msg.type_ = MecMsg::CONTROL;
            // msg.data_.control_.controlId_ = type;
            // msg.data_.control_.value_ = v;
            // queue_.addToQueue(msg);
            break;
        }
        default: {
            // TODO
            // everything else;
            // need to consider sysex replies?
            break;
        }
    } //switch

    return true;
}


void Push2::processNoteOn(unsigned n, unsigned v) {
    currentMode()->processNoteOn(n, v);

}

void Push2::processNoteOff(unsigned n, unsigned v) {
    currentMode()->processNoteOff(n, v);
}


void Push2::processCC(unsigned cc, unsigned v) {
    currentMode()->processCC(cc, v);
}


void Push2::updatePadColours() {
    for (int8_t r = 0; r < 8; r++) {
        for (int8_t c = 0; c < 8; c++) {
            unsigned clr = determinePadScaleColour(r, c);
            sendNoteOn(0, P2_NOTE_PAD_START + (r * 8) + c, clr);
        }
    }
}

unsigned Push2::determinePadScaleColour(int8_t r, int8_t c) {
    auto note_s = (r * rowOffset_) + c;
    if (chromatic_) {
        int8_t i = note_s % (int8_t) 12;
        int8_t v = (int8_t) (scale_ & ((int8_t) 1 << ((int8_t) 11 - i)));

        return i == 0 ? PAD_NOTE_ROOT_CLR : (v > 0 ? PAD_NOTE_IN_KEY_CLR : PAD_NOTE_OFF_CLR);
    } else {
        return (note_s % numNotesInScale_) == 0 ? PAD_NOTE_ROOT_CLR : PAD_NOTE_IN_KEY_CLR;
    }
}

} // namespace



