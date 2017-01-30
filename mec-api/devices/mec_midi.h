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
    MidiDevice(ICallback&);
    virtual ~MidiDevice();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    virtual bool midiCallback(double deltatime, std::vector< unsigned char > *message);

private:

    bool active_;

	ICallback& callback_;
    std::unique_ptr<RtMidiIn> midiDevice_; 

    MsgQueue queue_;

    struct VoiceData {
            float startNote_;
            float note_;
            float x_;
            float y_;
            float z_;
            bool  active_;
    };
    VoiceData   touches_[16];

    float       pitchbendRange_;
    bool        mpeMode_;
};

}

#endif // MecDeviceMidi_H