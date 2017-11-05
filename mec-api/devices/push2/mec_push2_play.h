#pragma once

#include "../mec_push2.h"

namespace mec {

class P2_PlayMode : public P2_PadMode {
public:
    P2_PlayMode(mec::Push2 &parent, const std::__1::shared_ptr<Push2API::Push2> &api);

    void processNoteOn(unsigned n, unsigned v) override;

    void processNoteOff(unsigned n, unsigned v) override;

    void processCC(unsigned cc, unsigned v) override;
    void activate() override;

    void rack(Kontrol::ParameterSource, const Kontrol::Rack &) override {;}

    void module(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &) override {;};

    void page(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Page &) override {;};

    void param(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override {;};

    void changed(Kontrol::ParameterSource src, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override {;};


private:
    Push2 &parent_;
    std::shared_ptr<Push2API::Push2> push2Api_;

//        float pitchbendRange_;
    // scales colour
    void updatePadColours();
    unsigned determinePadScaleColour(int8_t r, int8_t c);

    uint8_t octave_;    // current octave
    uint8_t scaleIdx_;
    uint16_t scale_;     // current scale
    uint8_t numNotesInScale_;   // number of notes in current scale
    uint8_t tonic_;     // current tonic
    uint8_t rowOffset_; // current tonic
    bool chromatic_; // are we in chromatic mode or not

};

}