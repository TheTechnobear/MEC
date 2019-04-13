#include "mec_push2_module.h"


#include <mec_log.h>
#include <algorithm>

namespace mec {

#define MAX_ROW 5
#define MAX_COL 7

P2_ModuleMode::P2_ModuleMode(mec::Push2 &parent, const std::shared_ptr<Push2API::Push2> &api)
        : parent_(parent),
          push2Api_(api),
          selectedModuleIdx_(0),
          moduleOffset_(0) {
    model_ = Kontrol::KontrolModel::model();
}

static int encoder1Step_ = 0;
static int encoder2Step_ = 0;

void P2_ModuleMode::processNoteOn(unsigned note, unsigned) {
    switch (note) {
        case P2_NOTE_ENCODER_START :
            encoder1Step_ = 0;
            break;
        case P2_NOTE_ENCODER_START + 1:
            encoder2Step_ = 0;
            break;
        default:
            break;

    }
}

void P2_ModuleMode::processNoteOff(unsigned note, unsigned) {
    switch (note) {
        case P2_NOTE_ENCODER_START :
            encoder1Step_ = 0;
            break;
        case P2_NOTE_ENCODER_START + 1:
            encoder2Step_ = 0;
            break;
        default:
            break;

    }
}


void P2_ModuleMode::processCC(unsigned cc, unsigned v) {

    switch (cc) {
        case P2_TRACK_SELECT_CC_END: {
            if (v > 0) {
                auto pRack = model_->getRack(parent_.currentRack());
                auto pModule = model_->getModule(pRack, parent_.currentModule());
                if (pModule == nullptr) return;


                std::string selcat, selmod;
                unsigned idx=0;
                for(auto cat: categories_) {
                    if(selectedCatIdx_==idx) {
                        selcat = cat;
                        break;
                    }
                    idx++;
                }
                idx=0;
                for(auto mod: modules_) {
                    if(selectedModuleIdx_==idx) {
                        selmod = mod;
                        break;
                    }
                    idx++;
                }

                std::string modtype = selcat+selmod;
                LOG_0("selected module " << modtype);
                parent_.changeDisplayMode(P2D_Param);
                model_->loadModule(Kontrol::CS_LOCAL, pRack->id(), pModule->id(), modtype);
                return;
            }
            break;
        }

        case P2_ENCODER_CC_START: {
            auto pRack = model_->getRack(parent_.currentRack());
            if (pRack == nullptr) return;

            float value = v & 0x40 ? (128.0f - (float) v) * -1.0 : float(v);
            if (value > 0 && selectedCatIdx_ == (categories_.size()-1)) return;
            if (value < 0 && selectedCatIdx_ == 0) return;
            encoder1Step_ += value;
            bool chg = false;
            if (encoder1Step_ > 10) {
                selectedCatIdx_++;
                if(selectedCatIdx_- catOffset_ > (MAX_ROW-1)) {
                    catOffset_ ++;
                }
                chg = true;

            } else if (encoder1Step_ < -10.0) {
                selectedCatIdx_--;
                if(selectedCatIdx_< catOffset_) {
                    catOffset_ = selectedCatIdx_;
                }
                chg = true;
            }
            if(chg) {
                std::string selcat;
                unsigned idx=0;
                for(auto cat: categories_) {
                    if(selectedCatIdx_==idx) {
                        selcat = cat;
                        break;
                    }
                    idx++;
                }
                selectedModuleIdx_ = 0;
                moduleOffset_ = 0;
                unsigned catlen = selcat.length();
                auto res = pRack->getResources("module");
                modules_.clear();
                for (const auto &modtype : res) {
                    size_t pos = modtype.find("/");
                    auto cat = modtype.substr(0, pos + 1);
                    if(selcat == cat) {
                        std::string mod=modtype.substr(catlen,modtype.length()-catlen);
                        modules_.insert(mod);
                    }
                }
                encoder1Step_ = 0;
                displayPage();
            }

            break;
        }
        case P2_ENCODER_CC_START + 1: {
            auto pRack = model_->getRack(parent_.currentRack());
            if (pRack == nullptr) return;

            float value = v & 0x40 ? (128.0f - (float) v) * -1.0 : float(v);
            if (value > 0 && selectedModuleIdx_ == (modules_.size() - 1)) return;
            if (value < 0 && selectedModuleIdx_ == 0) return;

            bool chg = false;
            encoder2Step_ += value;
            if (encoder2Step_ > 10) {
                selectedModuleIdx_++;
                chg = true;
            } else if (encoder2Step_ < -10.0) {
                selectedModuleIdx_--;
                chg = true;
            }
            if(chg) {
                encoder2Step_ = 0;
                displayPage();

            }
            break;
        }
        case P2_CURSOR_LEFT_CC : {
            if(!v) return;
            auto pRack = model_->getRack(parent_.currentRack());
            if (pRack == nullptr) return;
            int offset = moduleOffset_ - (MAX_COL * MAX_ROW);
            offset = std::max(offset,0);
            if (offset != moduleOffset_) {
                moduleOffset_ = offset;
                displayPage();
            }
            break;
        }
        case P2_CURSOR_RIGHT_CC : {
            if(!v) return;
            auto pRack = model_->getRack(parent_.currentRack());
            if (pRack == nullptr) return;
            int offset = moduleOffset_ + (MAX_COL * MAX_ROW);
            offset = std::min(offset, (int) (pRack->getResources("module").size() - 1) );
            if (offset != moduleOffset_) {
                moduleOffset_ = offset;
                displayPage();
            }
            break;
        }
    }
}


void P2_ModuleMode::displayPage() {

    auto rack = model_->getRack(parent_.currentRack());
    auto module = model_->getModule(rack, parent_.currentModule());
    if (module == nullptr) return;

    Push2API::Push2::Colour catclr(0xFF, 0, 0xFF);
    Push2API::Push2::Colour clr(0xFF, 0xFF, 0xFF);

    push2Api_->clearDisplay();

    push2Api_->drawCell8(MAX_ROW, MAX_COL, "    Select", clr);


    int x = 0, y = 0;
    int idx = 0;

    auto citer = categories_.begin();
    idx = 0;
    for (int i = 0; i <catOffset_ && citer != categories_.end(); i++) {
        citer++;
        idx++;
    }
    for(int i=0;i< MAX_ROW;i++) {
        if(citer != categories_.end()) {
            if(idx==selectedCatIdx_)  {
                push2Api_->drawInvertedCell8(i, y, citer->c_str(), catclr);
            } else {
                push2Api_->drawCell8(i, y, citer->c_str(), catclr);
            }
            citer++;
            idx++;
        }
    }


    auto iter = modules_.begin();
    idx = 0;
    for (int i = 0; i < moduleOffset_ && iter != modules_.end(); i++) {
        iter++;
        idx++;
    }

    y = 1; x= 0;
    if (iter != modules_.end()) {
        for (; iter != modules_.end(); iter++) {
            if (idx == selectedModuleIdx_) {
                push2Api_->drawInvertedCell8(x, y, iter->c_str(), clr);
            } else {
                push2Api_->drawCell8(x, y, iter->c_str(), clr);
            }

            x++;
            if (x >= MAX_ROW) {
                x = 0;
                y++;
                if (y > MAX_COL) return;
            }
            idx++;
        }
    }

}

void P2_ModuleMode::activate() {
    P2_DisplayMode::activate();
    auto pRack = model_->getRack(parent_.currentRack());
    auto pModule = model_->getModule(pRack, parent_.currentModule());
    if (pModule == nullptr) return;

    moduleOffset_ = 0;
    selectedModuleIdx_ = 0;
    selectedCatIdx_ = 0;
    catOffset_ = 0;
    modules_.clear();
    categories_.clear();

    auto res = pRack->getResources("module");

    std::string selcat;
    size_t pos =std::string::npos;
    pos = pModule->type().find("/");
    if(pos!=std::string::npos) {
        selcat=pModule->type().substr(0,pos+1);
    }
    unsigned catlen = selcat.length();

    unsigned idx = 0;
    for (const auto &modtype : res) {
        size_t pos = modtype.find("/");
        auto cat = modtype.substr(0, pos + 1);
        categories_.insert(cat);
        if(selcat == cat) {
            if(modtype == pModule->type()) {
                selectedModuleIdx_ = idx;
                if (selectedModuleIdx_ > ((MAX_COL-1) * MAX_ROW) -1) {
                    moduleOffset_ = selectedModuleIdx_ - ((MAX_COL-1) * MAX_ROW ) - 1;
                } else {
                    moduleOffset_ = 0;
                }
            }
            std::string mod=modtype.substr(catlen,modtype.length()-catlen);
            modules_.insert(mod);
            idx++;
        }
    }

    idx = 0;
    for(const auto cat: categories_) {
        if(cat == selcat) {
            selectedCatIdx_ = idx;
            if(selectedCatIdx_ > (MAX_ROW-1)) {
                catOffset_ = selectedCatIdx_ - (MAX_ROW-1);
            }
            break;
        }
        idx++;
    }


    for (int i = P2_DEV_SELECT_CC_START; i <= P2_DEV_SELECT_CC_END; i++) {
        parent_.sendCC(0, i, 0);
    }
    for (int i = P2_TRACK_SELECT_CC_START; i <= P2_TRACK_SELECT_CC_END; i++) {
        parent_.sendCC(0, i, 0);
    }
    parent_.sendCC(0, P2_TRACK_SELECT_CC_END, 0x7f);
    parent_.sendCC(0, P2_CURSOR_LEFT_CC, 0x7f);
    parent_.sendCC(0, P2_CURSOR_RIGHT_CC, 0x7f);
    parent_.sendCC(0, P2_SETUP_CC,0x00);
    parent_.sendCC(0, P2_AUTOMATE_CC,0x00);
    displayPage();
}
} //namespace
