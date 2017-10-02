#include "mec_push2.h"

#include "../mec_log.h"
#include "../mec_prefs.h"
#include "../mec_voice.h"

////////////////////////////////////////////////
// TODO
// this is skeleton only , patching thru midi, and test screen
// in the future :
// - display will show MEC parameters, and allow altering (needs properties)
// - midi will be interpreted for buttons, so we can navigate params, changes scales etc
// - surface/scale mapping for pads.


// see https://github.com/Ableton/push-interface/blob/master/doc/AbletonPush2MIDIDisplayInterface.asc#command-list


////////////////////////////////////////////////

namespace mec {

const unsigned P2_NOTE_PAD_START = 36;
const unsigned P2_NOTE_PAD_END = P2_NOTE_PAD_START + 63;

const unsigned P2_ENCODER_CC_TEMPO = 14;
const unsigned P2_ENCODER_CC_SWING = 15;
const unsigned P2_ENCODER_CC_START = 71;
const unsigned P2_ENCODER_CC_END = P2_ENCODER_CC_START + 7;
const unsigned P2_ENCODER_CC_VOLUME = 79;

const unsigned P2_DEV_SELECT_CC_START = 102;
const unsigned P2_DEV_SELECT_CC_END = P2_DEV_SELECT_CC_START + 7 ;

const unsigned P2_TRACK_SELECT_CC_START = 20;
const unsigned P2_TRACK_SELECT_CC_END = P2_DEV_SELECT_CC_START + 7 ;

static const struct Scales {
    char name[20];
    int16_t value;
} scales [] = {
    {"Chromatic",        0b111111111111 },
    {"Major",            0b101011010101 },
    {"Harmonic Minor",   0b101101011001 },
    {"Melodic Minor",    0b101101010101 },
    {"Whole",            0b101010101010 },
    {"Octatonic",        0b101101101101 }
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
static const char scaleModes [2][15] = {
    "Chromatic",
    "In Key"
};





class Push2_OLED : public oKontrol::ParameterCallback {
public:
    Push2_OLED(Push2& parent, const std::shared_ptr<Push2API::Push2>& api) :
        parent_(parent), push2Api_(api), currentPage_(0) {
        param_model_ = oKontrol::ParameterModel::model();
        ;
    }
    void setCurrentPage(unsigned page);
    // ParameterCallback
    virtual void addClient(const std::string&, unsigned );
    virtual void page(oKontrol::ParameterSource , const oKontrol::Page& );
    virtual void param(oKontrol::ParameterSource, const oKontrol::Parameter&);
    virtual void changed(oKontrol::ParameterSource src, const oKontrol::Parameter& p);

private:
    void displayPage(const std::string& id);
    void drawParam(unsigned pos, const oKontrol::Parameter& param);
    Push2& parent_;
    std::shared_ptr<Push2API::Push2> push2Api_;
    std::shared_ptr<oKontrol::ParameterModel> param_model_;
    unsigned currentPage_;
};

int NumNotesInScale(int16_t scale) {
    int s = scale;
    int n = 0;
    for (int i = 0; i < 12; i++) {
        if ( s & 0x01) {
            n++;
        }
        s = s >> 1;
    }
    return n;
}

Push2::Push2(ICallback& cb) :
    MidiDevice(cb),
    scaleIdx_(1),
    octave_(2),
    chromatic_(true),
    scale_(scales[scaleIdx_].value),
    numNotesInScale_(0),
    tonic_(0),
    rowOffset_(5) {
    numNotesInScale_ = NumNotesInScale(scale_);
    PaUtil_InitializeRingBuffer(& midiQueue_, sizeof(MidiMsg), MAX_N_MIDI_MSGS, msgData_);
}

Push2::~Push2() {
    deinit();
}

void Push2InCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    Push2* self = static_cast<Push2*>(userData);
    self->midiCallback(deltatime, message);
}

RtMidiIn::RtMidiCallback Push2::getMidiCallback() {

    return Push2InCallback;
}


bool Push2::init(void* arg) {
    if (MidiDevice::init(arg)) {

        Preferences prefs(arg);

        // push2 api setup
        push2Api_.reset(new Push2API::Push2());
        push2Api_->init();

        // Kontrol setup
        param_model_ = oKontrol::ParameterModel::model();
        oled_ = std::make_shared<Push2_OLED>(*this, push2Api_);
        param_model_->addCallback("push2.oled", oled_);

        push2Api_->clearDisplay();

        // // 'test for Push 1 compatibility mode'
        // int row = 2;
        // push2Api_->drawText(row, 10, "...01234567891234567......");
        // push2Api_->clearRow(row);
        // push2Api_->p1_drawCell4(row, 0, "01234567891234567");
        // push2Api_->p1_drawCell4(row, 1, "01234567891234567");
        // push2Api_->p1_drawCell4(row, 2, "01234567891234567");
        // push2Api_->p1_drawCell4(row, 3, "01234567891234567");


        active_ = true;
        updatePadColours();
        LOG_0("Push2::init - complete");

        return active_;
    }
    return false;
}

bool Push2::process() {
    push2Api_->render(); // TODO maybe this should be moved off to separate slow thread

    while (PaUtil_GetRingBufferReadAvailable(& midiQueue_))
    {
        MidiMsg msg;
        PaUtil_ReadRingBuffer(& midiQueue_, &msg, 1);
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

bool Push2::midiCallback(double deltatime, std::vector< unsigned char > *message)  {
    unsigned int n = message->size();
    if (n > 3)  LOG_0("midiCallback unexpect midi size" << n);

    MidiMsg m;

    m.data[0] = (int)message->at(0);
    if (n > 1) m.data[1] = (int)message->at(1);
    if (n > 2) m.data[2] = (int)message->at(2);

    // LOG_0("midi: s " << std::hex << m.status_ << " "<< m.data[1] << " " << m.data[2]);
    PaUtil_WriteRingBuffer(&midiQueue_, (void*) &m, 1);
    return true;
}

bool Push2::processMidi(const MidiMsg& midimsg)  {
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
            msg.data_.touch_.z_ = float(midimsg.data[2]) / 127.0;
            queue_.addToQueue(msg);
        } else {
            if (midimsg.data[2] > 0) {
                processPushNoteOn(midimsg.data[1], midimsg.data[2]);
            } else {
                processPushNoteOff(midimsg.data[1], 100);
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
            msg.data_.touch_.z_ = float(midimsg.data[2]) / 127.0;
            queue_.addToQueue(msg);
        } else {
            processPushNoteOff(midimsg.data[1], midimsg.data[2]);
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
        msg.data_.touch_.z_ = float(midimsg.data[2]) / 127.0;
        queue_.addToQueue(msg);
    }
    case 0xB0: { // CC
        processPushCC(midimsg.data[1], midimsg.data[2]);
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


void Push2::processPushNoteOn(unsigned n, unsigned v) {

}

void Push2::processPushNoteOff(unsigned n, unsigned v) {

}

void Push2::processPushCC(unsigned cc, unsigned v) {
    if (cc >= P2_ENCODER_CC_START && cc <= P2_ENCODER_CC_END) {
        unsigned idx = cc - P2_ENCODER_CC_START;
        auto pageId = param_model_->getPageId(currentPage_);
        if (pageId.empty()) return;
        auto id = param_model_->getParamId(pageId, idx);
        if (id.empty()) return;

        auto param = param_model_->getParam(id);
        if (param != nullptr) {
            const int steps = 1;
            // LOG_0("v = " << v);
            float  vel = 0.0;
            if (v & 0x40) {
                // -ve
                vel =  (128.0 - (float) v) / (128.0 * steps)  * -1.0;
                // LOG_0("vel - = " << vel);
            } else {
                vel = float(v) / (128.0 * steps);
                // LOG_0("vel = " << vel);
            }

            oKontrol::ParamValue calc = param->calcRelative(vel);
            param_model_->changeParam(oKontrol::PS_LOCAL, id, calc);
        }

    } else if (cc >= P2_DEV_SELECT_CC_START && cc <= P2_DEV_SELECT_CC_END) {
        unsigned idx = cc - P2_DEV_SELECT_CC_START;

    } else if (cc >= P2_TRACK_SELECT_CC_START && cc <= P2_TRACK_SELECT_CC_END) {
        unsigned idx = cc - P2_TRACK_SELECT_CC_START;
        if (currentPage_ != idx && idx < param_model_->getPageCount()) {
            currentPage_ = idx;
            oled_->setCurrentPage(currentPage_);
        }
    }
}

#define PAD_NOTE_ON_CLR 127
#define PAD_NOTE_OFF_CLR 0
#define PAD_NOTE_ROOT_CLR 41
#define PAD_NOTE_IN_KEY_CLR 3


void Push2::updatePadColours() {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int8_t clr = determinePadScaleColour(r, c);
            sendNoteOn(0, P2_NOTE_PAD_START + (r * 8) + c, clr);
        }
    }
}

int8_t Push2::determinePadScaleColour(int8_t r, int8_t c) {
    int8_t note_s = (r * rowOffset_) + c;
    if (chromatic_) {
        int i = note_s % 12;
        int v = scale_  & ( 1 << (11 - i) );

        return i == 0 ? PAD_NOTE_ROOT_CLR :  (v > 0  ? PAD_NOTE_IN_KEY_CLR : PAD_NOTE_OFF_CLR);
    } else {
        return (note_s % numNotesInScale_) == 0 ? PAD_NOTE_ROOT_CLR : PAD_NOTE_IN_KEY_CLR;
    }
}


// Push2_OLED
void Push2_OLED::addClient(const std::string& host, unsigned port) {
    // LOG_0("Push2_OLED::addClient from:" << host << ":" << port);
}

std::string centreText(const std::string t) {
    unsigned len = t.length();
    if (len > 24) return t;
    const std::string pad = "                         ";
    unsigned padlen = (24 - len) / 2;
    std::string ret = pad.substr(0, padlen) + t + pad.substr(0, padlen);
    // LOG_0("pad " << t << " l " << len << " pl " << padlen << "[" << ret << "]");
    return ret;
}

static const unsigned VSCALE = 3;
static const unsigned HSCALE = 1;

int16_t page_clrs[8] = {
    RGB565(0xFF, 0xFF, 0xFF),
    RGB565(0xFF, 0, 0xFF),
    RGB565(0xFF, 0, 0),
    RGB565(0, 0xFF, 0xFF),
    RGB565(0, 0xFF, 0),
    RGB565(0x7F, 0x7F, 0xFF),
    RGB565(0xFF, 0x7F, 0xFF)
};

void Push2_OLED::drawParam(unsigned pos, const oKontrol::Parameter& param) {
    // #define MONO_CLR RGB565(255,60,0)
    int16_t clr = page_clrs[currentPage_];

    push2Api_->drawCell8(1, pos, centreText(param.displayName()).c_str(), VSCALE, HSCALE, clr);
    push2Api_->drawCell8(2, pos, centreText(param.displayValue()).c_str(), VSCALE, HSCALE, clr);
    push2Api_->drawCell8(3, pos, centreText(param.displayUnit()).c_str(), VSCALE, HSCALE, clr);
}

void Push2_OLED::displayPage(const std::string& pageId) {
    int16_t clr = page_clrs[currentPage_];
    push2Api_->clearDisplay();
    for (int i = P2_TRACK_SELECT_CC_START; i < P2_TRACK_SELECT_CC_END; i++) { parent_.sendCC(0, i, 0);}

    push2Api_->drawCell8(0, 0, "Kontrol", VSCALE, HSCALE, RGB565(0x7F, 0x7F, 0x7F));

    for (int i = 0; i < 8 && i < param_model_->getPageCount(); i++) {
        auto pageId = param_model_->getPageId(i);
        if (pageId.empty()) return;

        auto page = param_model_->getPage(pageId);
        if (page == nullptr) continue;

        push2Api_->drawCell8(5, i, centreText(page->displayName()).c_str(), VSCALE, HSCALE, page_clrs[i]);
        parent_.sendCC(0, P2_TRACK_SELECT_CC_START + i, i == currentPage_ ? 122 : 124);
        if (i == currentPage_) {
            for (int j = 0; j < 8; j++) {
                auto id = param_model_->getParamId(pageId, j);
                if (!id.empty()) {
                    auto param  = param_model_ -> getParam(id);
                    if (param != nullptr) {
                        drawParam(j, *param);
                    }
                }
            }
        }
    }
}


void Push2_OLED::setCurrentPage(unsigned page) {
    if (page != currentPage_) {
        currentPage_ = page;
        auto pageId = param_model_->getPageId(currentPage_);
        displayPage(pageId);
    }
}


void Push2_OLED::page(oKontrol::ParameterSource , const oKontrol::Page& page) {
    // LOG_0("Push2_OLED::page " << page.id());
    displayPage(page.id());
}

void Push2_OLED::param(oKontrol::ParameterSource, const oKontrol::Parameter& param) {

    auto pageId = param_model_->getPageId(currentPage_);
    if (pageId.empty()) return;

    for (int i = 0; i < 8; i++) {
        auto id = param_model_->getParamId(pageId, i);
        if (id.empty()) return;
        if ( id == param.id()) {
            drawParam(i, param);
            return;
        }
    }
}

void Push2_OLED::changed(oKontrol::ParameterSource, const oKontrol::Parameter& param) {
    auto pageId = param_model_->getPageId(currentPage_);
    if (pageId.empty()) return;

    for (int i = 0; i < 8; i++) {
        auto id = param_model_->getParamId(pageId, i);
        if (id.empty()) return;
        if ( id == param.id()) {
            drawParam(i, param);
            return;
        }
    }
}



} // namespace



