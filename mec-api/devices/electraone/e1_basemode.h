#pragma once


#include "../mec_electraone.h"

#include <ElectraSchema.h>

namespace mec {

static constexpr unsigned NUI_NUM_BUTTONS = 3;

class ElectraOneBaseMode : public ElectraOneMode {
public:
    explicit ElectraOneBaseMode(ElectraOne &p);


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

    // ElectraOneDevice
    void onButton(unsigned id, unsigned value) override { buttonState_[id] = value; }

    void onEncoder(unsigned id, int value) override { ; }

    // Mode
    bool init() override { return true; }

    void poll() override;

    void activate() override { ; }

protected:
    ElectraOne &parent_;
    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    void initPreset();
    void createParam(unsigned pageid,unsigned id, const std::string& name, int val, int min, int max);
    void createDevice(unsigned id, const std::string& name, unsigned ch, unsigned port);
    void createPage(unsigned id,const std::string& name);
    void createGroup(unsigned id,const std::string& name);
    void clearPages();
    void createButton(unsigned id, unsigned row,unsigned col, const std::string& name);


    void createKey(unsigned id, const std::string& name, unsigned x,unsigned y);
    void createKeyboard();

    int popupTime_;
    bool buttonState_[3]={false,false,false};

    ElectraOnePreset::Preset preset_;
};


} //mec
