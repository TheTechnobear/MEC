#pragma once


#include "../mec_electraone.h"

#include <ElectraSchema.h>

namespace mec {

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


    void publishStart(Kontrol::ChangeSource, unsigned numRacks)  override { ; }
    void publishRackFinished(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }


    // ElectraOneDevice
    void onButton(unsigned id, unsigned value) override { ; }

    void onEncoder(unsigned id, int value) override { ; }

    // Mode
    bool init() override { return true; }

    void poll() override;

    void activate() override { ; }

protected:

    enum {
        E1_BTN_FIRST_MODULE = 80,
        E1_BTN_AUX = 100,
        E1_BTN_PREV_MODULE = 101,
        E1_BTN_NEXT_MODULE = 102,
        E1_BTN_NEW_PRESET = 103,
        E1_BTN_LOAD_PRESET = 104,
        E1_BTN_SAVE_PRESET = 105,
        E1_BTN_SAVE = 106,
        E1_BTN_LOAD_MODULE = 107,
        E1_BTN_MIDI_LEARN = 108,
        E1_BTN_MOD_LEARN = 109,
        E1_CTL_PRESET_LIST = 110,
        E1_CTL_MOD_LIST = 111
    };

    ElectraOne &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    void initPreset();
    unsigned createParam(unsigned pageid, unsigned ctrlsetid, unsigned kpageid, unsigned pos, unsigned pid,
                         const std::string &name, int val, int min, int max);
    void createDevice(unsigned id, const std::string &name, unsigned ch, unsigned port);
    void createPage(unsigned id, const std::string &name);
    void createGroup(unsigned pageid, unsigned ctrlsetid, unsigned kpageid, const std::string &name);
    void clearPages();
    void createButton(unsigned id, unsigned pageid, unsigned ctrlsetid, unsigned row, unsigned col, const std::string &name, bool toggle=false, bool tstate=false);
    void createList(unsigned id, unsigned pageid, unsigned ctrlsetid,
                    unsigned row, unsigned col, unsigned pid, const std::string &name,
                    std::set<std::string> &list, const std::string &select);


//    void createKey(unsigned id, unsigned pageid,const std::string& name, unsigned x,unsigned y);
//    void createKeyboard(unsigned pageid);

    unsigned lastId_ = 1;
    unsigned lastOverlayId_ = 1;
    ElectraOnePreset::Preset preset_;
};


} //mec
