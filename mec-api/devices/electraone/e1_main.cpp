#include "e1_main.h"

#include <iostream>

namespace mec {

void ElectraOneMainMode::display() {
//    std::cerr << "ElectraOneMainMode::display() - begin" << std::endl;
    clearPages();

    createDevice(1, "Orac", 1, 1);
    paramMap_.clear();

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
                createButton(E1_BTN_MIDI_LEARN, pageid, 1, 4, 0, "Midi Learn");
                createButton(E1_BTN_MOD_LEARN, pageid, 1, 5, 0, "Mod Learn");
                createButton(E1_BTN_PREV_MODULE, pageid, 1, 4, 5, "Prev Module");
                createButton(E1_BTN_NEXT_MODULE, pageid, 1, 5, 5, "Next Module");
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
                //            std::cerr << "ElectraOneMainMode::display() " << param->displayName() <<  std::endl;
                unsigned id = displayParamNum(pageid, ctrlsetid, kpageid, pos, pid, *param, true);
                paramMap_[pid] = std::make_shared<ParameterID>(id, rack->id(), module->id(), param->id());
                pos++;
                pid++;
            }
        }
    }
    createButton(E1_BTN_MIDI_LEARN, pageid, 1, 4, 0, "Midi Learn");
    createButton(E1_BTN_MOD_LEARN, pageid, 1, 5, 0, "Mod Learn");
    createButton(E1_BTN_PREV_MODULE, pageid, 1, 4, 5, "Prev Module");
    createButton(E1_BTN_NEXT_MODULE, pageid, 1, 5, 5, "Next Module");

    auto modlist = rack->getResources("module");
    createList(E1_CTL_MOD_LIST, 1, 3, 5, 1, 1, "Modules", modlist, module->type());
    createButton(E1_BTN_LOAD_MODULE, 1, 3, 5, 3, "Load");

    // module selection page
    pageid++;
    createPage(pageid, "Modules");
    unsigned mid = 0, row = 0, col = 0;
    for (auto module : parent_.model()->getModules(rack)) {
        std::string label = module->id() + " : " + module->displayName();
        createButton(E1_BTN_FIRST_MODULE + mid, pageid, 1, row, col, label);
        col++;
        if (col > 5) {
            row++;
            col = 0;
        }
        mid++;
    }

    pageid++;
    createPage(pageid, "Preset");
    createButton(E1_BTN_NEW_PRESET, pageid, 1, 0, 0, "Create Preset");
    createButton(E1_BTN_SAVE_PRESET, pageid, 1, 1, 0, "Save Preset");
    createButton(E1_BTN_SAVE, pageid, 1, 5, 0, "Save Default");

    auto plist = rack->getResources("preset");
    createList(E1_CTL_PRESET_LIST, pageid, 1,
               0, 2, 1, "Presets", plist, rack->currentPreset());
    createButton(E1_BTN_LOAD_PRESET, pageid, 1, 1, 2, "Load Preset");

//    std::cerr << "ElectraOneMainMode::display() - end " <<  std::endl;
    parent_.send(preset_);
}


void ElectraOneMainMode::activate() {
    ElectraOneBaseMode::activate();
    display();
}


unsigned ElectraOneMainMode::displayParamNum(unsigned pageid, unsigned ctrlsetid, unsigned kpageid, unsigned pos,
                                             unsigned pid, const Kontrol::Parameter &p, bool local) {
    return createParam(pageid, ctrlsetid, kpageid, pos, pid,
                       p.displayName(), p.current().floatValue(),
                       p.calcMinimum().floatValue(), p.calcMaximum().floatValue());
}

void ElectraOneMainMode::changeParam(unsigned idx, int relValue) {
    //TODO - needed?
//    try {
//        auto pRack = model()->getRack(parent_.currentRack());
//        auto pModule = model()->getModule(pRack, parent_.currentModule());
//        auto pPage = model()->getPage(pModule, parent_.currentPage());
//        auto pParams = model()->getParams(pModule, pPage);
//
//
//        if (idx >= pParams.size()) return;
//        auto &param = pParams[idx];
//
//        if (param != nullptr) {
//            const float steps = 128.0f;
//            float value = float(relValue) / steps;
//            Kontrol::ParamValue calc = param->calcRelative(value);
//            //std::cerr << "changeParam " << idx << " " << value << " cv " << calc.floatValue() << " pv " << param->current().floatValue() << std::endl;
//            model()->changeParam(Kontrol::CS_LOCAL, parent_.currentRack(), pModule->id(), param->id(), calc);
//        }
//    } catch (std::out_of_range) {
//    }
}


