#include "Bela.h"


enum BelaModes {
    BELA_MAIN
};



class BelaBaseMode : public DeviceMode {
public:
    BelaBaseMode(Bela &p) : parent_(p) { ; }

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
    Bela &parent_;
    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }
};


class BelaMainMode : public BelaBaseMode {
public:
    BelaMainMode(Bela &p) : BelaBaseMode(p){ ; }
};


Bela::Bela() {

}

Bela::~Bela() {

}

bool Bela::init() {
    if( KontrolDevice::init() ) {
	    addMode(BELA_MAIN, std::make_shared<BelaMainMode>(*this));
	    changeMode(BELA_MAIN);
    }
    return true;
}


#if 0
void Bela::digital(unsigned bus, bool value) {
    // for now, use a digital bus 1 and 2 for up n down preset
    if (bus == 1 || bus == 2) {
        static bool last[2] = {false, false};
        if (value && !last[bus - 1]) {
            last[bus - 1] = true;
            auto rack = model()->getRack(currentRack());
            if (rack != nullptr) {
                auto presets = rack->getPresetList();
                auto cur = rack->currentPreset();
                int idx = 0;
                int found = false;
                for (auto p : presets) {
                    if (p == cur) {
                        found = true;
                    }
                    if (!found) idx++;
                }
                if (found) {
                    if (bus == 2) {
                        idx++;
                        if (idx >= presets.size()) idx = 0;
                    } else {
                        idx--;
                        if (idx < 0) idx = presets.size() - 1;
                        if (idx < 0) idx = 0;
                    }
                } else {
                    idx = 0;
                }
                rack->applyPreset(presets[idx]);
            }
        } else {
            last[bus - 1] = false;
        }


    }
    KontrolDevice::digital(bus, value);
    return;
}
#endif
