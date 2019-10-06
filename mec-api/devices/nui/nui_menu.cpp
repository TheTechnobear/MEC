#include "nui_menu.h"

namespace mec {

static const unsigned NUI_NUM_TEXTLINES = 5;

//---- NuiMenuMode
void NuiMenuMode::activate() {
    display();
    popupTime_ = parent_.menuTimeout();
}

void NuiMenuMode::poll() {
    NuiBaseMode::poll();
    if (popupTime_ == 0) {
        parent_.changeMode(NM_PARAMETER);
        popupTime_ = -1;
    }
}

void NuiMenuMode::display() {
    parent_.clearDisplay();
    for (unsigned i = top_; i < top_ + NUI_NUM_TEXTLINES; i++) {
        displayItem(i);
    }
}

void NuiMenuMode::displayItem(unsigned i) {
    if (i < getSize()) {
        std::string item = getItemText(i);
        unsigned line = i - top_ + 1;
        parent_.displayLine(line, item.c_str());
        if (i == cur_) {
            parent_.invertLine(line);
        }
    }
}

void NuiMenuMode::onButton(unsigned id, unsigned value) {
    NuiBaseMode::onButton(id, value);
    switch (id) {
        case 0 : {
            if (!value) {
                // on release of button
                parent_.changeMode(NM_PARAMETER);
            }
            break;
        }
        case 1 : {
            break;
        }
        case 2 : {
            if (!value) {
                navActivate();
            }
            break;
        }
        default:;
    }
}

void NuiMenuMode::onEncoder(unsigned id, int value) {
    if (id == 0) {
        if (value > 0) {
            navNext();
        } else {
            navPrev();
        }
    }

}


void NuiMenuMode::navPrev() {
    unsigned cur = cur_;
    if (cur_ > 0) {
        cur--;
        unsigned int line = 0;
        if (cur < top_) {
            top_ = cur;
            cur_ = cur;
            display();
        } else if (cur >= top_ + NUI_NUM_TEXTLINES) {
            top_ = cur - (NUI_NUM_TEXTLINES - 1);
            cur_ = cur;
            display();
        } else {
            line = cur_ - top_ + 1;
            if (line <= NUI_NUM_TEXTLINES) parent_.invertLine(line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= NUI_NUM_TEXTLINES) parent_.invertLine(line);
        }
    }
    popupTime_ = parent_.menuTimeout();
}


void NuiMenuMode::navNext() {
    unsigned cur = cur_;
    cur++;
    cur = std::min(cur, getSize() - 1);
    if (cur != cur_) {
        unsigned int line = 0;
        if (cur < top_) {
            top_ = cur;
            cur_ = cur;
            display();
        } else if (cur >= top_ + NUI_NUM_TEXTLINES) {
            top_ = cur - (NUI_NUM_TEXTLINES - 1);
            cur_ = cur;
            display();
        } else {
            line = cur_ - top_ + 1;
            if (line <= NUI_NUM_TEXTLINES) parent_.invertLine(line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= NUI_NUM_TEXTLINES) parent_.invertLine(line);
        }
    }
    popupTime_ = parent_.menuTimeout();
}


void NuiMenuMode::navActivate() {
    clicked(cur_);
}

void NuiMenuMode::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    display();
    KontrolCallback::savePreset(source, rack, preset);
}

void NuiMenuMode::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    display();
    KontrolCallback::loadPreset(source, rack, preset);
}

void NuiMenuMode::midiLearn(Kontrol::ChangeSource src, bool b) {
    display();
    KontrolCallback::midiLearn(src, b);
}

void NuiMenuMode::modulationLearn(Kontrol::ChangeSource src, bool b) {
    display();
    KontrolCallback::modulationLearn(src, b);
}


/// main menu
enum OscMainMenuItms {
    NUI_MMI_MODULE,
    NUI_MMI_PRESET,
    NUI_MMI_MIDILEARN,
    NUI_MMI_MODLEARN,
    NUI_MMI_SAVE,
    NUI_MMI_SIZE
};

bool NuiMainMenu::init() {
    return true;
}


unsigned NuiMainMenu::getSize() {
    return (unsigned) NUI_MMI_SIZE;
}

std::string NuiMainMenu::getItemText(unsigned idx) {
    switch (idx) {
        case NUI_MMI_MODULE: {
            auto rack = model()->getRack(parent_.currentRack());
            auto module = model()->getModule(rack, parent_.currentModule());
            if (module == nullptr)
                return parent_.currentModule();
            else
                return parent_.currentModule() + ":" + module->displayName();
        }
        case NUI_MMI_PRESET: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                return rack->currentPreset();
            }
            return "No Preset";
        }
        case NUI_MMI_SAVE:
            return "Save";
        case NUI_MMI_MIDILEARN: {
            if (parent_.midiLearn()) {
                return "Midi Learn        [X]";
            }
            return "Midi Learn        [ ]";
        }
        case NUI_MMI_MODLEARN: {
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


void NuiMainMenu::clicked(unsigned idx) {
    switch (idx) {
        case NUI_MMI_MODULE: {
            parent_.changeMode(NM_MODULEMENU);
            break;
        }
        case NUI_MMI_PRESET: {
            parent_.changeMode(NM_PRESETMENU);
            break;
        }
        case NUI_MMI_MIDILEARN: {
            parent_.midiLearn(!parent_.midiLearn());
            displayItem(NUI_MMI_MIDILEARN);
            displayItem(NUI_MMI_MODLEARN);
            // parent_.changeMode(NM_PARAMETER);
            break;
        }
        case NUI_MMI_MODLEARN: {
            parent_.modulationLearn(!parent_.modulationLearn());
            displayItem(NUI_MMI_MIDILEARN);
            displayItem(NUI_MMI_MODLEARN);
            // parent_.changeMode(NM_PARAMETER);
            break;
        }
        case NUI_MMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                model()->saveSettings(Kontrol::CS_LOCAL, rack->id());
            }
            parent_.changeMode(NM_PARAMETER);
            break;
        }
        default:
            break;
    }
}

