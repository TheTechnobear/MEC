#ifndef Mec_MPE_Processor_H
#define Mec_MPE_Processor_H

//////////////
// this class can be used to process incoming callbacks and convert into Midi messages
// define the process method to determine what to do with the midi message

#include "../mec_api.h"

#include <list>

namespace mec {

class MPE_Processor : public ICallback {
public:
    MPE_Processor(float pbr=48.0);
    virtual ~MPE_Processor();

    struct MidiMsg {
        MidiMsg() { data[0]=0;size=0;}
        MidiMsg(char status) { data[0] = status; size=1;}
        MidiMsg(char status, char d1) : MidiMsg(status) {data[1]=d1;size=2;}
        MidiMsg(char status, char d1, char d2) : MidiMsg(status,d1) {data[2]=d2;size=3;}

        char        data[3];
        unsigned    size;
    };


    virtual void  process(MidiMsg& msg) = 0;
    void setPitchbendRange(float pbr);

    // ICallback handling
    virtual void touchOn(int touchId, float note, float x, float y, float z);
    virtual void touchContinue(int touchId, float note, float x, float y, float z);
    virtual void touchOff(int touchId, float note, float x, float y, float z);
    virtual void control(int ctrlId, float v);
    virtual void mec_control(int cmd, void* other); //ignores
  
private:

    // low level midi, open unchecked
    bool noteOn(unsigned ch, unsigned note, unsigned vel);
    bool noteOff(unsigned ch, unsigned note, unsigned vel);
    bool cc(unsigned ch, unsigned cc, unsigned v);
    bool pressure(unsigned ch, unsigned v);
    bool pitchbend(unsigned ch, unsigned v);

    int bipolar14bit(float v) {return ((v * 0x2000) + 0x2000);}
    int bipolar7bit(float v) {return ((v / 2.0f) + 0.5f)  * 127; }
    int unipolar7bit(float v) {return v * 127;}

    struct VoiceData {
            unsigned    startNote_;
            unsigned    note_;      //0
            int         pitchbend_; //1
            int         timbre_;    //2
            unsigned    pressure_;  //3
    };

    VoiceData voices_[16];
    float global_[127];
    float pitchbendRange_;
};    

}

#endif //MecMidiProcessor_H