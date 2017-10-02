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
RtMidiIn::RtMidiCallback MidiDevice::getMidiCallback() {

    return MidiDeviceInCallback;
}


bool MidiDevice::init(void* arg) {
    Preferences prefs(arg);

    if (active_) {
        deinit();
    }
    active_ = false;

    bool found = false;

    std::string input_device = prefs.getString("input device");
    if(!input_device.empty()) {

        try {
            midiInDevice_.reset(new RtMidiIn());
        } catch (RtMidiError  &error) {
            midiInDevice_.reset();
            LOG_0("MidiDevice RtMidiIn ctor error:" << error.what());
            return false;
        }

        mpeMode_ = prefs.getBool("mpe", true);
        pitchbendRange_ = (float) prefs.getDouble("pitchbend range", 48.0);

        for (int i = 0; i < midiInDevice_->getPortCount() && !found; i++) {
            if (input_device.compare(midiInDevice_->getPortName(i)) == 0) {
                try {
                    midiInDevice_->openPort(i);
                    found = true;
                    LOG_1("Midi input opened :" << input_device);
                } catch (RtMidiError  &error) {
                    LOG_0("Midi input open error:" << error.what());
                    midiInDevice_.reset();
                    return false;
                }
            }
        }
        if (!found) {
            LOG_0("Input device not found : [" << input_device << "]");
            LOG_0("available devices:");
            for (int i = 0; i < midiInDevice_->getPortCount(); i++) {
                LOG_0("[" << midiInDevice_->getPortName(i) << "]");
            }
            midiInDevice_.reset();
            return false;
        }


        midiInDevice_->ignoreTypes( true, true, true );
        midiInDevice_->setCallback( getMidiCallback(), this );
    } //midi input

    std::string output_device = prefs.getString("output device");
    if (!output_device.empty()) {
        bool virt =  prefs.getBool("virtual output", false);
        try {
            midiOutDevice_.reset(new RtMidiOut( RtMidi::Api::UNSPECIFIED, "MEC MIDI"));
        } catch (RtMidiError  &error) {
            midiOutDevice_.reset();
            LOG_0("MidiDevice RtMidiOut ctor error:" << error.what());
            return false;
        }
        found = false;
        if (virt) {
            try {
                midiOutDevice_->openVirtualPort(output_device);
                LOG_0( "Midi virtual output created :" << output_device );
                virtualOpen_ = true;
                found = true;
            } catch (RtMidiError  &error) {
                LOG_0("Midi virtual output create error:" << error.what());
                virtualOpen_ = false;
                midiOutDevice_.reset();
                return false;
            }
        } else {
            for (int i = 0; i < midiOutDevice_->getPortCount() && !found ; i++) {
                if (output_device.compare(midiOutDevice_->getPortName(i)) == 0) {
                    try {
                        midiOutDevice_->openPort(i);
                        LOG_0( "Midi output opened :" << output_device );
                        found = true;
                    } catch (RtMidiError  &error) {
                        LOG_0("Midi output create error:" << error.what());
                        midiOutDevice_.reset();
                        return false;
                    }
                }
            }

            if (!found) {
                LOG_0("Output device not found : [" << output_device << "]");
                LOG_0("available devices : ");
                for (int i = 0; i < midiOutDevice_->getPortCount(); i++) {
                    LOG_0("[" << midiOutDevice_->getPortName(i) << "]");
                }
                midiOutDevice_.reset();
            }
        }
    } // midi output


    active_ = midiInDevice_ || midiOutDevice_;
    LOG_0("MidiDevice::init - complete");
    return active_;
}

bool MidiDevice::process() {
    return queue_.process(callback_);
}

void MidiDevice::deinit() {
    LOG_0("MidiDevice::deinit");
    if (midiInDevice_) midiInDevice_->cancelCallback();
    midiInDevice_.reset();
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

bool MidiDevice::send(const MidiMsg& m) {
    if (midiOutDevice_ == nullptr || ! isOutputOpen()) return false;
    std::vector<unsigned char> msg;

    for (int i = 0; i < m.size; i++) {
        msg.push_back(m.data[i]);
    }

    try {
        midiOutDevice_->sendMessage( & msg );
    } catch (RtMidiError  &error) {
        LOG_0("MidiDevice output write error:" << error.what());
        return false;
    }
    return true;
}




} //namespace



