#include "TerminalTedium.h"

#include <algorithm>

#include "../../m_pd.h"

#include 

static const unsigned SCREEN_WIDTH = 21;

static const unsigned PAGE_SWITCH_TIMEOUT = 50;
static const unsigned MODULE_SWITCH_TIMEOUT = 50;
//static const int PAGE_EXIT_TIMEOUT = 5;
static const unsigned MENU_TIMEOUT = 350;


const int8_t PATCH_SCREEN = 3;

static const float MAX_POT_VALUE = 1023.0F;

char TerminalTedium::screenBuf_[TerminalTedium::OUTPUT_BUFFER_SIZE];

static const char *OSC_MOTHER_HOST = "127.0.0.1";
static const unsigned OSC_MOTHER_PORT = 4001;
static const unsigned MOTHER_WRITE_POLL_WAIT_TIMEOUT = 1000;

static const unsigned TERMINALTEDIUM_NUM_TEXTLINES = 4;
static const unsigned TERMINALTEDIUM_NUM_PARAMS = 4;

enum TerminalTediumModes {
    TT_PARAMETER,
    TT_MAINMENU,
    TT_PRESETMENU,
    TT_MODULEMENU,
    TT_MODULESELECTMENU
};


class TTBaseMode : public DeviceMode {
public:
    TTBaseMode(TerminalTedium &p) : parent_(p), popupTime_(-1) { ; }

    bool init() override { return true; }

    void poll() override;

    void changePot(unsigned, float) override { ; }

    void changeEncoder(unsigned, float) override { ; }

    void encoderButton(unsigned, bool) override { ; }

    void keyPress(unsigned, unsigned) override { ; }

    void selectPage(unsigned) override { ; }

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override { ; }

    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
              const Kontrol::Page &) override { ; }

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; }

    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override { ; }

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &,
                  const std::string &) override { ; };

    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void displayPopup(const std::string &text, unsigned time, bool dblline);
protected:
    TerminalTedium &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    int popupTime_;
};

struct Pots {
    enum {
        K_UNLOCKED,
        K_GT,
        K_LT,
        K_LOCKED
    } locked_[TERMINALTEDIUM_NUM_PARAMS];
    float rawValue[TERMINALTEDIUM_NUM_PARAMS];
};


class TTParamMode : public TTBaseMode {
public:
    TTParamMode(TerminalTedium &p) : TTBaseMode(p), pageIdx_(-1) { ; }

    bool init() override;
    void poll() override;
    void activate() override;
    void changePot(unsigned pot, float value) override;
    void changeEncoder(unsigned encoder, float value) override;
    void encoderButton(unsigned encoder, bool value) override;
    void keyPress(unsigned, unsigned) override;
    void selectPage(unsigned) override;


    void module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override;
    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override;
    void page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override;

    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::EntityId &,
                    const std::string &) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;

private:
    void setCurrentPage(unsigned pageIdx, bool UI);
    void display();

    void activateShortcut(unsigned key);

    std::shared_ptr<Pots> pots_;

    std::string moduleType_;
    int pageIdx_ = -1;
    Kontrol::EntityId pageId_;

    bool encoderAction_ = false;
    bool encoderDown_ = false;
};

class TTMenuMode : public TTBaseMode {
public:
    TTMenuMode(TerminalTedium &p) : TTBaseMode(p), cur_(0), top_(0) { ; }

    virtual unsigned getSize() = 0;
    virtual std::string getItemText(unsigned idx) = 0;
    virtual void clicked(unsigned idx) = 0;

    bool init() override { return true; }

    void poll() override;
    void activate() override;
    void changeEncoder(unsigned encoder, float value) override;
    void encoderButton(unsigned encoder, bool value) override;

protected:
    void display();
    void displayItem(unsigned idx);
    unsigned cur_;
    unsigned top_;
    bool clickedDown_;
};


class TTFixedMenuMode : public TTMenuMode {
public:
    TTFixedMenuMode(TerminalTedium &p) : TTMenuMode(p) { ; }

    unsigned getSize() override { return static_cast<unsigned int>(items_.size()); };

