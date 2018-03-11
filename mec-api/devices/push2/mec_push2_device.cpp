#include "mec_push2_device.h"


#include <mec_log.h>

namespace mec {
static const unsigned VSCALE = 3;
static const unsigned HSCALE = 1;

P2_DeviceMode::P2_DeviceMode(mec::Push2 &parent, const std::shared_ptr<Push2API::Push2> &api)
        : parent_(parent),
          push2Api_(api),
          selectedIdx_(0) {
    model_ = Kontrol::KontrolModel::model();
}

static int encoderStep_ = 0;

void P2_DeviceMode::processNoteOn(unsigned note, unsigned) {
    switch (note) {
        case P2_NOTE_ENCODER_START :
            encoderStep_ = 0;
            break;
        default:
            break;

    }
}

void P2_DeviceMode::processNoteOff(unsigned note, unsigned) {
    switch (note) {
        case P2_NOTE_ENCODER_START :
            encoderStep_ = 0;
            break;
        default:
            break;
    }
}


void P2_DeviceMode::processCC(unsigned cc, unsigned v) {

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
                        model_->loadModule(Kontrol::CS_LOCAL, pRack->id(), pModule->id(), modtype);
                        parent_.changeDisplayMode(P2D_Param);
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
        }
    }
}


void P2_DeviceMode::displayPage() {
    int16_t clr = RGB565(0xFF, 0xFF, 0xFF);

    push2Api_->clearDisplay();

    push2Api_->drawCell8(5, 7, "    Select", VSCALE, HSCALE, clr);

    // we need a current rack and also current module
    auto racks = model_->getRacks();
    if (racks.size() == 0) return;
    auto rack = racks[0];
    if (rack == nullptr) return;

    int x = 0, y = 0;
    int idx = 0;
    for (auto modtype : rack->getResources("module")) {
        if (idx == selectedIdx_) {
            push2Api_->drawInvertedCell8(x, y, modtype.c_str(), VSCALE, HSCALE, clr);
        } else {
            push2Api_->drawCell8(x, y, modtype.c_str(), VSCALE, HSCALE, clr);
        }
        x++;
        if (x > 5) {
            x = 0;
            y++;
            if (y > 8) return;
        }
        idx++;
    }
}

void P2_DeviceMode::activate() {
    P2_DisplayMode::activate();
    auto pRack = model_->getRack(parent_.currentRack());
    auto pModule = model_->getModule(pRack, parent_.currentModule());
    if (pModule == nullptr) return;
    int idx = 0;
    for (auto modtype : pRack->getResources("module")) {
        if (modtype == pModule->type()) {
            selectedIdx_ = idx;
        }
        idx++;
    }


    for (int i = P2_DEV_SELECT_CC_START; i <= P2_DEV_SELECT_CC_END; i++) {
        parent_.sendCC(0, i, 0);
    }
    for (int i = P2_TRACK_SELECT_CC_START; i <= P2_TRACK_SELECT_CC_END; i++) {
        parent_.sendCC(0, i, 0);
    }
    parent_.sendCC(0, P2_TRACK_SELECT_CC_END, 255);
    displayPage();
}

} //namespace
