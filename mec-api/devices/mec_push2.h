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

class P2_DisplayMode : Kontrol::KontrolCallback {
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

class P2_PadMode : Kontrol::KontrolCallback {
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

enum PushDisplayModes {
    P2D_Param
};

enum PushPadModes {
    P2P_Play
};


class Push2 : public MidiDevice, public Kontrol::KontrolCallback {

public:
    Push2(ICallback &);

    virtual ~Push2();

    //MidiDevice
    bool init(void *) override;
    bool process() override;
    void deinit() override;
    RtMidiIn::RtMidiCallback getMidiCallback() override; //override

    bool midiCallback(double deltatime, std::vector<unsigned char> *message) override;

    // Kontrol::KontrolCallback
    void rack(Kontrol::ParameterSource source, const Kontrol::Rack &rack) override;
    void module(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override;
    void page(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override;
    void param(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
               const Kontrol::Parameter &parameter) override;
    void changed(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                 const Kontrol::Parameter &parameter) override;


    void addDisplayMode(PushDisplayModes mode, std::shared_ptr<P2_DisplayMode>);
    void changeDisplayMode(PushDisplayModes);
    void addPadMode(PushPadModes mode, std::shared_ptr<P2_PadMode>);
    void changePadMode(PushPadModes);
private:
    inline std::shared_ptr<P2_PadMode> currentPadMode() { return padModes_[currentPadMode_]; }
    inline std::shared_ptr<P2_DisplayMode> currentDisplayMode() { return displayModes_[currentDisplayMode_]; }

    bool processMidi(const MidiMsg &);
    void processNoteOn(unsigned n, unsigned v);
    void processNoteOff(unsigned n, unsigned v);
    void processCC(unsigned cc, unsigned v);

    PushPadModes currentPadMode_;
    std::map<unsigned, std::shared_ptr<P2_PadMode>> padModes_;
    PushDisplayModes currentDisplayMode_;
    std::map<unsigned, std::shared_ptr<P2_DisplayMode>> displayModes_;


    // push display
    std::shared_ptr<Push2API::Push2> push2Api_;

    // kontrol interface
    std::shared_ptr<Kontrol::KontrolModel> model_;

    static const unsigned int MAX_N_MIDI_MSGS = 16;
    PaUtilRingBuffer midiQueue_; // draw midi from P2
    char msgData_[sizeof(MidiMsg) * MAX_N_MIDI_MSGS];
};

}

