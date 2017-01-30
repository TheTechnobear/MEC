
#include "mec_midiprocessor.h"

#include "../mec_log.h"
namespace mec {

#define TIMBRE_CC 74

MidiProcessor::MidiProcessor(float pbr) : pitchbendRange_ (pbr) {
    ;
}

MidiProcessor::~MidiProcessor() {
    ;
}

void MidiProcessor::setPitchbendRange(float v) {
    pitchbendRange_ = v;
}

/////////////////////////
// ICallback interface
void MidiProcessor::touchOn(int id, float note, float x, float y, float z) {

    VoiceData& voice = voices_[id];

    unsigned ch = id;
    voice.startNote_ = note; //int

    float semis = note - float(voice.startNote_);
    int pb = bipolar14bit(semis / pitchbendRange_);

    unsigned mx = bipolar14bit(x);
    int my = bipolar7bit(y);
    int mz = unipolar7bit(z);

    // LOG_1("MidiProcessor::touchOn  note: ");
    // LOG_1("   note" << note  << " semi :" << semis << " pb: " << pb);
    // LOG_1("   x :" << x << " mx: " << mx);
    // LOG_1("   y :" << y << " my: " << my);
    // LOG_1("   z :" << z << " mz: " << mz);

    pitchbend(ch, pb);
    cc(ch, TIMBRE_CC,  my);
    noteOn(ch, note, mz);

    voice.note_ = voice.startNote_;
    voice.pitchbend_ = pb;
    voice.timbre_ = my;

    // start with zero z, as we use intial z of velocity
    pressure(ch, 0.0f);
    voice.pressure_ = 0.0f;
}

void MidiProcessor::touchContinue(int id, float note, float x, float y, float z) {

    VoiceData& voice = voices_[id];
    unsigned ch = id;
    // unsigned mx = bipolar14bit(x);
    int my = bipolar7bit(y);
    unsigned mz = unipolar7bit(z);

    float semis = note - float(voice.startNote_);
    int pb = bipolar14bit(semis / pitchbendRange_);

    // LOG_2(std::cout  << "midi output c")
    // LOG_2(           << " note :" << note << " pb: " << pb << " semis: " << semis)
    // LOG_2(           << " y :" << y << " my: " << my)
    // LOG_2(           << " z :" << z << " mz: " << mz)
    // LOG_2(           << " startnote :" << voice.startNote_ << " pbr: " << pitchbendRange_)
    // LOG_2(           )

    voice.note_ = note;

    if (voice.pitchbend_ != pb) {
        voice.pitchbend_ = pb;
        pitchbend(ch, pb) ;
    }
    if (voice.timbre_ != my) {
        voice.timbre_ = my;
        cc(ch, TIMBRE_CC, my);
    }
    if (voice.pressure_ != mz) {
        voice.pressure_ = mz;
        pressure(ch, mz);
    }
}

void MidiProcessor::touchOff(int id, float note, float x, float y, float z) {

    VoiceData& voice = voices_[id];

    unsigned ch = id;
    unsigned vel = 0.0f; // last vel = release velocity
    noteOff(ch, voice.startNote_ , vel);
    pressure(ch, 0.0f);

    voice.startNote_ = 0;
    voice.note_ = 0.0f;
    voice.pitchbend_ = 0.0f;
    voice.timbre_ = 0.0f;
    voice.pressure_ = 0.0f;//
}

void MidiProcessor::control(int attr, float v) {

    if (global_[attr] != v ) {
        global_[attr] = v;
         cc(0, attr, unipolar7bit(v));
        // cc(ch, attr, isBipolar ? bipolar7bit(v) : unipolar7bit(v));
    }
}

void MidiProcessor::mec_control(int cmd, void* other) {
    // ignored
    ;
}


bool MidiProcessor::noteOn(unsigned ch, unsigned note, unsigned vel) {
    // LOG_1( "midi note on ch " << ch << " note " << note  << " vel " << vel );
    MidiMsg msg(int(0x90 + ch), note, vel);
    process(msg);
    return true;
}


bool MidiProcessor::noteOff(unsigned ch, unsigned note, unsigned vel) {
    // LOG_1( "midi  note off ch " << ch << " note " << note  << " vel " << vel )
    MidiMsg msg(int(0x80 + ch), note, vel);
    process(msg);
    return true;
}

bool MidiProcessor::cc(unsigned ch, unsigned cc, unsigned v) {
    // LOG_1( "midi note off ch " << ch << " note " << note  << " vel " << vel )
    MidiMsg msg(0xB0 + ch, cc, v);
    process(msg);
    return true;
}

bool MidiProcessor::pressure(unsigned ch, unsigned v) {
    // LOG_1( "midi pressure ch " << ch << " v  " << v)
    MidiMsg msg(0xD0 + ch, v);
    process(msg);
    return true;
}

bool MidiProcessor::pitchbend(unsigned ch, unsigned v) {
    // LOG_1( "midi pitchbend ch " << ch << " v  " << v)
    MidiMsg msg(0xE0 + ch, v & 0x7f, (v & 0x3F80) >> 7);
    process(msg);
    return true;
}


}