#include "e1_param.h"

#include <iostream>

namespace mec {


//const unsigned SCREEN_HEIGHT = 600;
//const unsigned SCREEN_WIDTH = 1024;
const unsigned NUM_ENCODERS = 12;
//
//const unsigned encNum(unsigned page, unsigned param) {
//    unsigned pg = page % 3; // 3 pages per screen
//    unsigned row = param / 2;
//    unsigned col = param % 2;
//    return (row * (NUM_ENCODERS/ 2)) + (pg * 2) + col;
//}


void ElectraOneParamMode::display() {
//    std::cerr << "ElectraOneParamMode::display() - begin" << std::endl;
    clearPages();
    createDevice(1, "Orac", 1, 1);

    createButton(100,0,0,"Prev Page");
    createButton(101,0,1,"Next Page");
    createButton(102,0,4,"Prev Module");
    createButton(103,0,5,"Next Module");

    createKeyboard();

    //    parent_.clearDisplay();
    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());

    std::string md = "";
    std::string pd = "";
    if (module) md = module->id() + " : " + module->displayName();

    createPage(1, md);
    auto pages = parent_.model()->getPages(module);

    for (auto pg = 0; pg < 3; pg++) {
        if (pageIdx_ + pg < pages.size()) {
            auto page = pages[pageIdx_ + pg];
            if (page) {
                auto params = parent_.model()->getParams(module, page);
                if (page) pd = page->displayName();

                // create group for page
                createGroup(pg, pd);
                unsigned int j = 0;
                for (auto param : params) {
                    if (param != nullptr) {
                        //            std::cerr << "ElectraOneParamMode::display() " << param->displayName() <<  std::endl;
                        displayParamNum(pg, j, *param, true);
                    }
                    j++;
                    if (j == 4) break;
                }
                for (; j < 4; j++) {
                    //        parent_.clearParamNum(j);
                }
            }
        }
    }

//    std::cerr << "ElectraOneParamMode::display() - end " <<  std::endl;
    parent_.send(preset_);
}


void ElectraOneParamMode::activate() {
    ElectraOneBaseMode::activate();
    display();
}

void ElectraOneParamMode::onButton(unsigned id, unsigned value) {
    ElectraOneBaseMode::onButton(id, value);
    switch (id) {
        case 0 : {
            if (!value) {
                // on release of button
                parent_.changeMode(NM_MAINMENU);
            }
            break;
        }
        case 1 : {
            break;
        }
        case 2 : {
            break;
        }
        default:;
    }
}

void ElectraOneParamMode::displayParamNum(unsigned pageid, unsigned num, const Kontrol::Parameter &p, bool local) {
    createParam(pageid, num, p.displayName(), p.current().floatValue(),
                p.calcMinimum().floatValue(), p.calcMaximum().floatValue());
}

void ElectraOneParamMode::changeParam(unsigned idx, int relValue) {
    try {
        auto pRack = model()->getRack(parent_.currentRack());
        auto pModule = model()->getModule(pRack, parent_.currentModule());
        auto pPage = model()->getPage(pModule, parent_.currentPage());
        auto pParams = model()->getParams(pModule, pPage);


        if (idx >= pParams.size()) return;
        auto &param = pParams[idx];

        if (param != nullptr) {
            const float steps = 128.0f;
            float value = float(relValue) / steps;
            Kontrol::ParamValue calc = param->calcRelative(value);
            //std::cerr << "changeParam " << idx << " " << value << " cv " << calc.floatValue() << " pv " << param->current().floatValue() << std::endl;
            model()->changeParam(Kontrol::CS_LOCAL, parent_.currentRack(), pModule->id(), param->id(), calc);
        }
    } catch (std::out_of_range) {
    }
}


void ElectraOneParamMode::onEncoder(unsigned idx, int v) {
    ElectraOneBaseMode::onEncoder(idx, v);
    if (idx == 2 && buttonState_[2]) {
        // if holding button 3. then turning encoder 3 changed page
        if (v > 0) {
            nextPage();
        } else {
            prevPage();
        }

    } else if (idx == 1 && buttonState_[2]) {
        // if holding button 3. then turning encoder 2 changed module
        if (v > 0) {
            parent_.nextModule();
        } else {
            parent_.prevModule();
        }
    } else {
        if (idx >= sizeof(idx)) return;
//        auto paramIdx = param1EncoderMap[idx];
//        changeParam(paramIdx, v);
    }
}


