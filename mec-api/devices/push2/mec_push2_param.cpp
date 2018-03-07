#include "mec_push2_param.h"

#include <mec_log.h>

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

uint16_t page_clrs[8] = {
        RGB565(0xFF, 0xFF, 0xFF),
        RGB565(0xFF, 0, 0xFF),
        RGB565(0xFF, 0, 0),
        RGB565(0, 0xFF, 0xFF),
        RGB565(0, 0xFF, 0),
        RGB565(0x7F, 0x7F, 0xFF),
        RGB565(0xFF, 0x7F, 0xFF)
};

P2_ParamMode::P2_ParamMode(mec::Push2 &parent, const std::shared_ptr<Push2API::Push2> &api)
        : parent_(parent),
          push2Api_(api),
          pageIdx_(-1),
          moduleIdx_(-1) {
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
        try {
            auto rack = model_->getRack(rackId_);
            auto module = model_->getModule(rack, moduleId_);
            auto page = model_->getPage(module, pageId_);
            auto params = model_->getParams(module, page);

            if (idx >= params.size()) return;

            auto &param = params[idx];
            // auto page = pages_[currentPage_]
            if (param != nullptr) {
                const int steps = 1;
                // LOG_0("v = " << v);
                float vel = v & 0x40 ? (128.0f - (float) v) / (128.0f * steps) * -1.0f : float(v) / (128.0f * steps);

                Kontrol::ParamValue calc = param->calcRelative(vel);
                model_->changeParam(Kontrol::CS_LOCAL, rackId_, module->id(), param->id(), calc);
            }
        } catch (std::out_of_range) {

        }

    } else if (cc >= P2_DEV_SELECT_CC_START && cc <= P2_DEV_SELECT_CC_END) {
        unsigned idx = cc - P2_DEV_SELECT_CC_START;
        setCurrentPage(idx);

    } else if (cc >= P2_TRACK_SELECT_CC_START && cc <= P2_TRACK_SELECT_CC_END) {
        unsigned idx = cc - P2_TRACK_SELECT_CC_START;
        setCurrentModule(idx);
    } else if (cc == P2_USER_CC) {
        //DEBUG
        auto rack = model_->getRack(rackId_);
        auto module = model_->getModule(rack, moduleId_);
        if(module!= nullptr) module->dumpCurrentValues();
    }

}

void P2_ParamMode::drawParam(unsigned pos, const Kontrol::Parameter &param) {
    // #define MONO_CLR RGB565(255,60,0)
    uint16_t clr = page_clrs[pageIdx_];

    push2Api_->drawCell8(1, pos, centreText(param.displayName()).c_str(), VSCALE, HSCALE, clr);
    push2Api_->drawCell8(2, pos, centreText(param.displayValue()).c_str(), VSCALE, HSCALE, clr);
    push2Api_->drawCell8(3, pos, centreText(param.displayUnit()).c_str(), VSCALE, HSCALE, clr);
}

