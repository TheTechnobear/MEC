#ifndef MEC_MIDI_OUTPUT_H
#define MEC_MIDI_OUTPUT_H

#include <memory>
#include <RtMidi.h>

#include <mec_voice.h>

struct MidiVoiceData {
        unsigned startNote_;
        unsigned note_;      //0
        int pitchbend_; //1
        int timbre_;    //2
        unsigned pressure_;  //3
        int changeMask_; // not used currently
};


class MidiOutput {
public:
    MidiOutput(int maxVoices, float pbr = 48.0);
    virtual ~MidiOutput();

    bool create(const std::string& portname,bool virt=false);

    bool isOpen() { return (output_ && output_->isPortOpen()); }

    // send msg directly, check open
    bool sendMsg(std::vector<unsigned char>& msg);

    // touch interface, check open
    bool control(int id, int attr, float value, bool isBipolar = false);
    bool touchOn(int id, float note, float x, float y, float z);
    bool touchContinue(int id, float note, float x, float y, float z);
    bool touchOff(int id);

    // low level midi, open unchecked
    bool noteOn(unsigned ch, unsigned note, unsigned vel);
    bool noteOff(unsigned ch, unsigned note, unsigned vel);
    bool cc(unsigned ch, unsigned cc, unsigned v);
    bool pressure(unsigned ch, unsigned v);
    bool pitchbend(unsigned ch, unsigned v);

    int bipolar14bit(float v) {return ((v * 0x2000) + 0x2000);}
    int bipolar7bit(float v) {return ((v / 2) + 0.5)  * 127; }
    int unipolar7bit(float v) {return v * 127;}
    void setPitchbendRange(float v) { pitchbendRange_ = v;}

private:
    std::unique_ptr<RtMidiOut> output_;
    MidiVoiceData voices_[16];
    float global_[127];
    float pitchbendRange_;
};

#endif //MEC_MIDI_OUTPUT_H
