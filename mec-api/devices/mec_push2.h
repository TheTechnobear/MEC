#pragma once

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"
#include "mec_mididevice.h"
#include <KontrolModel.h>
#include <RtMidi.h>
#include <memory>
#include <vector>
#include <map>
#include <push2lib/push2lib.h>
#include <pa_ringbuffer.h>

namespace mec {
static const unsigned P2_NOTE_PAD_START = 36;
static const unsigned P2_NOTE_PAD_END = P2_NOTE_PAD_START + 63;

static const unsigned P2_ENCODER_CC_TEMPO = 14;
static const unsigned P2_ENCODER_CC_SWING = 15;
static const unsigned P2_ENCODER_CC_START = 71;
static const unsigned P2_ENCODER_CC_END = P2_ENCODER_CC_START + 7;
static const unsigned P2_ENCODER_CC_VOLUME = 79;

static const unsigned P2_DEV_SELECT_CC_START = 102;
static const unsigned P2_DEV_SELECT_CC_END = P2_DEV_SELECT_CC_START + 7;

static const unsigned P2_TRACK_SELECT_CC_START = 20;
static const unsigned P2_TRACK_SELECT_CC_END = P2_DEV_SELECT_CC_START + 7;

class P2_Mode : Kontrol::KontrolCallback {
public:
    virtual void activate() { ; }

    virtual void processNoteOn(unsigned n, unsigned v) { ; }

    virtual void processNoteOff(unsigned n, unsigned v) { ; }

    virtual void processCC(unsigned cc, unsigned v) { ; }

    virtual void rack(Kontrol::ParameterSource, const Kontrol::Rack &) override { ; }

    virtual void module(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &) override { ; }

    virtual void page(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                      const Kontrol::Page &) override { ; }

    virtual void param(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                       const Kontrol::Parameter &) override { ; };

    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                         const Kontrol::Parameter &) override { ; }
};

class Push2 : public MidiDevice {

public:
    Push2(ICallback &);

    virtual ~Push2();

    //mididevice : device
    virtual bool init(void *);

    virtual bool process();

    virtual void deinit();

    virtual RtMidiIn::RtMidiCallback getMidiCallback(); //override

    virtual bool midiCallback(double deltatime, std::vector<unsigned char> *message);

    void changeMode(unsigned);

    void addMode(unsigned mode, std::shared_ptr<P2_Mode>);

private:

    static const unsigned int MAX_N_MIDI_MSGS = 16;

    bool processMidi(const MidiMsg &);

    void processNoteOn(unsigned n, unsigned v);

    void processNoteOff(unsigned n, unsigned v);

    void processCC(unsigned cc, unsigned v);

    // push display
    std::shared_ptr<Push2API::Push2> push2Api_;

    // kontrol interface
    std::shared_ptr<Kontrol::KontrolModel> model_;


//        float pitchbendRange_;
    PaUtilRingBuffer midiQueue_; // draw midi from P2
    char msgData_[sizeof(MidiMsg) * MAX_N_MIDI_MSGS];


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

    std::map<unsigned, std::shared_ptr<P2_Mode>> modes_;
    unsigned currentMode_;

    inline std::shared_ptr<P2_Mode> currentMode() { return modes_[currentMode_]; }
};

}

