#pragma once

#include "../mec_push2.h"

namespace mec {

class P2_ParamMode : public P2_DisplayMode {
public:
    P2_ParamMode(mec::Push2 &parent, const std::__1::shared_ptr<Push2API::Push2> &api);

    void processNoteOn(unsigned n, unsigned v) override;

    void processNoteOff(unsigned n, unsigned v) override;

    void processCC(unsigned cc, unsigned v) override;

    void rack(Kontrol::ParameterSource, const Kontrol::Rack &) override;

    void module(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &) override;

    void page(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Page &) override;

    void param(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;

    void changed(Kontrol::ParameterSource src, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;

    void setCurrentPage(int page);

private:
    void displayPage();

    void drawParam(unsigned pos, const Kontrol::Parameter &param);

    Push2 &parent_;
    std::shared_ptr<Push2API::Push2> push2Api_;
    std::shared_ptr<Kontrol::KontrolModel> model_;
    int currentModule_ = -1;
    int currentPage_ = -1;

    std::shared_ptr<Kontrol::Rack> rack_;
    std::vector<std::shared_ptr<Kontrol::Module>> modules_;
    std::vector<std::shared_ptr<Kontrol::Page>> pages_;
    std::vector<std::shared_ptr<Kontrol::Parameter>> params_;
};

}