    std::string getItemText(unsigned i) override { return items_[i]; }

protected:
    std::vector<std::string> items_;
};

class TTMainMenu : public TTMenuMode {
public:
    TTMainMenu(TerminalTedium &p) : TTMenuMode(p) { ; }

    bool init() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
};

class TTPresetMenu : public TTMenuMode {
public:
    TTPresetMenu(TerminalTedium &p) : TTMenuMode(p) { ; }

    bool init() override;
    void activate() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
private:
    std::vector<std::string> presets_;
};


class TTModuleMenu : public TTFixedMenuMode {
public:
    TTModuleMenu(TerminalTedium &p) : TTFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
private:
    void populateMenu(const std::string& catSel);
    std::string cat_;
};

class TToduleSelectMenu : public TTFixedMenuMode {
public:
    TToduleSelectMenu(TerminalTedium &p) : TTFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};


void TTBaseMode::displayPopup(const std::string &text, unsigned time, bool dblline) {
    popupTime_ = time;
    parent_.displayPopup(text, dblline);
}

void TTBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

bool TTParamMode::init() {
    TTBaseMode::init();
    pots_ = std::make_shared<Pots>();
    for (int i = 0; i < TERMINALTEDIUM_NUM_PARAMS; i++) {
        pots_->rawValue[i] = std::numeric_limits<float>::max();
        pots_->locked_[i] = Pots::K_LOCKED;
    }
    return true;
}

void TTParamMode::display() {
    parent_.clearDisplay();

    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
    auto page = parent_.model()->getPage(module, pageId_);
//    auto pages = parent_.model()->getPages(module);
    auto params = parent_.model()->getParams(module, page);


    unsigned int j = 0;
    for (const auto &param : params) {
        if (param != nullptr) {
            parent_.displayParamLine(j + 1, *param);
        }
        j++;
        if (j == TERMINALTEDIUM_NUM_PARAMS) break;
    }

    parent_.flipDisplay();
}


void TTParamMode::activate() {
    auto module = model()->getModule(model()->getRack(parent_.currentRack()), parent_.currentModule());
    if (module != nullptr) {
        auto page = parent_.model()->getPage(module, pageId_);
        parent_.sendPdMessage("activePage", module->id(), (page == nullptr ? "none" : page->id()));
    }
    display();
}


void TTParamMode::poll() {
    TTBaseMode::poll();
    // release pop, redraw display
    if (popupTime_ == 0) {
        display();

        // cancel timing
        popupTime_ = -1;
    }
}

