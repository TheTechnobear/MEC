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
MecPush2::MecPush2(IMecCallback& cb) :
    active_(false), callback_(cb) {
}

MecPush2::~MecPush2() {
    deinit();
}


void MecPush2InCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    MecPush2* self = static_cast<MecPush2*>(userData);
    self->midiCallback(deltatime, message);
}


bool MecPush2::init(void* arg) {
    MecPreferences prefs(arg);

    if (active_) {
        deinit();
    }
    active_ = false;

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
    midiDevice_->setCallback( MecPush2InCallback, this );


    push2Api_.reset(new Push2API::Push2());
    push2Api_->init();

    // 'test for Push 1 compatibility mode'
    int row = 2;
    push2Api_->clearDisplay();
    push2Api_->drawText(row, 10, "...01234567891234567......");
    push2Api_->clearRow(row);
    push2Api_->p1_drawCell(row, 0, "01234567891234567");
    push2Api_->p1_drawCell(row, 1, "01234567891234567");
    push2Api_->p1_drawCell(row, 2, "01234567891234567");
    push2Api_->p1_drawCell(row, 3, "01234567891234567");

    active_ = true;
    LOG_0("MecPush2::init - complete");
    return active_;
}

bool MecPush2::process() {
    MecMsg msg;
    while (queue_.nextMsg(msg)) {
        switch (msg.type_) {
        case MecMsg::TOUCH_ON:
            callback_.touchOn(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::TOUCH_CONTINUE:
            callback_.touchContinue(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::TOUCH_OFF:
            callback_.touchOff(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::CONTROL :
            callback_.control(
                msg.data_.control_.controlId_,
                msg.data_.control_.value_);
            break;
        default:
            LOG_0("MecPush2::process unhandled message type");
        }
    }

    // TODO maybe this should be moved off to separate slow thread
    push2Api_->render();

    return true;
}

void MecPush2::deinit() {
    LOG_0("MecPush2::deinit");

    if(push2Api_)push2Api_->deinit();
    push2Api_.reset();

    if (midiDevice_) midiDevice_->cancelCallback();
    midiDevice_.reset();

    active_ = false;
}

bool MecPush2::isActive() {
    return active_;
}

bool MecPush2::midiCallback(double deltatime, std::vector< unsigned char > *message)  {
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
        if(data1>0)
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





