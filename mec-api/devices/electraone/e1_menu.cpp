#include "e1_menu.h"

namespace mec {

//---- ElectraOneMenuMode
void ElectraOneMenuMode::activate() {
    display();
}

void ElectraOneMenuMode::poll() {
    ElectraOneBaseMode::poll();
}

void ElectraOneMenuMode::display() {
}



void ElectraOneMenuMode::onButton(unsigned id, unsigned value) {
    ElectraOneBaseMode::onButton(id, value);
}

void ElectraOneMenuMode::onEncoder(unsigned id, int value) {
    ElectraOneBaseMode::onEncoder(id, value);
}



void ElectraOneMenuMode::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    display();
    KontrolCallback::savePreset(source, rack, preset);
}

void ElectraOneMenuMode::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    display();
    KontrolCallback::loadPreset(source, rack, preset);
}

void ElectraOneMenuMode::midiLearn(Kontrol::ChangeSource src, bool b) {
    display();
    KontrolCallback::midiLearn(src, b);
}

void ElectraOneMenuMode::modulationLearn(Kontrol::ChangeSource src, bool b) {
    display();
    KontrolCallback::modulationLearn(src, b);
}


/// main menu
enum OscMainMenuItms {
    E1_MMI_MODULE,
    E1_MMI_PRESET,
    E1_MMI_MIDILEARN,
    E1_MMI_MODLEARN,
    E1_MMI_SAVE,
    E1_MMI_SIZE
};

bool ElectraOneMainMenu::init() {
    return true;
}


unsigned ElectraOneMainMenu::getSize() {
    return (unsigned) E1_MMI_SIZE;
}

std::string ElectraOneMainMenu::getItemText(unsigned idx) {
    switch (idx) {
        case E1_MMI_MODULE: {
            auto rack = model()->getRack(parent_.currentRack());
            auto module = model()->getModule(rack, parent_.currentModule());
            if (module == nullptr)
                return parent_.currentModule();
            else
                return parent_.currentModule() + ":" + module->displayName();
        }
        case E1_MMI_PRESET: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                return rack->currentPreset();
            }
            return "No Preset";
        }
        case E1_MMI_SAVE:
            return "Save";
        case E1_MMI_MIDILEARN: {
            if (parent_.midiLearn()) {
                return "Midi Learn        [X]";
            }
            return "Midi Learn        [ ]";
        }
        case E1_MMI_MODLEARN: {
            if (parent_.modulationLearn()) {
                return "Mod Learn         [X]";
            }
            return "Mod Learn         [ ]";
        }
        default:
            break;
    }
    return "";
}


void ElectraOneMainMenu::clicked(unsigned idx) {
    switch (idx) {
        case E1_MMI_MODULE: {
            parent_.changeMode(E1_MODULEMENU);
            break;
        }
        case E1_MMI_PRESET: {
            parent_.changeMode(E1_PRESETMENU);
            break;
        }
        case E1_MMI_MIDILEARN: {
            parent_.midiLearn(!parent_.midiLearn());
//            displayItem(E1_MMI_MIDILEARN);
//            displayItem(E1_MMI_MODLEARN);
            // parent_.changeMode(E1_PARAMETER);
            break;
        }
        case E1_MMI_MODLEARN: {
            parent_.modulationLearn(!parent_.modulationLearn());
//            displayItem(E1_MMI_MIDILEARN);
//            displayItem(E1_MMI_MODLEARN);
            // parent_.changeMode(E1_PARAMETER);
            break;
        }
        case E1_MMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                model()->saveSettings(Kontrol::CS_LOCAL, rack->id());
            }
            parent_.changeMode(E1_PARAMETER);
            break;
        }
        default:
            break;
    }
}

void ElectraOneMainMenu::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) {
    display();
}

// preset menu
enum OscPresetMenuItms {
    E1_PMI_SAVE,
    E1_PMI_NEW,
    E1_PMI_SEP,
    E1_PMI_LAST
};


bool ElectraOnePresetMenu::init() {
    return true;
}

void ElectraOnePresetMenu::activate() {
    presets_.clear();
    auto rack = model()->getRack(parent_.currentRack());
    if (rack == nullptr) return;
    unsigned idx = 0;
    auto res = rack->getResources("preset");
    for (auto preset : res) {
        presets_.push_back(preset);
//        if (preset == rack->currentPreset()) {
//            cur_ = idx + 3;
//            top_ = idx + 3;
//        }
        idx++;
    }
    ElectraOneMenuMode::activate();
}


unsigned ElectraOnePresetMenu::getSize() {
    return (unsigned) E1_PMI_LAST + presets_.size();
}

std::string ElectraOnePresetMenu::getItemText(unsigned idx) {
//    switch (idx) {
//        case E1_PMI_SAVE:
//            return "Save Preset";
//        case E1_PMI_NEW:
//            return "New Preset";
//        case E1_PMI_SEP:
//            return "--------------------";
//        default:
//            return presets_[idx - E1_PMI_LAST];
//    }
    return "";
}


