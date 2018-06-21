
#include "mec_midi_processor.h"

//#include "mec_log.h"

namespace mec {

Midi_Processor::Midi_Processor(unsigned baseCh, float pbr) : baseChannel_(baseCh), pitchbendRange_ (pbr) {
    ;
}

Midi_Processor::~Midi_Processor() {
    ;
}

void Midi_Processor::setPitchbendRange(float v) {
    pitchbendRange_ = v;
}

/////////////////////////
// ICallback interface
void Midi_Processor::touchOn(int id, float note, float , float , float z) {
    unsigned ch = baseChannel_;
    unsigned mz = unipolar7bit(z);
    noteOn(ch, (unsigned) note, mz);
}

void Midi_Processor::touchContinue(int, float, float , float , float ) {
    //TODO  : perhaps use for poly aftertouch?
    ; // ignore
}

void Midi_Processor::touchOff(int id, float note, float , float , float z) {
    unsigned ch = baseChannel_;
    unsigned mz = unipolar7bit(z);
    noteOff(ch, (unsigned) note, mz);
}

void Midi_Processor::control(int attr, float v) {

    if (global_[attr] != v ) {
        global_[attr] = v;
        cc(baseChannel_, static_cast<unsigned int>(attr), unipolar7bit(v));
        // cc(ch, attr, isBipolar ? bipolar7bit(v) : unipolar7bit(v));
    }
}

void Midi_Processor::mec_control(int , void* ) {
    // ignored
    ;
}


bool Midi_Processor::noteOn(unsigned ch, unsigned note, unsigned vel) {
    // LOG_1( "midi note on ch " << ch << " note " << note  << " vel " << vel );
    MidiMsg msg(static_cast<char>(0x90 + ch), static_cast<char>(note), static_cast<char>(vel));
    process(msg);
    return true;
}


bool Midi_Processor::noteOff(unsigned ch, unsigned note, unsigned vel) {
    // LOG_1( "midi  note off ch " << ch << " note " << note  << " vel " << vel )
    MidiMsg msg(static_cast<char>(0x80 + ch), static_cast<char>(note), static_cast<char>(vel));
    process(msg);
    return true;
}

bool Midi_Processor::cc(unsigned ch, unsigned cc, unsigned v) {
    // LOG_1( "midi note off ch " << ch << " note " << note  << " vel " << vel )
    MidiMsg msg(static_cast<char>(0xB0 + ch), static_cast<char>(cc), static_cast<char>(v));
    process(msg);
    return true;
}

bool Midi_Processor::pressure(unsigned ch, unsigned v) {
    // LOG_1( "midi pressure ch " << ch << " v  " << v)
    MidiMsg msg(static_cast<char>(0xD0 + ch),static_cast<char>(v));
    process(msg);
    return true;
}

bool Midi_Processor::pitchbend(unsigned ch, unsigned v) {
    // LOG_1( "midi pitchbend ch " << ch << " v  " << v)
    MidiMsg msg(static_cast<char>(0xE0 + ch), static_cast<char>(v & 0x7f), static_cast<char>((v & 0x3F80) >> 7));
    process(msg);
    return true;
}


}