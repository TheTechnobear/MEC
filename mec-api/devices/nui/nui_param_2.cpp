#include "nui_param_2.h"


void NuiParamMode2::activate() {
    NuiParamMode1::activate();
}

static unsigned parameterOrder[]={1,4,2,3};
void NuiParamMode2::onEncoder(unsigned id, int value) {
    if(buttonState_[2]) {
        // change module/page functionality
        NuiParamMode1::onEncoder();
    } else {
        switch(id) {
            case 0: {
                break;
            }
            case 1: {
                auto rack = parent_.model()->getRack(parent_.currentRack());
                auto module = parent_.model()->getModule(rack, parent_.currentModule());
                auto page = parent_.model()->getPage(module, pageId_);
                auto params = parent_.model()->getParams(module,page);
                if(params) {
                    if(value) {
                        selectedParamIdx_++;
                        if(selectedParamIdx_>= params.size()) {
                            selectedParamIdx_ = 1;
                        }
                    } else {
                        selectedParamIdx_--;
                        if(selectedParamIdx_< 1)) {
                            selectedParamIdx_ = params.size();
                        }
                    }
                }
                break;
            }
            case 2: {
                changeParam(parameterOrder[selectedParamIdx_],value);
                break;
            }
            default:
            case 3: {
                break;
            }
        }
    }
}

void NuiParamMode2:: setCurrentPage(unsigned pageIdx, bool UI) {
    try {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
        //auto page = parent_.model()->getPage(module, pageId_);
        auto pages = parent_.model()->getPages(module);

        if (pageIdx_ != pageIdx) {
            if (pageIdx < pages.size()) {
                try {
                    auto page = pages[pageIdx_];
                    auto params = parent_.model()->getParams(module,page);
                    if(selectedParamIdx_> params.size()) {
                        selectedParamIdx_ = 1;
                    }
                } catch (std::out_of_range) { ;
                }
            }
        }

    } catch (std::out_of_range) { ;
    }

    return NuiParamMode1::setCurrentPage(pageIdx, UI);
}

void NuiParamMode1::displayParamNum(unsigned num, const Kontrol::Parameter &p, bool local) {
    parent_.displayParamNum(j + 1, *param, true, num == parameterOrder[selectedParamIdx_]);
}
