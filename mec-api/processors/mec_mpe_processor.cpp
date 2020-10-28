
#include "mec_mpe_processor.h"

#include "mec_log.h"

namespace mec {

static constexpr unsigned TIMBRE_CC=74;

MPE_Processor::MPE_Processor(float pbr) : Midi_Processor(1, pbr) {
    for(unsigned i=0;i<MAX_VOICE;i++) {
        VoiceData& voice = voices_[i];
        voice.startNote_ = 0;
        voice.note_ = 0;
        voice.pitchbend_ = 0;
        voice.timbre_ = 0;
        voice.pressure_ = 0;
        voice.active_ = false;
    }

}

MPE_Processor::~MPE_Processor() {
    ;
}

/////////////////////////
// ICallback interface
void MPE_Processor::touchOn(int id, float note, float x, float y, float z) {

    VoiceData& voice = voices_[id];

    unsigned ch = id + baseChannel_; // MPE starts on 2

    unsigned startNote = static_cast<unsigned>(note + 0.4999999 );
    float semis = note - float(startNote);
    unsigned pb = bipolar14bit(semis / pitchbendRange_);

    unsigned mx = bipolar14bit(x);
    unsigned my = bipolar7bit(y);
    unsigned mz = unipolar7bit(z);

    if(voice.active_) {
        LOG_1("WARN: duplicated note, starting new note on active channel " << ch << " new note: " << startNote  << " existing note " << voice.startNote_);
        noteOff(ch, voice.startNote_ , 0);
    }

    // LOG_1("MPE_Processor::touchOn");
    // LOG_1("   note : " << note  << " startNote " << startNote << " semi :" << semis << " pb: " << pb);
    // LOG_1("   x :" << x << " mx: " << mx);
    // LOG_1("   y :" << y << " my: " << my);
    // LOG_1("   z :" << z << " mz: " << mz);

    pitchbend(ch, pb);
    cc(ch, TIMBRE_CC,  my);
    noteOn(ch, startNote, mz);
    // LOG_1("note on : " << startNote);

    voice.active_ = true;
    voice.startNote_ = startNote;
    voice.note_ = startNote;
    voice.pitchbend_ = pb;
    voice.timbre_ = my;

    // start with zero z, as we use intial z of velocity
    pressure(ch, 0);
    voice.pressure_ = 0;
}

void MPE_Processor::touchContinue(int id, float note, float x, float y, float z) {

    VoiceData& voice = voices_[id];
    unsigned ch = id + baseChannel_; // MPE starts on 2
    // unsigned mx = bipolar14bit(x);
    unsigned my = bipolar7bit(y);
    unsigned mz = unipolar7bit(z);

    float semis = note - float(voice.startNote_);
    unsigned pb = bipolar14bit(semis / pitchbendRange_);

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

    unsigned ch = id + baseChannel_; // MPE starts on 2

    if(!voice.active_) {
        LOG_1("WARN: touchOff for inactive note , channel: " << ch << " note: " << voice.startNote_ << " n:" << note << " x:" << x << " y:" << y << " z: "<< z);
        return;
    }

    unsigned vel = 0; // last vel = release velocity
    pressure(ch, 0);
    noteOff(ch, voice.startNote_ , vel);
    // LOG_1("note off : " << voice.startNote_);

    voice.active_ = false;
    // voice.startNote_ = 0;
    // voice.note_ = 0;
    // voice.pitchbend_ = 0;
    // voice.timbre_ = 0;
    // voice.pressure_ = 0;//
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


}