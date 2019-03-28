#ifndef MecDeviceMidi_H
#define MecDeviceMidi_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

#include <RtMidi.h>

#include <memory>
#include <vector>

namespace mec {

class MidiDevice : public Device {

public:
    MidiDevice(ICallback &);
    virtual ~MidiDevice();
    virtual bool init(void *);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    virtual bool midiCallback(double deltatime, std::vector<unsigned char> *message);

    bool sendCC(unsigned ch, unsigned cc, unsigned v) { return send(MidiMsg(0xB0 + ch, cc, v)); }

    bool sendNoteOn(unsigned ch, unsigned note, unsigned vel) { return send(MidiMsg(int(0x90 + ch), note, vel)); }

    bool sendNoteOff(unsigned ch, unsigned note, unsigned vel) { return send(MidiMsg(int(0x80 + ch), note, vel)); }

    void queueMecMsg(MecMsg &msg) { queue_.addToQueue(msg);}

protected:
    virtual RtMidiIn::RtMidiCallback getMidiCallback();

    struct MidiMsg {
        MidiMsg() {
            data[0] = 0;
            size = 0;
        }

        MidiMsg(unsigned char status) {
            data[0] = status;
            size = 1;
        }

        MidiMsg(unsigned char status, unsigned char d1) : MidiMsg(status) {
            data[1] = d1;
            size = 2;
        }

        MidiMsg(unsigned char status, unsigned char d1, unsigned char d2) : MidiMsg(status, d1) {
            data[2] = d2;
            size = 3;
        }

        unsigned char data[3];
        unsigned size;
    };


    bool isOutputOpen() { return (midiOutDevice_ && (virtualOpen_ || midiOutDevice_->isPortOpen())); }

    bool send(const MidiMsg &msg);

    bool active_;

    ICallback &callback_;
    std::unique_ptr<RtMidiIn> midiInDevice_;
    std::unique_ptr<RtMidiOut> midiOutDevice_;
    bool virtualOpen_;

    MsgQueue queue_;

    struct VoiceData {
        float startNote_;
        float note_;
        float x_;
        float y_;
        float z_;
        bool active_;
    };
    VoiceData touches_[16];

    float pitchbendRange_;
    bool mpeMode_;
};


}

#endif // MecDeviceMidi_H