void ElectraOneMainMode::onButton(unsigned id, unsigned value) {
    ElectraOneBaseMode::onButton(id, value);
    auto rack = model()->getRack(parent_.currentRack());
    if (!rack) return;
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
    if (!module) return;

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
            case E1_BTN_NEW_PRESET : {
                auto presets = rack->getResources("preset");
                std::string newPreset = "new-" + std::to_string(presets.size());
                model()->savePreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
                break;
            }
            case E1_BTN_LOAD_PRESET : {
                auto presets = rack->getResources("preset");
                unsigned idx = 0;
                for (auto preset : presets) {
                    if (selectedPresetIdx_ == idx) {
                        model()->loadPreset(Kontrol::CS_LOCAL, rack->id(), preset);
                        break;
                    }
                    idx++;
                }
                break;
            }
            case E1_BTN_SAVE_PRESET : {
                rack->savePreset(rack->currentPreset());
                break;
            }
            case E1_BTN_SAVE : {
                break;
            }
            case E1_BTN_LOAD_MODULE : {
                auto modules = rack->getResources("module");
                unsigned idx = 0;
                for (auto modtype : modules) {
                    if (selectedModuleIdx_ == idx) {
                        model()->loadModule(Kontrol::CS_LOCAL, rack->id(), module->id(), modtype);
                        break;
                    }
                    idx++;
                }
                break;
            }
            case E1_BTN_MIDI_LEARN : {
                parent_.midiLearn(!parent_.midiLearn());
                break;
            }
            case E1_BTN_MOD_LEARN : {
                parent_.modulationLearn(!parent_.modulationLearn());
                break;
            }
            default: {
                auto rack = parent_.model()->getRack(parent_.currentRack());
                auto modules = parent_.model()->getModules(rack);
                int mnum = id - E1_BTN_FIRST_MODULE;
                if (mnum >= 0 && mnum < modules.size()) {
                    parent_.currentModule(modules[mnum]->id());
                }
            }
        }
    }
}


void ElectraOneMainMode::onEncoder(unsigned idx, int v) {
    ElectraOneBaseMode::onEncoder(idx, v);
    auto rack = model()->getRack(parent_.currentRack());
    if (!rack) return;
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
    if (!module) return;
    switch (idx) {
        case E1_CTL_PRESET_LIST : {
            selectedPresetIdx_ = v;
            break;
        }
        case E1_CTL_MOD_LIST : {
            selectedModuleIdx_ = v;
            break;
        }
        default : {
            auto rack = parent_.model()->getRack(parent_.currentRack());
            auto module = parent_.model()->getModule(rack, parent_.currentModule());

            // check we have the param, also that we have not changed modules in the meantime (unlikely)
            if (module && paramMap_.find(idx) != paramMap_.end()) {
                auto paramId = paramMap_[idx];
                if (paramId->rack_ == rack->id() && paramId->module_ == module->id()) {
                    auto param = module->getParam(paramId->parameter_);
                    if (param) {
                        param->change(param->calcMidi(v), false);
                    }
                }
            }
        }
    }
}


void ElectraOneMainMode::setCurrentPage(unsigned pageIdx, bool UI) {
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


void ElectraOneMainMode::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                                      const Kontrol::Module &module) {
    if (rack.id() == parent_.currentRack()) {
        if (src != Kontrol::CS_LOCAL && module.id() != parent_.currentModule()) {
            parent_.currentModule(module.id());
        }
        pageIdx_ = -1;
        setCurrentPage(0, false);
    }
}


void ElectraOneMainMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                                 const Kontrol::Parameter &param) {
    if (rack.id() != parent_.currentRack() || module.id() != parent_.currentModule()) return;

    for (auto eParam : paramMap_) {
        auto eP = eParam.second;
        if (eP->rack_ == rack.id()
            && eP->module_ == module.id()
            && eP->parameter_ == param.id()) {
            unsigned pid=eP->eId_;
//                p->change(param.current(), src == Kontrol::CS_PRESET); //?
//            unsigned id = displayParamNum(pageid, ctrlsetid, kpageid, pos, pid, *param, true);

        }
    }
}

void ElectraOneMainMode::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                const Kontrol::Module &module) {
    if (moduleType_ != module.type()) {
        pageIdx_ = -1;
    }
    moduleType_ = module.type();
}

void ElectraOneMainMode::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                              const Kontrol::Page &page) {
    if (pageIdx_ < 0) setCurrentPage(0, false);
}


void ElectraOneMainMode::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                    const Kontrol::EntityId &moduleId, const std::string &modType) {
    if (parent_.currentModule() == moduleId) {
        if (moduleType_ != modType) {
            pageIdx_ = -1;
            moduleType_ = modType;
        }
    }
}

void ElectraOneMainMode::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    pageIdx_ = -1;
    setCurrentPage(0, false);
}


ParameterID::ParameterID(unsigned eId,
                         const Kontrol::EntityId &r,
                         const Kontrol::EntityId &m,
                         const Kontrol::EntityId &p)
        : eId_(eId), rack_(r), module_(m), parameter_(p) {
    ;
}
} // mec
