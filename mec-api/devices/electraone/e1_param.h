#pragma once


#include "e1_basemode.h"


namespace mec {


class ElectraOneParamMode : public ElectraOneBaseMode {
public:
    explicit ElectraOneParamMode(ElectraOne &p) : ElectraOneBaseMode(p), pageIdx_(-1) { ; }

    bool init() override { return true; };
    void activate() override;

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override;
    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override;
    void page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override;

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; }

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &, const std::string &) override { ; }

    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::EntityId &,
                    const std::string &) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;


    void onButton(unsigned id, unsigned value) override;
    void onEncoder(unsigned id, int value) override;

protected:
    virtual void setCurrentPage(unsigned pageIdx, bool UI);
    virtual void displayParamNum(unsigned pageid, unsigned ctrlsetid, unsigned kpageid, unsigned pos,
                                 unsigned pid,const Kontrol::Parameter &p, bool local);

    void changeParam(unsigned idx, int relValue);
    void display();

    std::string moduleType_;
    int pageIdx_ = -1;
    Kontrol::EntityId pageId_;
};

} //mec
