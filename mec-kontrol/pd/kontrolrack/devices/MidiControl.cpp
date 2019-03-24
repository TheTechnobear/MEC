#include "MidiControl.h"


enum MidiControlModes {
    MIDI_MAIN
};



class MidiControlBaseMode : public DeviceMode {
public:
    MidiControlBaseMode(MidiControl &p) : parent_(p) { ; }

    bool init() override { return true; }

    void poll() override { ; }
    void activate() override { ; }

    void changePot(unsigned, float) override { ; }

    void changeEncoder(unsigned, float) override { ; }

    void encoderButton(unsigned, bool) override { ; }

    void keyPress(unsigned, unsigned) override { ; }

    void selectPage(unsigned) override { ; }

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override { ; }

    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
              const Kontrol::Page &) override { ; }

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; }

    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override { ; }

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &,
                  const std::string &) override { ; };

    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

protected:
    MidiControl &parent_;
    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }
};


class MidiControlMainMode : public MidiControlBaseMode {
public:
    MidiControlMainMode(MidiControl &p) : MidiControlBaseMode(p){ ; }
};


MidiControl::MidiControl() {

}

MidiControl::~MidiControl() {

}

bool MidiControl::init() {
    if( KontrolDevice::init() ) {
	    addMode(MIDI_MAIN, std::make_shared<MidiControlMainMode>(*this));
	    changeMode(MIDI_MAIN);
    }
    return true;
}

