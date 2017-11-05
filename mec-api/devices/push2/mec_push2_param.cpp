#include "mec_push2_param.h"

//#include <mec_log.h>

namespace mec {
std::string centreText(const std::string t) {
    unsigned len = t.length();
    if (len > 24) return t;
    const std::string pad = "                         ";
    unsigned padlen = (24 - len) / 2;
    std::string ret = pad.substr(0, padlen) + t + pad.substr(0, padlen);
    // LOG_0("pad " << t << " l " << len << " pl " << padlen << "[" << ret << "]");
    return ret;
}

static const unsigned VSCALE = 3;
static const unsigned HSCALE = 1;

int16_t page_clrs[8] = {
        RGB565(0xFF, 0xFF, 0xFF),
        RGB565(0xFF, 0, 0xFF),
        RGB565(0xFF, 0, 0),
        RGB565(0, 0xFF, 0xFF),
        RGB565(0, 0xFF, 0),
        RGB565(0x7F, 0x7F, 0xFF),
        RGB565(0xFF, 0x7F, 0xFF)
};

P2_ParamMode::P2_ParamMode(mec::Push2 &parent, const std::__1::shared_ptr<Push2API::Push2> &api)
        : parent_(parent),
          push2Api_(api),
          currentModule_(-1),
          currentPage_(-1) {
    model_ = Kontrol::KontrolModel::model();;
}

void P2_ParamMode::processNoteOn(unsigned, unsigned) {
    ;
}

void P2_ParamMode::processNoteOff(unsigned, unsigned) {
    ;
}


void P2_ParamMode::processCC(unsigned cc, unsigned v) {
    if (cc >= P2_ENCODER_CC_START && cc <= P2_ENCODER_CC_END) {
        unsigned idx = cc - P2_ENCODER_CC_START;
        //TODO
        try {
            if (idx >= params_.size()) return;
            auto param = params_[idx];
            // auto page = pages_[currentPage_]
            if (param != nullptr) {
                const int steps = 1;
                // LOG_0("v = " << v);
                float vel = v & 0x40 ? (128.0f - (float) v) / (128.0f * steps) * -1.0f : float(v) / (128.0f * steps);

                Kontrol::ParamValue calc = param->calcRelative(vel);
                auto module = modules_[currentModule_];
                model_->changeParam(Kontrol::PS_LOCAL, rack_->id(), module->id(), param->id(), calc);
            }
        } catch (std::out_of_range) {

        }

    } else if (cc >= P2_DEV_SELECT_CC_START && cc <= P2_DEV_SELECT_CC_END) {
//            unsigned idx = cc - P2_DEV_SELECT_CC_START;

    } else if (cc >= P2_TRACK_SELECT_CC_START && cc <= P2_TRACK_SELECT_CC_END) {
        unsigned idx = cc - P2_TRACK_SELECT_CC_START;
        setCurrentPage(idx);
    }
}

void P2_ParamMode::drawParam(unsigned pos, const Kontrol::Parameter &param) {
    // #define MONO_CLR RGB565(255,60,0)
    int16_t clr = page_clrs[currentPage_];

    push2Api_->drawCell8(1, pos, centreText(param.displayName()).c_str(), VSCALE, HSCALE, clr);
    push2Api_->drawCell8(2, pos, centreText(param.displayValue()).c_str(), VSCALE, HSCALE, clr);
    push2Api_->drawCell8(3, pos, centreText(param.displayUnit()).c_str(), VSCALE, HSCALE, clr);
}

void P2_ParamMode::displayPage() {
//        int16_t clr = page_clrs[currentPage_];
    push2Api_->clearDisplay();
    for (unsigned int i = P2_TRACK_SELECT_CC_START; i < P2_TRACK_SELECT_CC_END; i++) { parent_.sendCC(0, i, 0); }

    push2Api_->drawCell8(0, 0, "Kontrol", VSCALE, HSCALE, RGB565(0x7F, 0x7F, 0x7F));

    unsigned int i = 0;
    for (auto page : pages_) {
        push2Api_->drawCell8(5, i, centreText(page->displayName()).c_str(), VSCALE, HSCALE, page_clrs[i]);
        parent_.sendCC(0, P2_TRACK_SELECT_CC_START + i, i == currentPage_ ? 122 : 124);

        if (i == currentPage_) {
            unsigned int j = 0;
            for (auto param : params_) {
                if (param != nullptr) {
                    drawParam(j, *param);
                }
                j++;
                if (j == 8) break;
            }
        }
        i++;
        if (i == 8) break;
    }
}

void P2_ParamMode::setCurrentPage(int page) {
    if (page != currentPage_ && page< pages_.size()) {
        currentPage_ = page;
        try {
            auto pmodule = modules_[currentModule_];
            auto ppage = pages_[currentPage_];
            params_ = pmodule->getParams(ppage);
            displayPage();
        } catch (std::out_of_range) { ;
        }
    }
}

void P2_ParamMode::rack(Kontrol::ParameterSource src, const Kontrol::Rack &rack) {
    P2_DisplayMode::rack(src,rack);
}

void P2_ParamMode::module(Kontrol::ParameterSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    P2_DisplayMode::module(src,rack,module);
}

void P2_ParamMode::page(Kontrol::ParameterSource src,const Kontrol::Rack &rack, const Kontrol::Module &module, const Kontrol::Page &page) {
    P2_DisplayMode::page(src, rack,module, page);
    auto model = Kontrol::KontrolModel::model();
    if (!rack_) {
        rack_ = model->getRack(rack.id());
    }

    if (currentModule_ < 0) {
        auto pr = model->getRack(rack.id());
        auto pm = model->getModule(pr, module.id());
        currentModule_ = 0;
        modules_ = model->getModules(pr);
        if (pm) {
            pages_ = model->getPages(pm);
            if (pages_.size() > 0) {
                setCurrentPage(0);
            } else {
                currentPage_ = -1;
            }
        }
    } else {
        // really we need to track current module here
        auto pm = model->getModule(
                model->getRack(rack.id()),
                module.id());
        pages_ = model->getPages(pm);
    }

    // LOG_0("P2_ParamMode::page " << page.id());
    displayPage();
}

void P2_ParamMode::param(Kontrol::ParameterSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                         const Kontrol::Parameter &param) {
    P2_DisplayMode::param(src,rack,module,param);
    unsigned i = 0;
    for (auto p: params_) {
        if (p->id() == param.id()) {
            drawParam(i, param);
            return;
        }
        i++;
    }
}

void P2_ParamMode::changed(Kontrol::ParameterSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                           const Kontrol::Parameter &param) {
    P2_DisplayMode::changed(src,rack,module,param);
    unsigned i = 0;
    for (auto p: params_) {
        if (p->id() == param.id()) {
            drawParam(i, param);
            return;
        }
        i++;
    }
}

} //namespace
