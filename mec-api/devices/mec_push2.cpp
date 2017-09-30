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

////////////////////////////////////////////////

namespace mec {


class Push2_OLED : public oKontrol::ParameterCallback {
public:
    Push2_OLED(const std::shared_ptr<Push2API::Push2>& api) : push2Api_(api) {
        param_model_ = oKontrol::ParameterModel::model();
        ;
    }
    // ParameterCallback
    virtual void addClient(const std::string&, unsigned );
    virtual void page(oKontrol::ParameterSource , const oKontrol::Page& );
    virtual void param(oKontrol::ParameterSource, const oKontrol::Parameter&);
    virtual void changed(oKontrol::ParameterSource src, const oKontrol::Parameter& p);

private:
    void drawParam(unsigned pos, const oKontrol::Parameter& param);

    std::shared_ptr<Push2API::Push2> push2Api_;
    std::shared_ptr<oKontrol::ParameterModel> param_model_;
};

Push2::Push2(ICallback& cb) :
    active_(false), callback_(cb) {
}

Push2::~Push2() {
    deinit();
}

void Push2InCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    Push2* self = static_cast<Push2*>(userData);
    self->midiCallback(deltatime, message);
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
    param_model_->addCallback(std::make_shared<Push2_OLED>(push2Api_));

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

    return queue_.process(callback_);
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

bool Push2::midiCallback(double deltatime, std::vector< unsigned char > *message)  {
    int status = 0, data1 = 0, data2 = 0, data3 = 0;
    unsigned int n = message->size();
    if (n > 3)  LOG_0("midiCallback unexpect midi size" << n);

    status = (int)message->at(0);
    if (n > 1) data1 = (int)message->at(1);
    if (n > 2) data2 = (int)message->at(2);
    if (n > 3) data3 = (int)message->at(3);


    // for now just send messages thru the touch interface
    // later we are going to be interpretting CC/notes to provide navigation etc
    int ch = status & 0x0F;
    int type = status & 0xF0;
    MecMsg msg;
    switch (type) {
    case 0x90: {
        // note on (+note off if vel =0)
        if (data1 > 0)
            msg.type_ = MecMsg::TOUCH_ON;
        else
            msg.type_ = MecMsg::TOUCH_OFF;

        msg.data_.touch_.touchId_ = ch;
        msg.data_.touch_.note_ = (float) data1;
        msg.data_.touch_.x_ = 0;
        msg.data_.touch_.y_ = 0;
        msg.data_.touch_.z_ = float(data2) / 127.0;
        queue_.addToQueue(msg);
        break;
    }

    case 0x80: {
        // note off
        msg.type_ = MecMsg::TOUCH_OFF;
        msg.data_.touch_.touchId_ = ch;
        msg.data_.touch_.note_ = (float) data1;
        msg.data_.touch_.x_ = 0;
        msg.data_.touch_.y_ = 0;
        queue_.addToQueue(msg);
        msg.data_.touch_.z_ = float(data2) / 127.0;
        break;
    }
    case 0xB0: {
        // CC
        float v = float(data2) / 127.0f;
        msg.type_ = MecMsg::CONTROL;
        msg.data_.control_.controlId_ = data1;
        msg.data_.control_.value_ = v;
        queue_.addToQueue(msg);
        break;
    }
    case 0xD0: {
        // channel pressure
        float v = float(data1) / 127.0f;
        msg.type_ = MecMsg::CONTROL;
        msg.data_.control_.controlId_ = type;
        msg.data_.control_.value_ = v;
        queue_.addToQueue(msg);
        break;
    }
    case 0xE0: {
        // PB
        float pb = (float) ((data2 << 7) + data1);
        float v = (pb / 8192.0f) - 1.0f;  // -1.0 to 1.0
        msg.type_ = MecMsg::CONTROL;
        msg.data_.control_.controlId_ = type;
        msg.data_.control_.value_ = v;
        queue_.addToQueue(msg);
        break;
    }
    default: {
        // everything else;
        //ignore
        break;
    }
    } //switch

    return true;
}


unsigned currentPage = 0;
// Push2_OLED
void Push2_OLED::addClient(const std::string& host, unsigned port) {
    // LOG_0("Push2_OLED::addClient from:" << host << ":" << port);
}

std::string centreText(const std::string t) {
    unsigned len = t.length();
    if(len>12) return t;
    const std::string pad = "            "; 
    unsigned padlen = (12 - len) / 2;
    std::string ret = pad.substr(0,padlen) + t + pad.substr(0,padlen);
    // LOG_0("pad " << t << " l " << len << " pl " << padlen << "[" << ret << "]");
    return ret;
}

void Push2_OLED::drawParam(unsigned pos,const oKontrol::Parameter& param) {
    push2Api_->p1_drawCell8(1, pos, centreText(param.displayName()).c_str());
    push2Api_->p1_drawCell8(2, pos, centreText(param.displayValue()).c_str());
    push2Api_->p1_drawCell8(3, pos, centreText(param.displayUnit()).c_str());
}

void Push2_OLED::page(oKontrol::ParameterSource , const oKontrol::Page& page) {
    // LOG_0("Push2_OLED::page " << page.id());

    for (int i = 0; i < 8 && i < param_model_->getPageCount(); i++) {
        auto pageId = param_model_->getPageId(i);
        if (pageId.empty()) return;
    
        if (pageId == page.id()) {
            push2Api_->p1_drawCell8(0, i, centreText(page.displayName()).c_str());
            if(i==currentPage) {
                for (int j = 0; j < 8; j++) {
                    auto id = param_model_->getParamId(pageId, j);
                    if (!id.empty()) {
                        auto param  = param_model_ -> getParam(id);
                        if(param!=nullptr) {
                            drawParam(j,*param);
                        }
                    }
                }
            }
            return;
        }
    }   
}

void Push2_OLED::param(oKontrol::ParameterSource, const oKontrol::Parameter& param) {

    auto pageId = param_model_->getPageId(currentPage);
    if (pageId.empty()) return;

    for (int i = 0; i < 8; i++) {
        auto id = param_model_->getParamId(pageId, i);
        if (id.empty()) return;
        if ( id == param.id()){
            drawParam(i,param);
            return;
        }
    }
}

void Push2_OLED::changed(oKontrol::ParameterSource, const oKontrol::Parameter& param) {
    auto pageId = param_model_->getPageId(currentPage);
    if (pageId.empty()) return;

    for (int i = 0; i < 8; i++) {
        auto id = param_model_->getParamId(pageId, i);
        if (id.empty()) return;
        if ( id == param.id()){
            drawParam(i,param);
            return;
        }
    }
}



} // namespace



