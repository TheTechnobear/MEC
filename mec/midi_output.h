#ifndef MEC_MIDI_OUTPUT_H
#define MEC_MIDI_OUTPUT_H

#include <memory>
#include <RtMidi.h>
#include "voice.h"

class MidiOutput {
public:
    MidiOutput();
    virtual ~MidiOutput();

    bool create(const std::string& portname,bool virt=false);

    bool isOpen() { return (output_ && output_->isPortOpen()); }

    // send msg directly, check open
    bool sendMsg(std::vector<unsigned char>& msg);

    // touch interface, check open
    bool global(int id, int attr, float value, bool isBipolar = false);
    bool startTouch(int id, int note, float x, float y, float z);
    bool continueTouch(int id, int note, float x, float y, float z);
    bool stopTouch(int id);

    // low level midi, open unchecked
    bool noteOn(unsigned ch, unsigned note, unsigned vel);
    bool noteOff(unsigned ch, unsigned note, unsigned vel);
    bool cc(unsigned ch, unsigned cc, unsigned v);
    bool pressure(unsigned ch, unsigned v);
    bool pitchbend(unsigned ch, unsigned v);

    int bipolar14bit(float v) {return ((v * 0x2000) + 0x2000);}
    int bipolar7bit(float v) {return ((v / 2) + 0.5)  * 127; }
    int unipolar7bit(float v) {return v * 127;}
private:

    std::unique_ptr<RtMidiOut> output_;
    Voices voices_;
    float global_[127];
};

#endif //MEC_MIDI_OUTPUT_H
