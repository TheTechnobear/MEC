#include "mec_push2_module.h"


#include <mec_log.h>
#include <algorithm>

namespace mec {

#define MAX_ROW 5
#define MAX_COL 7

P2_ModuleMode::P2_ModuleMode(mec::Push2 &parent, const std::shared_ptr<Push2API::Push2> &api)
        : parent_(parent),
          push2Api_(api),
          selectedIdx_(0),
          pageOffset_(0) {
    model_ = Kontrol::KontrolModel::model();
}

static int encoderStep_ = 0;

void P2_ModuleMode::processNoteOn(unsigned note, unsigned) {
    switch (note) {
        case P2_NOTE_ENCODER_START :
            encoderStep_ = 0;
            break;
        default:
            break;

    }
}

void P2_ModuleMode::processNoteOff(unsigned note, unsigned) {
    switch (note) {
        case P2_NOTE_ENCODER_START :
            encoderStep_ = 0;
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
                int idx = 0;
                for (auto modtype : pRack->getResources("module")) {
                    if (idx == selectedIdx_) {
                        LOG_0("selected module " << modtype);
                        parent_.changeDisplayMode(P2D_Param);
                        model_->loadModule(Kontrol::CS_LOCAL, pRack->id(), pModule->id(), modtype);
                        return;
                    }
                    idx++;
                }
            }
            break;
        }

        case P2_ENCODER_CC_START: {
            auto pRack = model_->getRack(parent_.currentRack());
            if (pRack == nullptr) return;

            float value = v & 0x40 ? (128.0f - (float) v) * -1.0 : float(v);
            if (value > 0 && selectedIdx_ == (pRack->getResources("module").size() - 1)) return;
            if (value < 0 && selectedIdx_ == 0) return;

            encoderStep_ += value;
            if (encoderStep_ > 10) {
                selectedIdx_++;
                encoderStep_ = 0;
                displayPage();
            } else if (encoderStep_ < -10.0) {
                selectedIdx_--;
                encoderStep_ = 0;
                displayPage();
            }
            break;
        }
        case P2_CURSOR_LEFT_CC : {
            if(!v) return;
            auto pRack = model_->getRack(parent_.currentRack());
            if (pRack == nullptr) return;
            int offset = pageOffset_ - (MAX_COL * MAX_ROW);
            offset = std::max(offset,0);
            if (offset != pageOffset_) {
                pageOffset_ = offset;
                displayPage();
            }
            break;
        }
        case P2_CURSOR_RIGHT_CC : {
            if(!v) return;
            auto pRack = model_->getRack(parent_.currentRack());
            if (pRack == nullptr) return;
            int offset = pageOffset_ + (MAX_COL * MAX_ROW);
            offset = std::min(offset, (int) (pRack->getResources("module").size() - 1) );
            if (offset != pageOffset_) {
                pageOffset_ = offset;
                displayPage();
            }
            break;
        }
    }
}


void P2_ModuleMode::displayPage() {
    Push2API::Push2::Colour clr(0xFF, 0xFF, 0xFF);

    push2Api_->clearDisplay();

    push2Api_->drawCell8(MAX_ROW, MAX_COL, "    Select", clr);

    // we need a current rack and also current module
    auto racks = model_->getRacks();
    if (racks.size() == 0) return;
    auto rack = racks[0];
    if (rack == nullptr) return;

    int x = 0, y = 0;
    int idx = 0;
    auto modules = rack->getResources("module");
    auto iter = modules.begin();
    for (int i = 0; i < pageOffset_ && iter != modules.end(); i++) {
        iter++;
        idx++;
    }

    if (iter != modules.end()) {
        for (; iter != modules.end(); iter++) {
            if (idx == selectedIdx_) {
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
    int idx = 0;
    for (auto modtype : pRack->getResources("module")) {
        if (modtype == pModule->type()) {
            selectedIdx_ = idx;
            if (selectedIdx_ > (MAX_COL * MAX_ROW)) {
                pageOffset_ = selectedIdx_;
            } else {
                pageOffset_ = 0;
            }
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
    displayPage();
}
} //namespace
