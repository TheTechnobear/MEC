#include "mec_mididevice.h"

#include "../mec_log.h"
#include "../mec_prefs.h"
#include "../mec_voice.h"

namespace mec {

////////////////////////////////////////////////
MidiDevice::MidiDevice(ICallback& cb) :
    active_(false), callback_(cb) {
}

MidiDevice::~MidiDevice() {
    deinit();
}


void MidiDeviceInCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    MidiDevice* self = static_cast<MidiDevice*>(userData);
    self->midiCallback(deltatime, message);
}


bool MidiDevice::init(void* arg) {
    Preferences prefs(arg);

    if (active_) {
        deinit();
    }
    active_ = false;

    midiDevice_.reset(new RtMidiIn());

    std::string portname = prefs.getString("device");
    mpeMode_ = prefs.getBool("mpe", true);
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
    midiDevice_->setCallback( MidiDeviceInCallback, this );

    active_ = true;
    LOG_0("MidiDevice::init - complete");
    return active_;
}

bool MidiDevice::process() {
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
            LOG_0("MidiDevice::process unhandled message type");
        }
    }
    return true;
}

void MidiDevice::deinit() {
    LOG_0("MidiDevice::deinit");
    if (midiDevice_) midiDevice_->cancelCallback();
    midiDevice_.reset();
    active_ = false;
}

bool MidiDevice::isActive() {
    return active_;
}