void ElectraOneParamMode::setCurrentPage(unsigned pageIdx, bool UI) {
    try {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
        //auto page = parent_.model()->getPage(module, pageId_);
        auto pages = parent_.model()->getPages(module);
//        auto params = parent_.model()->getParams(module,page);

        if (pageIdx_ != pageIdx) {
            pageIdx_ = pageIdx;
            if (pageIdx < pages.size()) {
                pageIdx_ = pageIdx;
                try {
                    auto page = pages[pageIdx_];
                    pageId_ = page->id();
                    parent_.currentPage(pageId_);
                    display();
                } catch (std::out_of_range) { ;
                }
            } else {
//                parent_.clearDisplay();
                std::string md = "";
                std::string pd = "";
                if (module) md = module->id() + " : " + module->displayName();
//                parent_.displayTitle(md, "none");
            }
        }

    } catch (std::out_of_range) { ;
    }
}


void ElectraOneParamMode::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                                       const Kontrol::Module &module) {
    if (rack.id() == parent_.currentRack()) {
        if (src != Kontrol::CS_LOCAL && module.id() != parent_.currentModule()) {
            parent_.currentModule(module.id());
        }
        pageIdx_ = -1;
        setCurrentPage(0, false);
    }
}


void ElectraOneParamMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                                  const Kontrol::Parameter &param) {
    if (rack.id() != parent_.currentRack() || module.id() != parent_.currentModule()) return;

    auto prack = parent_.model()->getRack(parent_.currentRack());
    auto pmodule = parent_.model()->getModule(prack, parent_.currentModule());
    auto page = parent_.model()->getPage(pmodule, pageId_);
//    auto pages = parent_.model()->getPages(pmodule);
    auto params = parent_.model()->getParams(pmodule, page);


    unsigned sz = params.size();
    sz = sz < NUM_ENCODERS;
    for (unsigned int i = 0; i < sz; i++) {
        try {
            auto &p = params.at(i);
            if (p->id() == param.id()) {
                p->change(param.current(), src == Kontrol::CS_PRESET);
                unsigned pgid =0; //TODO !!!!
                displayParamNum(pgid,i, param, src != Kontrol::CS_LOCAL);
                return;
            }
        } catch (std::out_of_range) {
            return;
        }
    } // for
}

void ElectraOneParamMode::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                 const Kontrol::Module &module) {
    if (moduleType_ != module.type()) {
        pageIdx_ = -1;
    }
    moduleType_ = module.type();
}

void ElectraOneParamMode::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                               const Kontrol::Page &page) {
    if (pageIdx_ < 0) setCurrentPage(0, false);
}


void ElectraOneParamMode::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                     const Kontrol::EntityId &moduleId, const std::string &modType) {
    if (parent_.currentModule() == moduleId) {
        if (moduleType_ != modType) {
            pageIdx_ = -1;
            moduleType_ = modType;
        }
    }
}

void ElectraOneParamMode::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    pageIdx_ = -1;
    setCurrentPage(0, false);
}

void ElectraOneParamMode::nextPage() {
    if (pageIdx_ < 0) {
        setCurrentPage(0, false);
        return;
    }

    unsigned pagenum = (unsigned) pageIdx_;

    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
//        auto page = parent_.model()->getPage(module,pageId_);
    auto pages = parent_.model()->getPages(module);
//        auto params = parent_.model()->getParams(module,page);
    pagenum++;
    pagenum = std::min(pagenum, (unsigned) pages.size() - 1);

    if (pagenum != pageIdx_) {
        setCurrentPage(pagenum, true);
    }
}

void ElectraOneParamMode::prevPage() {
    if (pageIdx_ < 0) {
        setCurrentPage(0, false);
        return;
    }
    unsigned pagenum = (unsigned) pageIdx_;
    if (pagenum > 0) pagenum--;

    if (pagenum != pageIdx_) {
        setCurrentPage(pagenum, true);
    }
}


} // mec