void P2_ParamMode::displayPage() {
//        int16_t clr = page_clrs[currentPage_];
    push2Api_->clearDisplay();
    for (unsigned int i = P2_DEV_SELECT_CC_START; i < P2_DEV_SELECT_CC_END; i++) { parent_.sendCC(0, i, 0); }
    for (unsigned int i = P2_TRACK_SELECT_CC_START; i < P2_TRACK_SELECT_CC_END; i++) { parent_.sendCC(0, i, 0); }

    auto rack = model_->getRack(rackId_);
    auto modules = model_->getModules(rack);
    auto module = model_->getModule(rack, moduleId_);
    auto page = model_->getPage(module, pageId_);
    auto pages = model_->getPages(module);
    auto params = model_->getParams(module, page);

    // draw pages
    unsigned int i = 0;
    for (auto cpage : pages) {
        push2Api_->drawCell8(0, i, centreText(cpage->displayName()).c_str(), VSCALE, HSCALE, page_clrs[i]);
        parent_.sendCC(0, P2_DEV_SELECT_CC_START + i, i == pageIdx_ ? 122 : 124);

        if (i == pageIdx_) {
            unsigned int j = 0;
            for (auto param : params) {
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

    // draw modules
    i = 0;
    for (auto mod : modules) {
        push2Api_->drawCell8(5, i, centreText(mod->displayName()).c_str(), VSCALE, HSCALE, page_clrs[i]);
        parent_.sendCC(0, P2_TRACK_SELECT_CC_START + i, i == moduleIdx_ ? 122 : 124);
        i++;
        if (i == 8) break;
    }
}


void P2_ParamMode::setCurrentModule(int moduleIdx) {
    auto rack = model_->getRack(rackId_);

    auto modules = model_->getModules(rack);

    if (moduleIdx != moduleIdx_ && moduleIdx < modules.size()) {
        moduleIdx_ = moduleIdx;

        try {
            moduleId_ = modules[moduleIdx_]->id();

            pageIdx_ = -1;
            setCurrentPage(0);
        } catch (std::out_of_range) { ;
        }
    }
}


void P2_ParamMode::setCurrentPage(int pageIdx) {
    auto rack = model_->getRack(rackId_);
    auto module = model_->getModule(rack, moduleId_);
//    auto page = model_->getPage(module,pageId_);
    auto pages = model_->getPages(module);
//    auto params = model_->getParams(module,page);

    if (pageIdx != pageIdx_ && pageIdx < pages.size()) {
        pageIdx_ = pageIdx;

        try {
            pageId_ = pages[pageIdx_]->id();
            displayPage();
        } catch (std::out_of_range) { ;
        }
    }
}

void P2_ParamMode::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    P2_DisplayMode::rack(src, rack);
    if (rackId_.length() == 0) rackId_ = rack.id();
}

void P2_ParamMode::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    P2_DisplayMode::module(src, rack, module);
    if (rackId_ != rack.id()) return;
    if (moduleId_.length() == 0) {
        moduleId_ = module.id();
        setCurrentModule(0);
    } else {
        // adjust moduleidx
        auto prack = model_->getRack(rackId_);
        auto modules = model_->getModules(prack);
        unsigned int i = 0;
        for (auto mod : modules) {
            if (mod->id() == moduleId_) {
                moduleIdx_ = i;

                if(module.type()!=moduleType_) {
                    // type has changed, so need to update the display
                    pageIdx_ = -1;
                    pageId_ ="";
                }
                break;
            }
            i++;
        }

    }
    moduleType_ = module.type();
//    LOG_0("P2_ParamMode::module" << module.id());
    displayPage();
}

void P2_ParamMode::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                        const Kontrol::Page &page) {
    P2_DisplayMode::page(src, rack, module, page);
    if (rackId_ != rack.id()) return;
    if (moduleId_ != module.id()) return;

    if (pageId_.length() == 0) {
        auto prack = model_->getRack(rackId_);
        auto pmodule = model_->getModule(prack, moduleId_);
        auto ppages = model_->getPages(pmodule);
        if (ppages.size() > 0) {
            setCurrentPage(0);
        }
    }

    // LOG_0("P2_ParamMode::page " << page.id());
    displayPage();
}

void P2_ParamMode::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                         const Kontrol::Parameter &param) {
    P2_DisplayMode::param(src, rack, module, param);
//    LOG_0("P2_ParamMode::param " << param.id() << " : " << param.current().floatValue());

    if (rackId_ != rack.id()) return;
    if (moduleId_ != module.id()) return;

    auto pRack = model_->getRack(rackId_);
    auto pModule = model_->getModule(pRack, moduleId_);
    auto pPage = model_->getPage(pModule, pageId_);
//    auto pPages = model_->pModule(pModule);
    auto pParams = model_->getParams(pModule, pPage);

    unsigned i = 0;
    for (auto p: pParams) {
        if (p->id() == param.id()) {
            p->change(param.current(), src == Kontrol::CS_PRESET);
            drawParam(i, param);
            return;

        }
        i++;
    }
}

void P2_ParamMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                           const Kontrol::Parameter &param) {
    P2_DisplayMode::changed(src, rack, module, param);
//    LOG_0("P2_ParamMode::changed " << param.id() << " : " << param.current().floatValue());
    auto pRack = model_->getRack(rackId_);
    auto pModule = model_->getModule(pRack, moduleId_);
    auto pPage = model_->getPage(pModule, pageId_);
//    auto pPages = model_->pModule(pModule);
    auto pParams = model_->getParams(pModule, pPage);

    unsigned i = 0;
    for (auto p: pParams) {
        if (p->id() == param.id()) {
            p->change(param.current(), src == Kontrol::CS_PRESET);
            drawParam(i, param);
            return;
        }
        i++;
    }
}

} //namespace
