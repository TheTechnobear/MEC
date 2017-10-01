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

const unsigned P2_NOTE_PAD_START = 32;
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


class Push2_OLED : public oKontrol::ParameterCallback {
public:
    Push2_OLED(const std::shared_ptr<Push2API::Push2>& api) : push2Api_(api), currentPage_(0) {
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

    std::shared_ptr<Push2API::Push2> push2Api_;
    std::shared_ptr<oKontrol::ParameterModel> param_model_;
    unsigned currentPage_;
};

Push2::Push2(ICallback& cb) :
    active_(false), callback_(cb) {
    PaUtil_InitializeRingBuffer(& midiQueue_, sizeof(MidiMsg), MAX_N_MIDI_MSGS, msgData_);
}

Push2::~Push2() {
    deinit();
}

void Push2InCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    Push2* self = static_cast<Push2*>(userData);
    self->midiInCallback(deltatime, message);
}


bool Push2::init(void* arg) {
    Preferences prefs(arg);

    if (active_) {
        deinit();
    }
    active_ = false;

    // midi setup
    midiDevice_.reset(new RtMidiIn());

    std::string portname = prefs.getString("device");
    pitchbendRange_ = (float) prefs.getDouble("pitchbend range", 48.0);

    bool found = false;
    for (int i = 0; i < midiDevice_->getPortCount() && !found; i++) {
        if (portname.compare(midiDevice_->getPortName(i)) == 0) {
            try {
                midiDevice_->openPort(i);
                found = true;
                LOG_1("Midi input opened :" << portname);
            } catch (RtMidiError  &error) {
                LOG_0("Midi input open error:" << error.what());
                return false;
            }
        }
    }
    if (!found) {
        LOG_0("input port not found : [" << portname << "]");
        LOG_0("available ports:");
        for (int i = 0; i < midiDevice_->getPortCount(); i++) {
            LOG_0("[" << midiDevice_->getPortName(i) << "]");
        }
        return false;
    }

    midiDevice_->ignoreTypes( true, true, true );
    midiDevice_->setCallback( Push2InCallback, this );


    // push2 api setup
    push2Api_.reset(new Push2API::Push2());
    push2Api_->init();

    // Kontrol setup
    param_model_ = oKontrol::ParameterModel::model();
    oled_ = std::make_shared<Push2_OLED>(push2Api_);
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
    LOG_0("Push2::init - complete");
    return active_;
}

bool Push2::process() {
    push2Api_->render(); // TODO maybe this should be moved off to separate slow thread

    while (PaUtil_GetRingBufferReadAvailable(& midiQueue_))
    {
        MidiMsg msg;
        PaUtil_ReadRingBuffer(& midiQueue_, &msg, 1);
        processMidi(msg);
    }

    return touchQueue_.process(callback_);
}

void Push2::deinit() {
    LOG_0("Push2::deinit");

    if (push2Api_)push2Api_->deinit();
    push2Api_.reset();

    if (midiDevice_) midiDevice_->cancelCallback();
    midiDevice_.reset();

    active_ = false;
}

bool Push2::isActive() {
    return active_;
}

bool Push2::midiInCallback(double deltatime, std::vector< unsigned char > *message)  {
    unsigned int n = message->size();
    if (n > 3)  LOG_0("midiCallback unexpect midi size" << n);

    MidiMsg m;
    m.status_ = (int)message->at(0);
    if (n > 1) m.data1_ = (int)message->at(1);
    if (n > 2) m.data2_ = (int)message->at(2);

    // LOG_0("midi: s " << std::hex << m.status_ << " "<< m.data1_ << " " << m.data2_);
    PaUtil_WriteRingBuffer(&midiQueue_, (void*) &m, 1);
    return true;
}

bool Push2::processMidi(const MidiMsg& midimsg)  {
    int ch = midimsg.status_ & 0x0F;
    int type = midimsg.status_ & 0xF0;
    MecMsg msg;
    switch (type) {
    case 0x90: { // note on
        if (midimsg.data1_ >= P2_NOTE_PAD_START && midimsg.data1_ <= P2_NOTE_PAD_END) {
            // TODO: use voice, or make single ch midi
            if (midimsg.data2_ > 0)
                msg.type_ = MecMsg::TOUCH_ON;
            else
                msg.type_ = MecMsg::TOUCH_OFF;

            msg.data_.touch_.touchId_ = ch;
            msg.data_.touch_.note_ = (float) midimsg.data1_;
            msg.data_.touch_.x_ = 0;
            msg.data_.touch_.y_ = 0;
            msg.data_.touch_.z_ = float(midimsg.data2_) / 127.0;
            touchQueue_.addToQueue(msg);
        } else {
            if (midimsg.data2_ > 0) {
                processPushNoteOn(midimsg.data1_, midimsg.data2_);
            } else {
                processPushNoteOff(midimsg.data1_, 100);
            }
        }
        break;
    }

    case 0x80: { // note off
        if (midimsg.data1_ >= P2_NOTE_PAD_START && midimsg.data1_ <= P2_NOTE_PAD_END) {
            // TODO: use voice, or make single ch midi
            msg.type_ = MecMsg::TOUCH_OFF;
            msg.data_.touch_.touchId_ = ch;
            msg.data_.touch_.note_ = (float) midimsg.data1_;
            msg.data_.touch_.x_ = 0;
            msg.data_.touch_.y_ = 0;
            msg.data_.touch_.z_ = float(midimsg.data2_) / 127.0;
            touchQueue_.addToQueue(msg);
        } else {
            processPushNoteOff(midimsg.data1_, midimsg.data2_);
        }
        break;
    }
    case 0xA0: { // poly pressure
        // TODO: use voice, or make single ch midi
        msg.type_ = MecMsg::TOUCH_CONTINUE;
        msg.data_.touch_.touchId_ = ch;
        msg.data_.touch_.note_ = (float) midimsg.data1_;
        msg.data_.touch_.x_ = 0;
        msg.data_.touch_.y_ = 0;
        msg.data_.touch_.z_ = float(midimsg.data2_) / 127.0;
        touchQueue_.addToQueue(msg);
    }
    case 0xB0: { // CC
        processPushCC(midimsg.data1_, midimsg.data2_);
        break;
    }
    case 0xD0: { // channel pressure
        // float v = float(midimsg.data1_) / 127.0f;
        // msg.type_ = MecMsg::CONTROL;
        // msg.data_.control_.controlId_ = type;
        // msg.data_.control_.value_ = v;
        // touchQueue_.addToQueue(msg);
        break;
    }
    case 0xE0: { // pitchbend
        // float pb = (float) ((midimsg.data2_ << 7) + midimsg.data1_);
        // float v = (pb / 8192.0f) - 1.0f;  // -1.0 to 1.0
        // msg.type_ = MecMsg::CONTROL;
        // msg.data_.control_.controlId_ = type;
        // msg.data_.control_.value_ = v;
        // touchQueue_.addToQueue(msg);
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
    push2Api_->drawCell8(0, 0, "Kontrol", VSCALE, HSCALE, RGB565(0x7F,0x7F,0x7F));

    for (int i = 0; i < 8 && i < param_model_->getPageCount(); i++) {
        auto pageId = param_model_->getPageId(i);
        if (pageId.empty()) return;

        auto page = param_model_->getPage(pageId);
        if (page == nullptr) continue;

        push2Api_->drawCell8(5, i, centreText(page->displayName()).c_str(), VSCALE, HSCALE, page_clrs[i]);

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



