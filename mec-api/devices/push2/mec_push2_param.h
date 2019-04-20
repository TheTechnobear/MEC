#pragma once

#include "../mec_push2.h"

namespace mec {

class P2_ParamMode : public P2_DisplayMode {
public:
    P2_ParamMode(mec::Push2 &parent, const std::shared_ptr<Push2API::Push2> &api);

    void processNoteOn(unsigned n, unsigned v) override;

    void processNoteOff(unsigned n, unsigned v) override;

    void processCC(unsigned cc, unsigned v) override;

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override;

    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;

    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Page &) override;

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;

    void changed(Kontrol::ChangeSource src, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;

    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;

    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string&, const std::string &) override;

    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;

    void setCurrentPage(int page);
    void setCurrentModule(int mod);
    void activate() override;
private:
    void displayPage();

    std::vector<std::shared_ptr<Kontrol::Module>> getModules(const std::shared_ptr<Kontrol::Rack>& rack);

    void drawParam(unsigned pos, const Kontrol::Parameter &param);

    Push2 &parent_;
    std::shared_ptr<Push2API::Push2> push2Api_;
    std::shared_ptr<Kontrol::KontrolModel> model_;

    int moduleIdx_ = -1;
    std::string moduleType_;
    int moduleIdxOffset_ = 0;
    int pageIdx_ = -1;
    int pageIdxOffset_ = 0;
    std::vector<std::string> moduleOrder_;
};

}