void TTParamMode::changePot(unsigned pot, float rawvalue) {
    TTBaseMode::changePot(pot, rawvalue);
    try {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
        auto page = parent_.model()->getPage(module, pageId_);
        auto pages = parent_.model()->getPages(module);

        if (pages.size() == 0 || (page && page->isCustomPage())) {
            // a page with no parameters is a custom page
            // and recieves pot events
            char msg[7];
            sprintf(msg, "knob%d",pot+1);
            pots_->locked_[pot] = Pots::K_UNLOCKED;
            if(pots_->rawValue[pot]!=rawvalue) {
                float value = rawvalue / MAX_POT_VALUE;
                parent_.sendPdModuleMessage(msg, module->id(), ( page == nullptr ? "none" : page->id() ) , value);
            }
            pots_->rawValue[pot] = rawvalue;
            return;
        }


//        auto pages = parent_.model()->getPages(module);
        auto params = parent_.model()->getParams(module, page);

        if (pot >= params.size()) return;

        auto &param = params[pot];
        auto paramId = param->id();

        Kontrol::ParamValue calc;

        if (rawvalue != std::numeric_limits<float>::max()) {
            float value = rawvalue / MAX_POT_VALUE;
            calc = param->calcFloat(value);
            //std::cerr << "changePot " << pot << " " << value << " cv " << calc.floatValue() << " pv " << param->current().floatValue() << std::endl;
        }

        pots_->rawValue[pot] = rawvalue;


        if (pots_->locked_[pot] != Pots::K_UNLOCKED) {
            //if pot is locked, determined if we can unlock it
            if (calc == param->current()) {
                pots_->locked_[pot] = Pots::K_UNLOCKED;
                //std::cerr << "unlock condition met == " << pot << std::endl;
            } else if (pots_->locked_[pot] == Pots::K_GT) {
                if (calc > param->current()) {
                    pots_->locked_[pot] = Pots::K_UNLOCKED;
                    //std::cerr << "unlock condition met gt " << pot << std::endl;
                }
            } else if (pots_->locked_[pot] == Pots::K_LT) {
                if (calc < param->current()) {
                    pots_->locked_[pot] = Pots::K_UNLOCKED;
                    //std::cerr << "unlock condition met lt " << pot << std::endl;
                }
            } else if (pots_->locked_[pot] == Pots::K_LOCKED) {
                //std::cerr << "pot locked " << pot << " pv " << param->current().floatValue() << " cv " << calc.floatValue() << std::endl;
                // initial locked, determine unlock condition
                if (calc == param->current()) {
                    // pot value at current value, unlock it
                    pots_->locked_[pot] = Pots::K_UNLOCKED;
                    //std::cerr << "set unlock condition == " << pot << std::endl;
                } else if (rawvalue == std::numeric_limits<float>::max()) {
                    // stay locked , we need a real value ;)
                    // init state
                    //std::cerr << "cannot set unlock condition " << pot << std::endl;
                } else if (calc > param->current()) {
                    // pot starts greater than param, so wait for it to go less than
                    pots_->locked_[pot] = Pots::K_LT;
                    //std::cerr << "set unlock condition lt " << pot << std::endl;
                } else {
                    // pot starts less than param, so wait for it to go greater than
                    pots_->locked_[pot] = Pots::K_GT;
                    //std::cerr << "set unlock condition gt " << pot << std::endl;
                }
            }
        }

        if (pots_->locked_[pot] == Pots::K_UNLOCKED) {
            model()->changeParam(Kontrol::CS_LOCAL, parent_.currentRack(), parent_.currentModule(), paramId, calc);
        }
    } catch (std::out_of_range) {
        return;
    }

}

void TTParamMode::setCurrentPage(unsigned pageIdx, bool UI) {
    auto module = model()->getModule(model()->getRack(parent_.currentRack()), parent_.currentModule());
    if (module == nullptr) return;

    try {
//        auto rack = parent_.model()->getRack(parent_.currentRack());
//        auto module = parent_.model()->getModule(rack, parent_.currentModule());
        auto page = parent_.model()->getPage(module, pageId_);
        auto pages = parent_.model()->getPages(module);
//        auto params = parent_.model()->getParams(module,page);

        if (pageIdx_ != pageIdx) {
            if (pageIdx < pages.size()) {
                pageIdx_ = pageIdx;

                try {
                    page = pages[pageIdx_];
                    pageId_ = page->id();
                    display();
                } catch (std::out_of_range) { ;
                }
            } else {
                // if no pages, or page selected is out of range, display blank
                parent_.clearDisplay();
            }
        }

        if (UI) {
            displayPopup(page->displayName(), PAGE_SWITCH_TIMEOUT, false);
            parent_.flipDisplay();
        }

        parent_.sendPdMessage("activePage", module->id(), (page == nullptr ? "none" : page->id()));

        for (unsigned int i = 0; i < TERMINALTEDIUM_NUM_PARAMS; i++) {
            pots_->locked_[i] = Pots::K_LOCKED;
            changePot(i, pots_->rawValue[i]);
        }
    } catch (std::out_of_range) { ;
    }
}

void TTParamMode::selectPage(unsigned page) {
    setCurrentPage(page,true);
}


void TTParamMode::changeEncoder(unsigned enc, float value) {
    TTBaseMode::changeEncoder(enc, value);


    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
//        auto page = parent_.model()->getPage(module,pageId_);
    auto pages = parent_.model()->getPages(module);
//        auto params = parent_.model()->getParams(module,page);

    if (pages.size()<2) {
        // if single page send encoder messages to modules
        parent_.sendPdModuleMessage("enc", module->id(), value);
        return;
    }

    if (pageIdx_ < 0) {
        setCurrentPage(0, false);
        return;
    }


    auto pagenum = (unsigned) pageIdx_;

    if (value > 0) {
        // clockwise
        pagenum++;
        pagenum = std::min(pagenum, (unsigned) pages.size() - 1);
    } else {
        // anti clockwise
        if (pagenum > 0) pagenum--;
    }

    if (pagenum != pageIdx_) {
        setCurrentPage(pagenum, true);
    }
}

