#include "mec_push2_param.h"

#include <mec_log.h>

namespace mec {
std::string centreText(const std::string t) {
    const unsigned max_len = 16;
    unsigned len = t.length();
    if (len > max_len) return t;
    const std::string pad = "                         ";
    unsigned padlen = (max_len - len) / 2;
    std::string ret = pad.substr(0, padlen) + t + pad.substr(0, padlen);
    // LOG_0("pad " << t << " l " << len << " pl " << padlen << "[" << ret << "]");
    return ret;
}




Push2API::Push2::Colour page_clrs[8] = {
        Push2API::Push2::Colour(0xFF, 0xFF, 0xFF),
        Push2API::Push2::Colour(0xFF, 0, 0xFF),
        Push2API::Push2::Colour(0xFF, 0, 0),
        Push2API::Push2::Colour(0, 0xFF, 0xFF),
        Push2API::Push2::Colour(0, 0xFF, 0),
        Push2API::Push2::Colour(0x7F, 0x7F, 0xFF),
        Push2API::Push2::Colour(0xFF, 0x7F, 0xFF),
        Push2API::Push2::Colour(0, 0, 0)
};

P2_ParamMode::P2_ParamMode(mec::Push2 &parent, const std::shared_ptr<Push2API::Push2> &api)
        : parent_(parent),
          push2Api_(api),
          pageIdx_(-1),
          moduleIdx_(-1) {
    model_ = Kontrol::KontrolModel::model();
}

void P2_ParamMode::processNoteOn(unsigned, unsigned) {
    ;
}

void P2_ParamMode::processNoteOff(unsigned, unsigned) {
    ;
}


void P2_ParamMode::processCC(unsigned cc, unsigned v) {
    if(!v) return;

    if (cc >= P2_ENCODER_CC_START && cc <= P2_ENCODER_CC_END) {
        unsigned idx = cc - P2_ENCODER_CC_START;
        try {
            auto pRack = model_->getRack(parent_.currentRack());
            auto pModule = model_->getModule(pRack, parent_.currentModule());
            auto pPage = model_->getPage(pModule, parent_.currentPage());
            auto pParams = model_->getParams(pModule, pPage);

            if (idx >= pParams.size()) return;

            auto &param = pParams[idx];
            // auto page = pages_[currentPage_]
            if (param != nullptr) {
                const int steps = 1;
                // LOG_0("v = " << v);
                float value = v & 0x40 ? (128.0f - (float) v) / (128.0f * steps) * -1.0f : float(v) / (128.0f * steps);

                Kontrol::ParamValue calc = param->calcRelative(value);
                model_->changeParam(Kontrol::CS_LOCAL, parent_.currentRack(), pModule->id(), param->id(), calc);
            }
        } catch (std::out_of_range) {

        }

    } else if (cc >= P2_DEV_SELECT_CC_START && cc <= P2_DEV_SELECT_CC_END) {
        unsigned idx = cc - P2_DEV_SELECT_CC_START;
        setCurrentPage(idx);

    } else if (cc >= P2_TRACK_SELECT_CC_START && cc <= P2_TRACK_SELECT_CC_END) {
        unsigned idx = cc - P2_TRACK_SELECT_CC_START;
        setCurrentModule(idx);
    } else if (cc == P2_SETUP_CC) {
        parent_.midiLearn(!parent_.midiLearn());
        parent_.sendCC(0, P2_SETUP_CC, ( parent_.midiLearn() ? 0x7f:  0x10));
    } else if (cc == P2_USER_CC) {
        //DEBUG
        auto pRack = model_->getRack(parent_.currentRack());
        auto pModule = model_->getModule(pRack, parent_.currentModule());
        if (pModule != nullptr) pModule->dumpCurrentValues();
    }
}

void P2_ParamMode::drawParam(unsigned pos, const Kontrol::Parameter &param) {
    Push2API::Push2::Colour clr = page_clrs[pageIdx_];

    push2Api_->drawCell8(1, pos, centreText(param.displayName()).c_str(), clr);
    push2Api_->drawCell8(2, pos, centreText(param.displayValue()).c_str(),clr);
    push2Api_->drawCell8(3, pos, centreText(param.displayUnit()).c_str(), clr);
}

void P2_ParamMode::displayPage() {
//        int16_t clr = page_clrs[currentPage_];
    push2Api_->clearDisplay();
    for (unsigned int i = P2_DEV_SELECT_CC_START; i < P2_DEV_SELECT_CC_END; i++) { parent_.sendCC(0, i, 0); }
    for (unsigned int i = P2_TRACK_SELECT_CC_START; i < P2_TRACK_SELECT_CC_END; i++) { parent_.sendCC(0, i, 0); }

    auto pRack = model_->getRack(parent_.currentRack());
    auto pModules = model_->getModules(pRack);
    auto pModule = model_->getModule(pRack, parent_.currentModule());
    auto pPage = model_->getPage(pModule, parent_.currentPage());
    auto pPages = model_->getPages(pModule);
    auto pParams = model_->getParams(pModule, pPage);

    // draw pages
    unsigned int i = 0;
    for (auto cpage : pPages) {
        push2Api_->drawCell8(0, i, centreText(cpage->displayName()).c_str(), page_clrs[i]);
        parent_.sendCC(0, P2_DEV_SELECT_CC_START + i, i == pageIdx_ ? 122 : 124);

        if (i == pageIdx_) {
            unsigned int j = 0;
            for (auto param : pParams) {
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
    for (auto mod : pModules) {
        push2Api_->drawCell8(5, i, centreText(mod->displayName()).c_str(), page_clrs[i]);
        parent_.sendCC(0, P2_TRACK_SELECT_CC_START + i, i == moduleIdx_ ? 122 : 124);
        i++;
        if (i == 8) break;
    }
}


void P2_ParamMode::setCurrentModule(int moduleIdx) {
    auto pRack = model_->getRack(parent_.currentRack());
    auto pModules = model_->getModules(pRack);
//    auto pModule = model_->getModule(pRack, parent_.currentModule());
////    auto pPage = model_->getPage(pModule, parent_.currentPage());
//    auto pPages = model_->getPages(pModule);
////    auto pParams = model_->getParams(pModule, pPage);


    if (moduleIdx != moduleIdx_ && moduleIdx < pModules.size()) {
        moduleIdx_ = moduleIdx;

        try {
            parent_.currentModule(pModules[moduleIdx_]->id());

            pageIdx_ = -1;
            setCurrentPage(0);
        } catch (std::out_of_range) { ;
        }
    }
}


void P2_ParamMode::setCurrentPage(int pageIdx) {
    auto pRack = model_->getRack(parent_.currentRack());
    auto pModule = model_->getModule(pRack, parent_.currentModule());
//    auto pPage = model_->getPage(pModule, parent_.currentPage());
    auto pPages = model_->getPages(pModule);
//    auto pParams = model_->getParams(pModule, pPage);

    if(pPages.size()==0) {
        parent_.currentPage("");
        displayPage();
    } else if (pageIdx != pageIdx_ && pageIdx < pPages.size()) {
        pageIdx_ = pageIdx;

        try {
            parent_.currentPage(pPages[pageIdx_]->id());
            displayPage();
        } catch (std::out_of_range) { ;
        }
    }
}

void P2_ParamMode::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    P2_DisplayMode::rack(src, rack);
    if (parent_.currentRack().length() == 0) parent_.currentRack(rack.id());
}

void P2_ParamMode::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    P2_DisplayMode::module(src, rack, module);
    if (parent_.currentRack() != rack.id()) return;
    if (parent_.currentModule().length() == 0) {
        parent_.currentModule(module.id());
        setCurrentModule(0);
    } else {
        // adjust moduleidx
        auto prack = model_->getRack(parent_.currentRack());
        auto modules = model_->getModules(prack);
        unsigned int i = 0;
        for (auto mod : modules) {
            if (mod->id() == parent_.currentModule()) {
                moduleIdx_ = i;

                if (module.type() != moduleType_) {
                    // type has changed, so need to update the display
                    pageIdx_ = -1;
                    parent_.currentPage("");
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
    if (parent_.currentRack() != rack.id()) return;
    if (parent_.currentModule() != module.id()) return;

    if (parent_.currentPage().length() == 0) {
        auto pRack = model_->getRack(parent_.currentRack());
        auto pModule = model_->getModule(pRack, parent_.currentModule());
        auto pPages = model_->getPages(pModule);
        if (pPages.size() > 0) {
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

    if (parent_.currentRack() != rack.id()) return;
    if (parent_.currentModule() != module.id()) return;

    auto pRack = model_->getRack(parent_.currentRack());
    auto pModule = model_->getModule(pRack, parent_.currentModule());
    auto pPage = model_->getPage(pModule, parent_.currentPage());
//    auto pPages = model_->getPages(pModule);
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
    auto pRack = model_->getRack(parent_.currentRack());
    auto pModule = model_->getModule(pRack, parent_.currentModule());
    auto pPage = model_->getPage(pModule, parent_.currentPage());
//    auto pPages = model_->getPages(pModule);
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

void P2_ParamMode::activate() {
    P2_DisplayMode::activate();
    displayPage();
    for (int i = P2_DEV_SELECT_CC_START; i <= P2_DEV_SELECT_CC_END; i++) {
        parent_.sendCC(0, i, 0);
    }
    for (int i = P2_TRACK_SELECT_CC_START; i <= P2_TRACK_SELECT_CC_END; i++) {
        parent_.sendCC(0, i, 0);
    }
    parent_.sendCC(0, P2_CURSOR_LEFT_CC, 0x00);
    parent_.sendCC(0, P2_CURSOR_RIGHT_CC, 0x00);
    parent_.sendCC(0, P2_SETUP_CC, ( parent_.midiLearn() ? 0x7f:  0x10));
}


void P2_ParamMode::applyPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    P2_DisplayMode::applyPreset(source,rack,preset);
    setCurrentPage(0);
    displayPage();
}


void P2_ParamMode::midiLearn(Kontrol::ChangeSource src, bool b) {
    parent_.sendCC(0, P2_SETUP_CC, ( parent_.midiLearn() ? 0x7f:  0x10));
}

void P2_ParamMode::modulationLearn(Kontrol::ChangeSource src, bool b) {
    parent_.sendCC(0, P2_SETUP_CC, ( parent_.midiLearn() ? 0x7f:  0x10));
}



} //namespace
