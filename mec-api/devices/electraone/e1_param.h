#pragma once

#include "e1_param_1.h"

namespace mec {


class ElectraOneParamMode : public ElectraOneParamMode1 {
public:
    explicit ElectraOneParamMode(ElectraOne &p) : ElectraOneParamMode1(p) { ; }

    bool init() override;

    void onEncoder(unsigned id, int value) override;

protected:
    void setCurrentPage(unsigned pageIdx, bool UI) override;
    void displayParamNum(unsigned num, const Kontrol::Parameter &p, bool local) override;

private:
    unsigned selectedParamIdx_ = 0;
};

} //mec