void TTParamMode::encoderButton(unsigned enc, bool value) {
    TTBaseMode::encoderButton(enc, value);

    if(parent_.enableMenu()) {
        if (encoderAction_ && !value) {
            parent_.changeMode(TT_MAINMENU);
        }
    } else {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
        parent_.sendPdModuleMessage("encbut", module->id(), value);
    }

    encoderDown_ = value;
    encoderAction_ = value;
}


void TTParamMode::keyPress(unsigned key, unsigned value) {
    if (value == 0 && encoderDown_) {
        activateShortcut(key);
        encoderAction_ = false;
    }
}

void TTParamMode::activateShortcut(unsigned key) {
    if (key == 0) {
        if(parent_.enableMenu()) {
            // normal op = select menu
            encoderDown_ = false;
            encoderAction_ = false;
            parent_.changeMode(TT_MODULESELECTMENU);
            return;
        } else {
            //TODO backcompat mode
            //this is not good, as it will obliterate screen
            //and also will mean encoder cannot be used again
            //but it necessary otherwise you cannot change module
            //(also changing module means we need to have the menu enabled again)

            // re-enable main menu
            encoderDown_ = false;
            encoderAction_ = false;
            parent_.enableMenu(true);
            parent_.changeMode(TT_MAINMENU);
        }
    }

    if (key > 0) {
        unsigned moduleIdx = key - 1;
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto modules = parent_.getModules(rack);
        if (moduleIdx < modules.size()) {
            auto module = modules[moduleIdx];
            auto moduleId = module->id();
            if (parent_.currentModule() != moduleId) {
                // re-enable main menu
                parent_.enableMenu(true);

                parent_.currentModule(moduleId);
                displayPopup(module->id() + ":" + module->displayName(), MODULE_SWITCH_TIMEOUT, true);
                parent_.flipDisplay();
            }
        }
    }
}

void TTParamMode::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &rack, const Kontrol::Module &) {
    if (rack.id() == parent_.currentRack()) {
        pageIdx_ = -1;
        setCurrentPage(0, false);
        parent_.flipDisplay();
    }
}


void TTParamMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                         const Kontrol::Parameter &param) {
    TTBaseMode::changed(src, rack, module, param);
    if (popupTime_ > 0) return;

    if (rack.id() != parent_.currentRack() || module.id() != parent_.currentModule()) return;

    auto prack = parent_.model()->getRack(parent_.currentRack());
    auto pmodule = parent_.model()->getModule(prack, parent_.currentModule());
    auto page = parent_.model()->getPage(pmodule, pageId_);
//    auto pages = parent_.model()->getPages(pmodule);
    auto params = parent_.model()->getParams(pmodule, page);


    auto sz = static_cast<unsigned int>(params.size());
    sz = sz < TERMINALTEDIUM_NUM_PARAMS ? sz : TERMINALTEDIUM_NUM_PARAMS;
    for (unsigned int i = 0; i < sz; i++) {
        try {
            auto &p = params.at(i);
            if (p->id() == param.id()) {
                p->change(param.current(), src == Kontrol::CS_PRESET);
                parent_.displayParamLine(i + 1, param);
                if (src != Kontrol::CS_LOCAL) {
                    //std::cerr << "locking " << param.id() << " src " << src << std::endl;
                    pots_->locked_[i] = Pots::K_LOCKED;
                    changePot(i, pots_->rawValue[i]);
                }
                parent_.flipDisplay();
                return;
            }
        } catch (std::out_of_range) {
            return;
        }
    } // for
}

void TTParamMode::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    TTBaseMode::module(source, rack, module);
    if (moduleType_ != module.type()) {
        pageIdx_ = -1;
    }
    moduleType_ = module.type();
}

