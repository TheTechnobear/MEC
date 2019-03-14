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
#include <thread>

namespace mec {
static const unsigned P2_NOTE_PAD_START = 36;
static const unsigned P2_NOTE_PAD_END = P2_NOTE_PAD_START + 63;

static const unsigned P2_ENCODER_CC_TEMPO = 14;
static const unsigned P2_ENCODER_CC_SWING = 15;
static const unsigned P2_ENCODER_CC_START = 71;
static const unsigned P2_ENCODER_CC_END = P2_ENCODER_CC_START + 7;
static const unsigned P2_ENCODER_CC_VOLUME = 79;

static const unsigned P2_NOTE_ENCODER_START = 0;
static const unsigned P2_NOTE_ENCODER_END = P2_NOTE_ENCODER_START + 7;


static const unsigned P2_DEV_SELECT_CC_START = 102;
static const unsigned P2_DEV_SELECT_CC_END = P2_DEV_SELECT_CC_START + 7;

static const unsigned P2_TRACK_SELECT_CC_START = 20;
static const unsigned P2_TRACK_SELECT_CC_END = P2_TRACK_SELECT_CC_START + 7;

static const unsigned P2_SETUP_CC = 30;
static const unsigned P2_USER_CC = 59;
static const unsigned P2_DEVICE_CC = 110;
static const unsigned P2_BROWSE_CC = 111;


static const unsigned P2_CURSOR_LEFT_CC = 44;
static const unsigned P2_CURSOR_RIGHT_CC = 45;
static const unsigned P2_CURSOR_UP = 46;
static const unsigned P2_CURSOR_DOWN = 47;




class P2_DisplayMode : Kontrol::KontrolCallback {
public:
    virtual void activate() { ; }

    virtual void processNoteOn(unsigned n, unsigned v) { ; }

    virtual void processNoteOff(unsigned n, unsigned v) { ; }

    virtual void processCC(unsigned cc, unsigned v) { ; }

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override { ; }

    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
              const Kontrol::Page &) override { ; }

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; };

    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override { ; }

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &,
                  const std::string&, const std::string &) override { ; };

    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override { ; }

    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; };

    void midiLearn(Kontrol::ChangeSource src, bool b) override  { ; }
    void modulationLearn(Kontrol::ChangeSource src, bool b) override { ; }
};

class P2_PadMode : Kontrol::KontrolCallback {
public:
    virtual void activate() { ; }

    virtual void processNoteOn(unsigned n, unsigned v) { ; }

    virtual void processNoteOff(unsigned n, unsigned v) { ; }

    virtual void processCC(unsigned cc, unsigned v) { ; }

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override { ; }

    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
              const Kontrol::Page &) override { ; }

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; };

    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override { ; }

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &,
                  const std::string&, const std::string &) override { ; };

    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override { ; }


    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; };


    void midiLearn(Kontrol::ChangeSource src, bool b) override  { ; }
    void modulationLearn(Kontrol::ChangeSource src, bool b) override { ; }

};

enum PushDisplayModes {
    P2D_Param,
    P2D_Module,
    P2D_Preset
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
    void rack(Kontrol::ChangeSource source, const Kontrol::Rack &rack) override;
    void module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override;
    void page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override;
    void param(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
               const Kontrol::Parameter &parameter) override;
    void changed(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                 const Kontrol::Parameter &parameter) override;

    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;

    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;


    void resource(Kontrol::ChangeSource, const Kontrol::Rack &,
                  const std::string&, const std::string &) override ;

    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; };



    void addDisplayMode(PushDisplayModes mode, std::shared_ptr<P2_DisplayMode>);
    void changeDisplayMode(PushDisplayModes);
    void addPadMode(PushPadModes mode, std::shared_ptr<P2_PadMode>);
    void changePadMode(PushPadModes);
    void processorRun();

    void currentRack(const Kontrol::EntityId id) { rackId_ = id;}
    void currentModule(const Kontrol::EntityId id);
    void currentPage(const Kontrol::EntityId id) { pageId_ = id;}

    Kontrol::EntityId currentRack()     { return rackId_;}
    Kontrol::EntityId currentModule()   { return moduleId_;}
    Kontrol::EntityId currentPage()     { return pageId_;}
    void midiLearn(bool b);
    bool midiLearn() { return midiLearnActive_;}
    void modulationLearn(bool b);
    bool modulationLearn() { return modulationLearnActive_;}

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

    Kontrol::EntityId rackId_;
    Kontrol::EntityId moduleId_;
    Kontrol::EntityId pageId_;


    bool midiLearnActive_;
    bool modulationLearnActive_;


    // push display
    std::shared_ptr<Push2API::Push2> push2Api_;

    // kontrol interface
    std::shared_ptr<Kontrol::KontrolModel> model_;

    static const unsigned int MAX_N_MIDI_MSGS = 16;
    PaUtilRingBuffer midiQueue_; // draw midi from P2
    char msgData_[sizeof(MidiMsg) * MAX_N_MIDI_MSGS];
    std::thread processor_;
};

}