void NuiMainMenu::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) {
    display();
}

// preset menu
enum OscPresetMenuItms {
    NUI_PMI_SAVE,
    NUI_PMI_NEW,
    NUI_PMI_SEP,
    NUI_PMI_LAST
};


bool NuiPresetMenu::init() {
    return true;
}

void NuiPresetMenu::activate() {
    presets_.clear();
    auto rack = model()->getRack(parent_.currentRack());
    if (rack == nullptr) return;
    unsigned idx = 0;
    auto res = rack->getResources("preset");
    for (auto preset : res) {
        presets_.push_back(preset);
        if (preset == rack->currentPreset()) {
            cur_ = idx + 3;
            top_ = idx + 3;
        }
        idx++;
    }
    NuiMenuMode::activate();
}


unsigned NuiPresetMenu::getSize() {
    return (unsigned) NUI_PMI_LAST + presets_.size();
}

std::string NuiPresetMenu::getItemText(unsigned idx) {
    switch (idx) {
        case NUI_PMI_SAVE:
            return "Save Preset";
        case NUI_PMI_NEW:
            return "New Preset";
        case NUI_PMI_SEP:
            return "--------------------";
        default:
            return presets_[idx - NUI_PMI_LAST];
    }
}


void NuiPresetMenu::clicked(unsigned idx) {
    switch (idx) {
        case NUI_PMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->savePreset(rack->currentPreset());
            }
            parent_.changeMode(NM_PARAMETER);
            break;
        }
        case NUI_PMI_NEW: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = "new-" + std::to_string(presets_.size());
                model()->savePreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
            }
            parent_.changeMode(NM_PARAMETER);
            break;
        }
        case NUI_PMI_SEP: {
            break;
        }
        default: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = presets_[idx - NUI_PMI_LAST];
                parent_.changeMode(NM_PARAMETER);
                model()->loadPreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
            }
            break;
        }
    }
}


void NuiModuleMenu::populateMenu(const std::string &catSel) {
    auto rack = model()->getRack(parent_.currentRack());
    auto module = model()->getModule(rack, parent_.currentModule());
    if (module == nullptr) return;
    unsigned idx = 0;
    auto res = rack->getResources("module");
    items_.clear();
    cur_ = 0;
    top_ = 0;
    std::set<std::string> cats;
    unsigned catlen = cat_.length();

    if (catlen) {
        items_.push_back("..");
        idx++;
    }

    for (const auto &modtype : res) {
        if (cat_.length()) {
            size_t pos = modtype.find(cat_);
            if (pos == 0) {
                std::string mod = modtype.substr(catlen, modtype.length() - catlen);
                items_.push_back(mod);
                if (module->type() == modtype) {
                    cur_ = idx;
                    top_ = idx;
                }
                idx++;
            } // else filtered
        } else {
            // top level, so get categories
            size_t pos = modtype.find("/");
            if (pos == std::string::npos) {
                items_.push_back(modtype);
                if (modtype == module->type()) {
                    cur_ = idx;
                    top_ = idx;
                }
                idx++;
            } else {
                cats.insert(modtype.substr(0, pos + 1));
            }
        }
    }


    size_t pos = std::string::npos;
    std::string modcat;
    pos = module->type().find("/");
    if (pos != std::string::npos) {
        modcat = module->type().substr(0, pos + 1);
    }


    for (auto s: cats) {
        items_.push_back(s);
        if (catSel.length() && s == catSel) {
            cur_ = idx;
            top_ = idx;
        }
        idx++;
    }
}


void NuiModuleMenu::activate() {
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
    NuiFixedMenuMode::activate();
}


void NuiModuleMenu::clicked(unsigned idx) {
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
    parent_.changeMode(NM_PARAMETER);
}


void NuiModuleSelectMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    auto cmodule = model()->getModule(rack, parent_.currentModule());
    if (cmodule == nullptr) return;
    unsigned idx = 0;
    items_.clear();
    for (auto module : rack->getModules()) {
        std::string desc = module->id() + ":" + module->displayName();
        items_.push_back(desc);
        if (module->id() == cmodule->id()) {
            cur_ = idx;
            top_ = idx;
        }
        idx++;
    }
    NuiFixedMenuMode::activate();
}


void NuiModuleSelectMenu::clicked(unsigned idx) {
    parent_.changeMode(NM_MAINMENU);
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
