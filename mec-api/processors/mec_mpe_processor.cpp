
#include "mec_mpe_processor.h"

#include "mec_log.h"

namespace mec {

#define TIMBRE_CC 74

MPE_Processor::MPE_Processor(float pbr) : pitchbendRange_ (pbr) {
    ;
}

MPE_Processor::~MPE_Processor() {
    ;
}

void MPE_Processor::setPitchbendRange(float v) {
    pitchbendRange_ = v;
}

/////////////////////////
// ICallback interface
void MPE_Processor::touchOn(int id, float note, float x, float y, float z) {

    VoiceData& voice = voices_[id];

    unsigned ch = id + 1; // MPE starts on 2
    voice.startNote_ = (note + 0.4999999) ; //int

    float semis = note - float(voice.startNote_);
    int pb = bipolar14bit(semis / pitchbendRange_);

    unsigned mx = bipolar14bit(x);
    int my = bipolar7bit(y);
    int mz = unipolar7bit(z);

    // LOG_1("MPE_Processor::touchOn");
    // LOG_1("   note : " << note  << " startNote " << voice.startNote_ << " semi :" << semis << " pb: " << pb);
    // LOG_1("   x :" << x << " mx: " << mx);
    // LOG_1("   y :" << y << " my: " << my);
    // LOG_1("   z :" << z << " mz: " << mz);

    pitchbend(ch, pb);
    cc(ch, TIMBRE_CC,  my);
    noteOn(ch, voice.startNote_, mz);

    voice.note_ = voice.startNote_;
    voice.pitchbend_ = pb;
    voice.timbre_ = my;

    // start with zero z, as we use intial z of velocity
    pressure(ch, 0.0f);
    voice.pressure_ = 0.0f;
}

void MPE_Processor::touchContinue(int id, float note, float x, float y, float z) {

    VoiceData& voice = voices_[id];
    unsigned ch = id + 1; // MPE starts on 2
    // unsigned mx = bipolar14bit(x);
    int my = bipolar7bit(y);
    unsigned mz = unipolar7bit(z);

    float semis = note - float(voice.startNote_);
    int pb = bipolar14bit(semis / pitchbendRange_);

    // LOG_1(std::cout  << "midi output c")
    // LOG_1(           << " note :" << note << " pb: " << pb << " semis: " << semis)
    // LOG_1(           << " y :" << y << " my: " << my)
    // LOG_1(           << " z :" << z << " mz: " << mz)
    // LOG_1(           << " startnote :" << voice.startNote_ << " pbr: " << pitchbendRange_)
    // LOG_1(           )

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

void MPE_Processor::touchOff(int id, float note, float x, float y, float z) {

    VoiceData& voice = voices_[id];

    unsigned ch = id + 1; // MPE starts on 2
    unsigned vel = 0.0f; // last vel = release velocity
    pressure(ch, 0.0f);
    noteOff(ch, voice.startNote_ , vel);

    voice.startNote_ = 0;
    voice.note_ = 0.0f;
    voice.pitchbend_ = 0.0f;
    voice.timbre_ = 0.0f;
    voice.pressure_ = 0.0f;//
}

void MPE_Processor::control(int attr, float v) {

    if (global_[attr] != v ) {
        global_[attr] = v;
         cc(0, attr, unipolar7bit(v));
        // cc(ch, attr, isBipolar ? bipolar7bit(v) : unipolar7bit(v));
    }
}

void MPE_Processor::mec_control(int cmd, void* other) {
    // ignored
    ;
}


bool MPE_Processor::noteOn(unsigned ch, unsigned note, unsigned vel) {
    // LOG_1( "midi note on ch " << ch << " note " << note  << " vel " << vel );
    MidiMsg msg(int(0x90 + ch), note, vel);
    process(msg);
    return true;
}


bool MPE_Processor::noteOff(unsigned ch, unsigned note, unsigned vel) {
    // LOG_1( "midi  note off ch " << ch << " note " << note  << " vel " << vel )
    MidiMsg msg(int(0x80 + ch), note, vel);
    process(msg);
    return true;
}

bool MPE_Processor::cc(unsigned ch, unsigned cc, unsigned v) {
    // LOG_1( "midi note off ch " << ch << " note " << note  << " vel " << vel )
    MidiMsg msg(0xB0 + ch, cc, v);
    process(msg);
    return true;
}

bool MPE_Processor::pressure(unsigned ch, unsigned v) {
    // LOG_1( "midi pressure ch " << ch << " v  " << v)
    MidiMsg msg(0xD0 + ch, v);
    process(msg);
    return true;
}

bool MPE_Processor::pitchbend(unsigned ch, unsigned v) {
    // LOG_1( "midi pitchbend ch " << ch << " v  " << v)
    MidiMsg msg(0xE0 + ch, v & 0x7f, (v & 0x3F80) >> 7);
    process(msg);
    return true;
}


}