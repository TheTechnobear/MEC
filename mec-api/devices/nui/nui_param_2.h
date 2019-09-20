#pragma once

#include "nui_param_1.h"

namespace mec {


class NuiParamMode2 : public NuiParamMode1  {
public:
    explicit NuiParamMode2(Nui &p) : NuiParamMode1(p) {;}

    bool init() override { return NuiParamMode1::init(); };
    void activate() override;}

    void onEncoder(unsigned id, int value) override;

protected:
    void setCurrentPage(unsigned pageIdx, bool UI) override;
    void displayParamNum(unsigned num, const Kontrol::Parameter &p, bool local) override;

private:
    unsigned selectedParamIdx_;
};

