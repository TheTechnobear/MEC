#ifndef MecMidi_H 
#define MecMidi_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

#include <RtMidi.h>

#include <memory>
#include <vector>

class MecMidi : public MecDevice {

public:
    MecMidi(IMecCallback&);
    virtual ~MecMidi();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    virtual bool midiCallback(double deltatime, std::vector< unsigned char > *message);

private:

    bool active_;

	IMecCallback& callback_;
    std::unique_ptr<RtMidiIn> midiDevice_; 

    MecMsgQueue queue_;

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

#endif // MecMidi_H