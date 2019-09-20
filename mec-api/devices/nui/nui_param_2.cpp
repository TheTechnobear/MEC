#include "nui_param_2.h"

#include <iostream>

namespace mec {

static unsigned parameterOrder[]={1,4,2,3};

bool NuiParamMode2::init() {
	selectedParamIdx_=0;
	return NuiParamMode1::init();
}

void NuiParamMode2::onEncoder(unsigned id, int value) {
    if(buttonState_[2]) {
        // change module/page functionality
        NuiParamMode1::onEncoder(id,value);
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
	        std::cerr << "s idx 1 << " << selectedParamIdx_ <<  "  : " << params.size() << " , " << value << std::endl;
                if(value>0) {
                     selectedParamIdx_++;
                     if(selectedParamIdx_>= params.size() || selectedParamIdx_>3) {
                         selectedParamIdx_ = 0;
                     }
                } else {
                     if(selectedParamIdx_ > 0){
                     	selectedParamIdx_--;
		     } else {
                         selectedParamIdx_ = params.size()-1;
                     }
                }
	        std::cerr << "s idx 2 << " << selectedParamIdx_ <<  "  : " << params.size() << " , " << value << std::endl;
		display();
                break;
            }
            case 2: {
                changeParam(parameterOrder[selectedParamIdx_]-1,value);
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

	if(module==nullptr) return;
        auto pages = parent_.model()->getPages(module);

        if (pageIdx_ != pageIdx) {
            if (pageIdx < pages.size()) {
                try {
                    auto page = pages[pageIdx];
		    if(page==nullptr) return; 

                    auto params = parent_.model()->getParams(module,page);
                    if(selectedParamIdx_>= params.size() || selectedParamIdx_>3) {
                        selectedParamIdx_ = 0;
                    }
                } catch (std::out_of_range) { ;
                }
            }
        }

    } catch (std::out_of_range) { ;
    }

    // dont use pageIdx_ prior to this, since it might be -1
    return NuiParamMode1::setCurrentPage(pageIdx, UI);
}

void NuiParamMode2::displayParamNum(unsigned num, const Kontrol::Parameter &p, bool local) {
    parent_.displayParamNum(num,p,local, num == parameterOrder[selectedParamIdx_]);
}


} // mec