void TTParamMode::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                      const Kontrol::Page &page) {
    TTBaseMode::page(source, rack, module, page);
    if (pageIdx_ < 0) setCurrentPage(0, false);
}


void TTParamMode::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                            const Kontrol::EntityId &moduleId, const std::string &modType) {
    TTBaseMode::loadModule(source, rack, moduleId, modType);
    if (parent_.currentModule() == moduleId) {
        if (moduleType_ != modType) {
            pageIdx_ = -1;
            moduleType_ = modType;
        }
    }
}

void TTMenuMode::activate() {
    parent_.sendPdMessage("activePage", "none","none");
    display();
    popupTime_ = MENU_TIMEOUT;
    clickedDown_ = false;
}

void TTMenuMode::poll() {
    TTBaseMode::poll();
    if (popupTime_ == 0) {
        parent_.changeMode(TT_PARAMETER);
        popupTime_ = -1;
    }
}

void TTMenuMode::display() {
    parent_.clearDisplay();
    for (unsigned i = top_; i < top_ + TERMINALTEDIUM_NUM_TEXTLINES; i++) {
        displayItem(i);
    }
}

void TTMenuMode::displayItem(unsigned i) {
    if (i < getSize()) {
        std::string item = getItemText(i);
        unsigned line = i - top_ + 1;
        parent_.displayLine(line, item.c_str());
        if (i == cur_) {
            parent_.invertLine(line);
        }
    }
    parent_.flipDisplay();
}


void TTMenuMode::changeEncoder(unsigned, float value) {
    unsigned cur = cur_;
    if (value > 0) {
        // clockwise
        cur++;
        cur = std::min(cur, getSize() - 1);
    } else {
        // anti clockwise
        if (cur > 0) cur--;
    }
    if (cur != cur_) {
        unsigned int line = 0;
        if (cur < top_) {
            top_ = cur;
            cur_ = cur;
            display();
        } else if (cur >= top_ + TERMINALTEDIUM_NUM_TEXTLINES) {
            top_ = cur - (TERMINALTEDIUM_NUM_TEXTLINES - 1);
            cur_ = cur;
            display();
        } else {
            line = cur_ - top_ + 1;
            if (line <= TERMINALTEDIUM_NUM_TEXTLINES) parent_.invertLine(line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= TERMINALTEDIUM_NUM_TEXTLINES) parent_.invertLine(line);
            parent_.flipDisplay();
        }
    }
    popupTime_ = MENU_TIMEOUT;
}


void TTMenuMode::encoderButton(unsigned, bool value) {
    if (clickedDown_ && value < 1.0) {
        clicked(cur_);
    }
    clickedDown_ = value;
}


/// main menu
enum MainMenuItms {
    MMI_MODULE,
    MMI_PRESET,
    MMI_MIDILEARN,
    MMI_MODLEARN,
    MMI_SAVE,
    MMI_SIZE
};

bool TTMainMenu::init() {
    return true;
}


unsigned TTMainMenu::getSize() {
    return (unsigned) MMI_SIZE;
}

std::string TTMainMenu::getItemText(unsigned idx) {
    switch (idx) {
        case MMI_MODULE: {
            auto rack = model()->getRack(parent_.currentRack());
            auto module = model()->getModule(rack, parent_.currentModule());
            if (module == nullptr)
                return parent_.currentModule();
            else
                return parent_.currentModule() + ":" + module->displayName();
        }
        case MMI_PRESET: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                return rack->currentPreset();
            }
            return "No Preset";
        }
        case MMI_SAVE:
            return "Save";
        case MMI_MIDILEARN: {
            if (parent_.midiLearn()) {
                return "Midi Learn        [X]";
            }
            return "Midi Learn        [ ]";
        }
        case MMI_MODLEARN: {
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


void TTMainMenu::clicked(unsigned idx) {
    switch (idx) {
        case MMI_MODULE: {
            parent_.changeMode(TT_MODULEMENU);
            break;
        }
        case MMI_PRESET: {
            parent_.changeMode(TT_PRESETMENU);
            break;
        }
        case MMI_MIDILEARN: {
            parent_.midiLearn(!parent_.midiLearn());
            displayItem(MMI_MIDILEARN);
            displayItem(MMI_MODLEARN);
            parent_.flipDisplay();
            // parent_.changeMode(TT_PARAMETER);
            break;
        }
        case MMI_MODLEARN: {
            parent_.modulationLearn(!parent_.modulationLearn());
            displayItem(MMI_MIDILEARN);
            displayItem(MMI_MODLEARN);
            parent_.flipDisplay();
            // parent_.changeMode(TT_PARAMETER);
            break;
        }
        case MMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->saveSettings();
            }
            parent_.changeMode(TT_PARAMETER);
            break;
        }
        default:
            break;
    }
}

