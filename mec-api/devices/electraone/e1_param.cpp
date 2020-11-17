#include "e1_param.h"

#include <iostream>

namespace mec {

void ElectraOneParamMode::display() {
//    std::cerr << "ElectraOneParamMode::display() - begin" << std::endl;
    clearPages();

    createDevice(1, "Orac", 1, 1);

    auto rack = parent_.model()->getRack(parent_.currentRack());
    if (!rack) return;
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
    if (!module) return;

    unsigned kpageid = 0;
    unsigned pageid = 1;
    unsigned ctrlsetid = 1;

    unsigned pid = 1;
    createPage(pageid, module->id() + " : " + module->displayName());
    for (auto page: parent_.model()->getPages(module)) {
        kpageid++;
        if (kpageid > 3) {
            kpageid = 1;
            ctrlsetid++;
            if (ctrlsetid > 2) {
                createButton(E1_BTN_PREV_MODULE, pageid, 5, 4, "Prev Module");
                createButton(E1_BTN_NEXT_MODULE, pageid, 5, 5, "Next Module");
                ctrlsetid = 1;
                pageid++;
                std::string pname = module->id() + " : " + module->displayName() + " - " + std::to_string(pageid);
                createPage(pageid, pname);
            }
        }

        unsigned pos = 0;

        // create group for page
        createGroup(pageid, ctrlsetid, kpageid, page->displayName());

        for (auto param : parent_.model()->getParams(module, page)) {
            if (param != nullptr) {
                //            std::cerr << "ElectraOneParamMode::display() " << param->displayName() <<  std::endl;
                displayParamNum(pageid, ctrlsetid, kpageid, pos, pid, *param, true);
                pos++;
                pid++;
            }
        }
    }

    createButton(E1_BTN_MIDI_LEARN, pageid, 5, 0, "Midi Learn");
    createButton(E1_BTN_MOD_LEARN, pageid, 5, 1, "Mod Learn");
    createButton(E1_BTN_LOAD_MODULE, pageid, 5, 2, "Mod Learn");
    createButton(E1_BTN_PREV_MODULE, pageid, 5, 4, "Prev Module");
    createButton(E1_BTN_NEXT_MODULE, pageid, 5, 5, "Next Module");

    // module selection page
    pageid++;
    createPage(pageid, "Modules");
    unsigned mid=0, row=0,col=0;
    for(auto module : parent_.model()->getModules(rack)) {
        createButton(E1_BTN_FIRST_MODULE+mid,pageid,row, col,module->id());
        col++;
        if(col>5) {
            row++;
            col=0;
        }
        mid++;
    }

    pageid++;
    createPage(pageid, "Preset");
    createButton(E1_BTN_NEW_PRESET, pageid, 0, 0, "Create Preset");
    createButton(E1_BTN_SAVE_PRESET, pageid, 1, 0, "Save Preset");
    createButton(E1_BTN_LOAD_PRESET, pageid, 3, 0, "Load Preset");
    createButton(E1_BTN_SAVE, pageid, 5, 0, "Save Default");

    auto plist = rack->getResources("preset");
    createList(E1_CTL_PRESET_LIST, pageid,3,3,1, "Presets", plist);

    auto modlist = rack->getResources("module");
    createList(E1_CTL_MOD_LIST, pageid,4,3, 2, "Modules", modlist);


//    rack->savePreset(rack->currentPreset());
//    std::string newPreset = "new-" + std::to_string(presets_.size());
//    model()->savePreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
//    model()->loadPreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
//    model()->loadModule(Kontrol::CS_LOCAL, rack->id(), module->id(), modType);



//    std::cerr << "ElectraOneParamMode::display() - end " <<  std::endl;
    parent_.send(preset_);
}


void ElectraOneParamMode::activate() {
    ElectraOneBaseMode::activate();
    display();
}


void ElectraOneParamMode::displayParamNum(unsigned pageid, unsigned ctrlsetid, unsigned kpageid, unsigned pos,
                                          unsigned pid, const Kontrol::Parameter &p, bool local) {
    createParam(pageid, ctrlsetid, kpageid, pos, pid,
                p.displayName(), p.current().floatValue(),
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


void ElectraOneParamMode::onButton(unsigned id, unsigned value) {
    ElectraOneBaseMode::onButton(id, value);
    if (!value) {
        switch (id) {
            case E1_BTN_PREV_MODULE : {
                parent_.prevModule();
                break;
            }
            case E1_BTN_NEXT_MODULE : {
                parent_.nextModule();
                break;
            }
            default: {
                auto rack = parent_.model()->getRack(parent_.currentRack());
                auto modules =parent_.model()->getModules(rack);
                int mnum = id-E1_BTN_FIRST_MODULE;
                if (mnum >= 0  && mnum < modules.size() ) {
                    parent_.currentModule(modules[mnum]->id());
                }
            }
        }
    }
}


void ElectraOneParamMode::onEncoder(unsigned idx, int v) {
    ElectraOneBaseMode::onEncoder(idx, v);
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
                display();
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


//    unsigned sz = params.size();
//    sz = sz < NUM_ENCODERS;
//    for (unsigned int i = 0; i < sz; i++) {
//        try {
//            auto &p = params.at(i);
//            if (p->id() == param.id()) {
//                p->change(param.current(), src == Kontrol::CS_PRESET);
//                unsigned pgid =0; //TODO !!!!
//                displayParamNum(pgid,i, param, src != Kontrol::CS_LOCAL);
//                return;
//            }
//        } catch (std::out_of_range) {
//            return;
//        }
//    } // for
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


} // mec
