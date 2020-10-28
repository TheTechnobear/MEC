#pragma once
//////////////
// this class can be used to process incoming callbacks and convert into Midi messages
// define the process method to determine what to do with the midi message

#include "../mec_api.h"
#include "mec_midi_processor.h"
#include <list>

namespace mec {

class MPE_Processor : public Midi_Processor {
public:
    MPE_Processor(float pbr = 48.0);
    virtual ~MPE_Processor();

    virtual void  process(MidiMsg& msg) = 0;

    // ICallback handling
    virtual void touchOn(int touchId, float note, float x, float y, float z);
    virtual void touchContinue(int touchId, float note, float x, float y, float z);
    virtual void touchOff(int touchId, float note, float x, float y, float z);
    virtual void control(int ctrlId, float v);
    virtual void mec_control(int cmd, void* other); //ignores

private:
    static constexpr unsigned MAX_VOICE=16;

    struct VoiceData {
        unsigned    startNote_;
        unsigned    note_;      //0
        unsigned    pitchbend_; //1
        unsigned    timbre_;    //2
        unsigned    pressure_;  //3
        bool        active_;
    };

    VoiceData voices_[MAX_VOICE];
};

}

