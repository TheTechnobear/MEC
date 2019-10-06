#pragma once


#include "../mec_nui.h"

namespace mec {

static constexpr unsigned NUI_NUM_BUTTONS = 3;

class NuiBaseMode : public NuiMode {
public:
    explicit NuiBaseMode(Nui &p) : parent_(p), popupTime_(-1) { ; }


    // Kontrol
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

    // NuiDevice
    void onButton(unsigned id, unsigned value) override { buttonState_[id] = value; }

    void onEncoder(unsigned id, int value) override { ; }

    // Mode
    bool init() override { return true; }

    void poll() override;

    void activate() override { ; }

protected:
    Nui &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    int popupTime_;
    bool buttonState_[NUI_NUM_BUTTONS] = {false, false, false};
};


} //mec