// preset menu
enum PresetMenuItms {
    PMI_SAVE,
    PMI_NEW,
    PMI_SEP,
    PMI_LAST
};


bool TTPresetMenu::init() {
    auto rack = model()->getRack(parent_.currentRack());
    if (rack != nullptr) {
        presets_ = rack->getPresetList();
    } else {
        presets_.clear();
    }
    return true;
}

void TTPresetMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    if (rack != nullptr) {
        presets_ = rack->getPresetList();
    } else {
        presets_.clear();
    }
    TTMenuMode::activate();
}


unsigned TTPresetMenu::getSize() {
    return static_cast<unsigned int>((unsigned) PMI_LAST + presets_.size());
}

std::string TTPresetMenu::getItemText(unsigned idx) {
    switch (idx) {
        case PMI_SAVE:
            return "Save Preset";
        case PMI_NEW:
            return "New Preset";
        case PMI_SEP:
            return "--------------------";
        default:
            return presets_[idx - PMI_LAST];
    }
}


void TTPresetMenu::clicked(unsigned idx) {
    switch (idx) {
        case PMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->savePreset(rack->currentPreset());
            }
            parent_.changeMode(TT_PARAMETER);
            break;
        }
        case PMI_NEW: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = "new-" + std::to_string(presets_.size());
                rack->savePreset(newPreset);
            }
            parent_.changeMode(TT_MAINMENU);
            break;
        }
        case PMI_SEP: {
            break;
        }
        default: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = presets_[idx - PMI_LAST];
                parent_.changeMode(TT_PARAMETER);
                rack->loadPreset(newPreset);
            }
            break;
        }
    }
}

void TTModuleMenu::populateMenu(const std::string& catSel) {
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

    if(catlen) {
        items_.push_back("..");
        idx++;
    }

    for (const auto &modtype : res) {
        if(cat_.length()) {
            size_t pos=modtype.find(cat_);
            if(pos==0) {
               std::string mod=modtype.substr(catlen,modtype.length()-catlen);
                items_.push_back(mod);
                if (module->type() == modtype ) {
                    cur_ = idx;
                    top_ = idx;
                }
                idx++;
            } // else filtered
        } else {
            // top level, so get categories
            size_t pos=modtype.find("/");
            if(pos==std::string::npos) {
                items_.push_back(modtype);
                if (modtype == module->type()) {
                    cur_ = idx;
                    top_ = idx;
                }
                idx++;
            } else {
                cats.insert(modtype.substr(0,pos+1));
            }
        }
    }


    size_t pos =std::string::npos;
    std::string modcat;
    pos = module->type().find("/");
    if(pos!=std::string::npos) {
        modcat=module->type().substr(0,pos+1);
    }


    for(auto s: cats) {
        items_.push_back(s);
        if (catSel.length() && s == catSel) {
            cur_ = idx;
            top_ = idx;
        }
        idx++;
    }
}


void TTModuleMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    auto module = model()->getModule(rack, parent_.currentModule());
    if (module == nullptr) return;
    unsigned idx = 0;
    auto res = rack->getResources("module");
    cat_="";

    size_t pos =std::string::npos;
    pos = module->type().find("/");
    if(pos!=std::string::npos) {
        cat_=module->type().substr(0,pos+1);
    }

    populateMenu(cat_);

    TTFixedMenuMode::activate();
}