bool MidiDevice::midiCallback(double deltatime, std::vector< unsigned char > *message)  {
    int status = 0, data1 = 0, data2 = 0, data3 = 0;
    unsigned int n = message->size();
    if (n > 3)  LOG_0("midiCallback unexpect midi size" << n);

    status = (int)message->at(0);
    if (n > 1) data1 = (int)message->at(1);
    if (n > 2) data2 = (int)message->at(2);
    if (n > 3) data3 = (int)message->at(3);


    int ch = status & 0x0F;
    int type = status & 0xF0;
    VoiceData& touch = touches_[ch];
    MecMsg msg;
    switch (type) {
    case 0x90: {
        // note on (+note off if vel =0)
        if (mpeMode_) {
            if (touch.active_ || data2 == 0)  {
                // touch.x_ = 0.0f;
                touch.y_ = 0.0f;
                touch.z_ = 0.0f;
                touch.active_ = false;
                // assumption: callback handler wants note to be touch finish position
                msg.type_ = MecMsg::TOUCH_OFF;
                msg.data_.touch_.touchId_ = ch;
                msg.data_.touch_.note_ = touch.note_;
                msg.data_.touch_.x_ = touch.x_;
                msg.data_.touch_.y_ = touch.y_;
                msg.data_.touch_.z_ = touch.z_;
                queue_.addToQueue(msg);

                touch.startNote_ = 0.0f;
            }
            if (data2 > 0) {
                touch.startNote_ = (float) data1;
                touch.note_ = (float) data1;
                touch.x_ = 0.0f;
                touch.y_ = 0.0f;
                touch.z_ = float(data2) / 127.0f;
                touch.active_ = true;

                msg.type_ = MecMsg::TOUCH_ON;
                msg.data_.touch_.touchId_ = ch;
                msg.data_.touch_.note_ = touch.note_;
                msg.data_.touch_.x_ = touch.x_;
                msg.data_.touch_.y_ = touch.y_;
                msg.data_.touch_.z_ = touch.z_;
                queue_.addToQueue(msg);
            }
        } else {
            // not mpe mode, just send thru, allow multi notes on 1 channel
            touch.startNote_ = (float) data1;
            touch.note_ = (float) data1;
            touch.x_ = 0.0f;
            touch.y_ = 0.0f;
            touch.z_ = float(data2) / 127.0f;
            touch.active_ = true;

            msg.type_ = MecMsg::TOUCH_ON;
            msg.data_.touch_.touchId_ = ch;
            msg.data_.touch_.note_ = touch.note_;
            msg.data_.touch_.x_ = touch.x_;
            msg.data_.touch_.y_ = touch.y_;
            msg.data_.touch_.z_ = touch.z_;
            queue_.addToQueue(msg);
        }
        break;
    }

    case 0x80: {
        // note off
        if (mpeMode_) {
            // touch.note_ == data1 !!
            // touch.x_ = 0.0f;
            touch.y_ = 0.0f;
            touch.z_ = float(data2) / 127.0f;
            touch.active_ = false;
            // assumption: callback handler wants note to be touch finish position
            msg.type_ = MecMsg::TOUCH_OFF;
            msg.data_.touch_.touchId_ = ch;
            msg.data_.touch_.note_ = touch.note_;
            msg.data_.touch_.x_ = touch.x_;
            msg.data_.touch_.y_ = touch.y_;
            msg.data_.touch_.z_ = touch.z_;
            queue_.addToQueue(msg);

            touch.startNote_ = 0.0f;
        } else {
            touch.note_ = (float) data1;
            touch.x_ = 0.0f;
            touch.y_ = 0.0f;
            touch.z_ = float(data2) / 127.0f;
            touch.active_ = true;

            msg.type_ = MecMsg::TOUCH_OFF;
            msg.data_.touch_.touchId_ = ch;
            msg.data_.touch_.note_ = touch.note_;
            msg.data_.touch_.x_ = touch.x_;
            msg.data_.touch_.y_ = touch.y_;
            msg.data_.touch_.z_ = touch.z_;
            queue_.addToQueue(msg);

            touch.startNote_ = 0.0f;
        }
        break;
    }
    case 0xB0: {
        // CC
        float v = float(data2) / 127.0f;
        if (mpeMode_ && data1 == 74) {
            if (touch.active_) {
                touch.y_ = v;

                msg.type_ = MecMsg::TOUCH_CONTINUE;
                msg.data_.touch_.touchId_ = ch;
                msg.data_.touch_.note_ = touch.note_;
                msg.data_.touch_.x_ = touch.x_;
                msg.data_.touch_.y_ = touch.y_;
                msg.data_.touch_.z_ = touch.z_;
                queue_.addToQueue(msg);
            }
        } else {
            msg.type_ = MecMsg::CONTROL;
            msg.data_.control_.controlId_ = data1;
            msg.data_.control_.value_ = v;
            queue_.addToQueue(msg);
        }
        break;
    }
    case 0xD0: {
        // channel pressure
        float v = float(data1) / 127.0f;
        if (mpeMode_) {
            if (touch.active_) {
                touch.z_ = v;

                msg.type_ = MecMsg::TOUCH_CONTINUE;
                msg.data_.touch_.touchId_ = ch;
                msg.data_.touch_.note_ = touch.note_;
                msg.data_.touch_.x_ = touch.x_;
                msg.data_.touch_.y_ = touch.y_;
                msg.data_.touch_.z_ = touch.z_;
                queue_.addToQueue(msg);
            }
        } else {
            msg.type_ = MecMsg::CONTROL;
            msg.data_.control_.controlId_ = type;
            msg.data_.control_.value_ = v;
            queue_.addToQueue(msg);
        }
        break;
    }
    case 0xE0: {
        // PB
        float pb = (float) ((data2 << 7) + data1);
        float v = (pb / 8192.0f) - 1.0f;  // -1.0 to 1.0
        if (mpeMode_) {
            if (touch.active_) {
                touch.note_  = touch.startNote_ + (v * pitchbendRange_);
                touch.x_ = v;

                msg.type_ = MecMsg::TOUCH_CONTINUE;
                msg.data_.touch_.touchId_ = ch;
                msg.data_.touch_.note_ = touch.note_;
                msg.data_.touch_.x_ = touch.x_;
                msg.data_.touch_.y_ = touch.y_;
                msg.data_.touch_.z_ = touch.z_;
                queue_.addToQueue(msg);
            }
        } else {
            msg.type_ = MecMsg::CONTROL;
            msg.data_.control_.controlId_ = type;
            msg.data_.control_.value_ = v;
            queue_.addToQueue(msg);
        }
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

}