void ElectraOnePresetMenu::clicked(unsigned idx) {
    switch (idx) {
        case E1_PMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->savePreset(rack->currentPreset());
            }
            parent_.changeMode(E1_PARAMETER);
            break;
        }
        case E1_PMI_NEW: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = "new-" + std::to_string(presets_.size());
                model()->savePreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
            }
            parent_.changeMode(E1_PARAMETER);
            break;
        }
        case E1_PMI_SEP: {
            break;
        }
        default: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = presets_[idx - E1_PMI_LAST];
                parent_.changeMode(E1_PARAMETER);
                model()->loadPreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
            }
            break;
        }
    }
}


void ElectraOneModuleMenu::populateMenu(const std::string &catSel) {
    auto rack = model()->getRack(parent_.currentRack());
    auto module = model()->getModule(rack, parent_.currentModule());
    if (module == nullptr) return;
    unsigned idx = 0;
    auto res = rack->getResources("module");
    items_.clear();
//    cur_ = 0;
//    top_ = 0;
//    std::set<std::string> cats;
//    unsigned catlen = cat_.length();
//
//    if (catlen) {
//        items_.push_back("..");
//        idx++;
//    }
//
//    for (const auto &modtype : res) {
//        if (cat_.length()) {
//            size_t pos = modtype.find(cat_);
//            if (pos == 0) {
//                std::string mod = modtype.substr(catlen, modtype.length() - catlen);
//                items_.push_back(mod);
//                if (module->type() == modtype) {
//                    cur_ = idx;
//                    top_ = idx;
//                }
//                idx++;
//            } // else filtered
//        } else {
//            // top level, so get categories
//            size_t pos = modtype.find("/");
//            if (pos == std::string::npos) {
//                items_.push_back(modtype);
//                if (modtype == module->type()) {
//                    cur_ = idx;
//                    top_ = idx;
//                }
//                idx++;
//            } else {
//                cats.insert(modtype.substr(0, pos + 1));
//            }
//        }
//    }
//
//
//    size_t pos = std::string::npos;
//    std::string modcat;
//    pos = module->type().find("/");
//    if (pos != std::string::npos) {
//        modcat = module->type().substr(0, pos + 1);
//    }
//
//
//    for (auto s: cats) {
//        items_.push_back(s);
//        if (catSel.length() && s == catSel) {
//            cur_ = idx;
//            top_ = idx;
//        }
//        idx++;
//    }
}


void ElectraOneModuleMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    auto module = model()->getModule(rack, parent_.currentModule());
    if (module == nullptr) return;
    unsigned idx = 0;
    auto res = rack->getResources("module");
    cat_ = "";

    size_t pos = std::string::npos;
    pos = module->type().find("/");
    if (pos != std::string::npos) {
        cat_ = module->type().substr(0, pos + 1);
    }

    populateMenu(cat_);
    ElectraOneFixedMenuMode::activate();
}


void ElectraOneModuleMenu::clicked(unsigned idx) {
    if (idx < getSize()) {
        auto modtype = items_[idx];
        if (modtype == "..") {
            std::string oldcat = cat_;
            cat_ = "";
            populateMenu(oldcat);
            display();
            return;
        } else {
            if (cat_.length()) {
                // module dir
                Kontrol::EntityId modType = cat_ + modtype;
                auto rack = model()->getRack(parent_.currentRack());
                auto module = model()->getModule(rack, parent_.currentModule());
                if (modType != module->type()) {
                    //FIXME, workaround since changing the module needs to tell params to change page
                    model()->loadModule(Kontrol::CS_LOCAL, rack->id(), module->id(), modType);
                }
            } else {
                cat_ = modtype;
                populateMenu("");
                display();
                return;
            }
        }
    }
    parent_.changeMode(E1_PARAMETER);
}


void ElectraOneModuleSelectMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    auto cmodule = model()->getModule(rack, parent_.currentModule());
    if (cmodule == nullptr) return;
    unsigned idx = 0;
    items_.clear();
//    for (auto module : rack->getModules()) {
//        std::string desc = module->id() + ":" + module->displayName();
//        items_.push_back(desc);
//        if (module->id() == cmodule->id()) {
//            cur_ = idx;
//            top_ = idx;
//        }
//        idx++;
//    }
    ElectraOneFixedMenuMode::activate();
}


void ElectraOneModuleSelectMenu::clicked(unsigned idx) {
    parent_.changeMode(E1_MAINMENU);
    if (idx < getSize()) {
        unsigned moduleIdx = idx;
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto modules = parent_.model()->getModules(rack);
        if (moduleIdx < modules.size()) {
            auto module = modules[moduleIdx];
            auto moduleId = module->id();
            if (parent_.currentModule() != moduleId) {
                parent_.currentModule(moduleId);
            }
        }
    }
}


}//mec