void TTModuleMenu::clicked(unsigned idx) {
    if (idx < getSize()) {
        auto modtype = items_[idx];
        if (modtype == "..") {
            std::string oldcat = cat_;
            cat_ = "";
            populateMenu(oldcat);
            display();
            return;
        } else {
            if(cat_.length()) {
                // module dir
                Kontrol::EntityId modType = cat_ + modtype;
                auto rack = model()->getRack(parent_.currentRack());
                auto module = model()->getModule(rack, parent_.currentModule());
                if (modType != module->type()) {
                    //FIXME, workaround since changing the module needs to tell params to change page
                    parent_.changeMode(TT_PARAMETER);
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
    parent_.changeMode(TT_PARAMETER);
}


void TToduleSelectMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    auto cmodule = model()->getModule(rack, parent_.currentModule());
    if (cmodule == nullptr) return;
    unsigned idx = 0;
    items_.clear();
    auto modules = parent_.getModules(rack);
    for (const auto &module : modules) {
        std::string desc = module->id() + ":" + module->displayName();
        items_.push_back(desc);
        if (module->id() == cmodule->id()) {
            cur_ = idx;
            top_ = idx;
        }
        idx++;
    }
    TTFixedMenuMode::activate();
}


void TToduleSelectMenu::clicked(unsigned idx) {
    parent_.changeMode(TT_PARAMETER);
    if (idx < getSize()) {
        unsigned moduleIdx = idx;
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto modules = parent_.getModules(rack);
        if (moduleIdx < modules.size()) {
            auto module = modules[moduleIdx];
            auto moduleId = module->id();
            if (parent_.currentModule() != moduleId) {
                parent_.currentModule(moduleId);
            }
        }
    }
}



// TerminalTedium implmentation

TerminalTedium::TerminalTedium() {
}

TerminalTedium::~TerminalTedium() {
    stop();
}

void TerminalTedium::stop() {
    device_.stop();
}


bool TerminalTedium::init() {
    // add modes before KD init
    addMode(TT_PARAMETER, std::make_shared<TTParamMode>(*this));
    addMode(TT_MAINMENU, std::make_shared<TTMainMenu>(*this));
    addMode(TT_PRESETMENU, std::make_shared<TTPresetMenu>(*this));
    addMode(TT_MODULEMENU, std::make_shared<TTModuleMenu>(*this));
    addMode(TT_MODULESELECTMENU, std::make_shared<TToduleSelectMenu>(*this));

    if (KontrolDevice::init()) {
        device_.start();

        changeMode(TT_PARAMETER);
        return true;
    }
    return false;
}


void TerminalTedium::displayPopup(const std::string &text, bool dblline) {
    if (dblline) {
        device_.clearRect(0, 0, 2, 12, 118,38);
        // device_.drawRect(0, 1, 2, 12, 118,38);
    } else {
        device_.clearRect(0, 0, 4, 14, 114,34);
    }
    // device_.drawRect(0, 1, 4, 14, 114,34);

    device_.drawText(0, 1, 10,24, text);
}


std::string TerminalTedium::asDisplayString(const Kontrol::Parameter &param, unsigned width) const {
    std::string pad;
    std::string ret;
    std::string value = param.displayValue();
    std::string unit = std::string(param.displayUnit() + "  ").substr(0, 2);
    const std::string &dName = param.displayName();
    unsigned long fillc = width - (dName.length() + value.length() + 1 + unit.length());
    for (; fillc > 0; fillc--) pad += " ";
    ret = dName + pad + value + " " + unit;
    if (ret.length() > width) ret = ret.substr(width - ret.length(), width);
    return ret;
}

void TerminalTedium::clearDisplay() {
    device_.displayClear(0);
}

void TerminalTedium::displayParamLine(unsigned line, const Kontrol::Parameter &param) {
    std::string disp = asDisplayString(param, SCREEN_WIDTH);
    displayLine(line, disp.c_str());
}

void TerminalTedium::displayLine(unsigned line, const char *disp) {
    device_.displayText(0, 1, line, 0, disp);
}

void TerminalTedium::invertLine(unsigned line) {
    device_.invertLine(0,line);
}

void TerminalTedium::flipDisplay() {
    device_.displayPaint();
}

