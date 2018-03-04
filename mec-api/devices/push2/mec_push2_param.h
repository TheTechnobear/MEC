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

    void setCurrentPage(int page);
    void setCurrentModule(int mod);

private:
    void displayPage();

    void drawParam(unsigned pos, const Kontrol::Parameter &param);

    Push2 &parent_;
    std::shared_ptr<Push2API::Push2> push2Api_;
    std::shared_ptr<Kontrol::KontrolModel> model_;

    int moduleIdx_ = -1;
    Kontrol::EntityId moduleId_;
    int pageIdx_ = -1;
    Kontrol::EntityId pageId_;
    Kontrol::EntityId rackId_;

};

}
