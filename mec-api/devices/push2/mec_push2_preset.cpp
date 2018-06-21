#include "mec_push2_preset.h"


#include <mec_log.h>
#include <algorithm>

namespace mec {

#define MAX_ROW 5
#define MAX_COL 7

P2_PresetMode::P2_PresetMode(mec::Push2 &parent, const std::shared_ptr<Push2API::Push2> &api)
        : parent_(parent),
          push2Api_(api),
          selectedIdx_(0),
          pageOffset_(0) {
    model_ = Kontrol::KontrolModel::model();
}

static int encoderStep_ = 0;

void P2_PresetMode::processNoteOn(unsigned note, unsigned) {
    switch (note) {
        case P2_NOTE_ENCODER_START :
            encoderStep_ = 0;
            break;
        default:
            break;

    }
}

void P2_PresetMode::processNoteOff(unsigned note, unsigned) {
    switch (note) {
        case P2_NOTE_ENCODER_START :
            encoderStep_ = 0;
            break;
        default:
            break;
    }
}


void P2_PresetMode::processCC(unsigned cc, unsigned v) {

    switch (cc) {
        case (P2_TRACK_SELECT_CC_START): {
            if (v > 0) {
                auto pRack = model_->getRack(parent_.currentRack());
                if (pRack == nullptr) return;
                model_->saveSettings(Kontrol::CS_LOCAL, pRack->id());
                parent_.changeDisplayMode(P2D_Param);
                return;
            }
            break;
        }
        case (P2_TRACK_SELECT_CC_END - 1): {
            if (v > 0) {
                auto pRack = model_->getRack(parent_.currentRack());
                if (pRack == nullptr) return;
                model_->updatePreset(Kontrol::CS_LOCAL, pRack->id(), pRack->currentPreset());
                parent_.changeDisplayMode(P2D_Param);
                return;
            }
            break;
        }
        case (P2_TRACK_SELECT_CC_END - 2): {
            if (v > 0) {
                auto pRack = model_->getRack(parent_.currentRack());
                if (pRack == nullptr) return;
                auto presets = pRack->getResources("preset");
                int sz = presets.size();
                std::string newPreset = "New " + std::to_string(sz);
                LOG_0("create new preset " << newPreset);
                model_->updatePreset(Kontrol::CS_LOCAL, pRack->id(), newPreset);
                parent_.changeDisplayMode(P2D_Param);
                return;
            }
            break;
        }
        case P2_TRACK_SELECT_CC_END: {
            if (v > 0) {
                auto pRack = model_->getRack(parent_.currentRack());
                if (pRack == nullptr) return;
                int idx = 0;
                for (auto preset : pRack->getResources("preset")) {
                    if (idx == selectedIdx_) {
                        LOG_0("selected preset " << preset);
                        parent_.changeDisplayMode(P2D_Param);
                        model_->applyPreset(Kontrol::CS_LOCAL, pRack->id(), preset);
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
            if (value > 0 && selectedIdx_ == (pRack->getResources("preset").size() - 1)) return;
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
            offset = std::min(offset, (int) (pRack->getResources("preset").size() - 1) );
            if (offset != pageOffset_) {
                pageOffset_ = offset;
                displayPage();
            }
            break;
        }
    }
}


void P2_PresetMode::displayPage() {
    Push2API::Push2::Colour clr(0xFF, 0xFF, 0xFF);

    push2Api_->clearDisplay();

    push2Api_->drawCell8(MAX_ROW, MAX_COL, "    Apply", clr);
    push2Api_->drawCell8(MAX_ROW, MAX_COL-1, "    Update", clr);
    push2Api_->drawCell8(MAX_ROW, MAX_COL-2, "    New", clr);
    push2Api_->drawCell8(MAX_ROW, 0, "    Save", clr);

    // we need a current rack and also current module
    auto racks = model_->getRacks();
    if (racks.size() == 0) return;
    auto rack = racks[0];
    if (rack == nullptr) return;

    int x = 0, y = 0;
    int idx = 0;
    auto presets = rack->getResources("preset");
    auto iter = presets.begin();
    for (int i = 0; i < pageOffset_ && iter != presets.end(); i++) {
        iter++;
        idx++;
    }

    if (iter != presets.end()) {
        for (; iter != presets.end(); iter++) {
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

void P2_PresetMode::activate() {
    P2_DisplayMode::activate();
    auto pRack = model_->getRack(parent_.currentRack());
    if (pRack == nullptr) return;
    int idx = 0;
    for (auto preset : pRack->getResources("preset")) {
        if (preset == pRack->currentPreset()) {
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
    parent_.sendCC(0, P2_TRACK_SELECT_CC_START, 0x7f);
    parent_.sendCC(0, P2_TRACK_SELECT_CC_END-2, 0x7f);
    parent_.sendCC(0, P2_TRACK_SELECT_CC_END-1, 0x7f);
    parent_.sendCC(0, P2_TRACK_SELECT_CC_END, 0x7f);
    parent_.sendCC(0, P2_CURSOR_LEFT_CC, 0x7f);
    parent_.sendCC(0, P2_CURSOR_RIGHT_CC, 0x7f);
    parent_.sendCC(0, P2_SETUP_CC,0x00);
    displayPage();
}

} //namespace
