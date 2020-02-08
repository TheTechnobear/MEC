#include "TerminalTedium.h"

#include <algorithm>
#include <cstring>

#include "../../m_pd.h"

static const unsigned SCREEN_WIDTH = 21;

static const unsigned PAGE_SWITCH_TIMEOUT = 50;
static const unsigned MODULE_SWITCH_TIMEOUT = 50;
//static const int PAGE_EXIT_TIMEOUT = 5;
static const unsigned MENU_TIMEOUT = 350;


static const unsigned TT_WRITE_POLL_WAIT_TIMEOUT = 1000;



static const unsigned PARAM_DISPLAY=0;
static const unsigned MENU_DISPLAY=1;

// TODO
//
// remove popup, activateShortcut?
// split screen
// redo display?
// invert line? menu?
// check POT = 4096?

static const float MAX_POT_VALUE = 4000.0f;

static const unsigned TERMINALTEDIUM_NUM_TEXTLINES = 4;
static const unsigned TERMINALTEDIUM_NUM_PARAMS = 4;


void *terminaltedium_write_thread_func(void *aObj);

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
    //parent_.displayPopup(text, dblline);
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
    parent_.clearDisplay(PARAM_DISPLAY);

    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
    auto page = parent_.model()->getPage(module, pageId_);
//    auto pages = parent_.model()->getPages(module);
    auto params = parent_.model()->getParams(module, page);

    std::string md = "";
    std::string pd = "";
    if (module) md = module->id() + " : " + module->displayName();
    if (page) pd = page->displayName();
    parent_.displayTitle(md, pd);


    unsigned int j = 0;
    for (const auto &param : params) {
        if (param != nullptr) {
            parent_.displayParamLine(j + 1, *param);
        }
        j++;
        if (j == TERMINALTEDIUM_NUM_PARAMS) break;
    }

    parent_.flipDisplay(PARAM_DISPLAY);
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
            sprintf(msg, "knob%d", pot + 1);
            pots_->locked_[pot] = Pots::K_UNLOCKED;
            if (pots_->rawValue[pot] != rawvalue) {
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
                    assert(page!=nullptr);
                    pageId_ = page->id();
                    display();
                } catch (std::out_of_range) {
                    ;
                }
            } else {
                // if no pages, or page selected is out of range, display blank
                parent_.clearDisplay(PARAM_DISPLAY);
                std::string md = "";
                std::string pd = "";
                if (module) md = module->id() + " : " + module->displayName();
                parent_.displayTitle(md, "none");
            }
        }

        if (UI) {
            // displayPopup(page->displayName(), PAGE_SWITCH_TIMEOUT, false);
            parent_.flipDisplay(PARAM_DISPLAY);
        }

        parent_.sendPdMessage("activePage", module->id(), (page == nullptr ? "none" : page->id()));

        for (unsigned int i = 0; i < TERMINALTEDIUM_NUM_PARAMS; i++) {
            pots_->locked_[i] = Pots::K_LOCKED;
            changePot(i, pots_->rawValue[i]);
        }
    } catch (std::out_of_range) {
        ;
    }
}

void TTParamMode::selectPage(unsigned page) {
    setCurrentPage(page, true);
}


void TTParamMode::changeEncoder(unsigned enc, float value) {
    TTBaseMode::changeEncoder(enc, value);


    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
//        auto page = parent_.model()->getPage(module,pageId_);
    auto pages = parent_.model()->getPages(module);
//        auto params = parent_.model()->getParams(module,page);

    if(encoderDown_) {
        // if encoder is down, then we switch modules
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto modules = parent_.getModules(rack);
        int moduleIdx=0;
        auto cmoduleId = parent_.currentModule();
        
        for(auto m : modules) {
            if(m->id()==cmoduleId){
                moduleIdx=-1;
                break;
            }
            moduleIdx++;
        }
        
        if(moduleIdx == -1 || (moduleIdx + 1) >= modules.size()) {
            moduleIdx = 0;
        } else {
            moduleIdx++;
        }

        auto module = modules[moduleIdx];
        auto moduleId = module->id();
        if (cmoduleId != moduleId) {
            parent_.currentModule(moduleId);
            parent_.flipDisplay(PARAM_DISPLAY);
        }

    } else {
        // encoder up, so we swich pages

        if (pages.size() < 2) {
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
    } // else ! encoder down
}

void TTParamMode::encoderButton(unsigned enc, bool value) {
    encoderDown_ = value;
}


void TTParamMode::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &rack, const Kontrol::Module &) {
    if (rack.id() == parent_.currentRack()) {
        pageIdx_ = -1;
        setCurrentPage(0, false);
        parent_.flipDisplay(PARAM_DISPLAY);
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
                parent_.flipDisplay(PARAM_DISPLAY);
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
    parent_.clearDisplay(MENU_DISPLAY);
    for (unsigned i = top_; i < top_ + TERMINALTEDIUM_NUM_TEXTLINES; i++) {
        displayItem(i);
    }
}

void TTMenuMode::displayItem(unsigned i) {
    if (i < getSize()) {
        std::string item = getItemText(i);
        unsigned line = i - top_ + 1;
        parent_.displayLine(MENU_DISPLAY,line, item.c_str());
        if (i == cur_) {
            parent_.invertLine(MENU_DISPLAY,line);
        }
    }
    parent_.flipDisplay(MENU_DISPLAY);
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
            if (line <= TERMINALTEDIUM_NUM_TEXTLINES) parent_.invertLine(MENU_DISPLAY,line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= TERMINALTEDIUM_NUM_TEXTLINES) parent_.invertLine(MENU_DISPLAY,line);
            parent_.flipDisplay(MENU_DISPLAY);
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
        parent_.flipDisplay(MENU_DISPLAY);
        // parent_.changeMode(TT_PARAMETER);
        break;
    }
    case MMI_MODLEARN: {
        parent_.modulationLearn(!parent_.modulationLearn());
        displayItem(MMI_MIDILEARN);
        displayItem(MMI_MODLEARN);
        parent_.flipDisplay(MENU_DISPLAY);
        // parent_.changeMode(TT_PARAMETER);
        break;
    }
    case MMI_SAVE: {
        auto rack = model()->getRack(parent_.currentRack());
        if (rack != nullptr) {
            rack->saveSettings();
        }
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
        parent_.changeMode(TT_MAINMENU);
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
            parent_.changeMode(TT_MAINMENU);
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
                if (module->type() == modtype ) {
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


    for (auto s : cats) {
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
    cat_ = "";

    size_t pos = std::string::npos;
    pos = module->type().find("/");
    if (pos != std::string::npos) {
        cat_ = module->type().substr(0, pos + 1);
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
            if (cat_.length()) {
                // module dir
                Kontrol::EntityId modType = cat_ + modtype;
                auto rack = model()->getRack(parent_.currentRack());
                auto module = model()->getModule(rack, parent_.currentModule());
                if (modType != module->type()) {
                    //FIXME, workaround since changing the module needs to tell params to change page
                    parent_.changeMode(TT_MAINMENU);
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
    parent_.changeMode(TT_MAINMENU);
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
    parent_.changeMode(TT_MAINMENU);
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

TerminalTedium::TerminalTedium() : messageQueue_(TTMsg::MAX_N_TT_MSGS) {
}

TerminalTedium::~TerminalTedium() {
    stop();
}

void TerminalTedium::stop() {
    running_ = false;
    writer_thread_.join();
    TTMsg msg;
    while (messageQueue_.try_dequeue(msg));
    device_.stop();
}


bool TerminalTedium::init() {
    // add modes before KD init
    paramDisplay_ = std::make_shared<TTParamMode>(*this);
    addMode(TT_MAINMENU, std::make_shared<TTMainMenu>(*this));
    addMode(TT_PRESETMENU, std::make_shared<TTPresetMenu>(*this));
    addMode(TT_MODULEMENU, std::make_shared<TTModuleMenu>(*this));
    addMode(TT_MODULESELECTMENU, std::make_shared<TToduleSelectMenu>(*this));

    if (KontrolDevice::init()) {
        device_.start();
        writer_thread_ = std::thread(terminaltedium_write_thread_func, this);
        running_ = true;
        paramDisplay_->init();
        paramDisplay_->activate();


        changeMode(TT_MAINMENU);
        return true;
    }
    return false;
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

void TerminalTedium::displayParamLine(unsigned line, const Kontrol::Parameter &param) {
    std::string disp = asDisplayString(param, SCREEN_WIDTH);
    displayLine(PARAM_DISPLAY, line, disp.c_str());
}


void TerminalTedium::displayTitle(const std::string &module, const std::string &page) {
    if (module.size() == 0 || page.size() == 0) return;
    std::string title = module + " > " + page;
    displayLine(PARAM_DISPLAY,0,title);
}


void TerminalTedium::clearDisplay(unsigned display) {
    TTMsg msg(TTMsg::DISPLAY_CLEAR, display);
    messageQueue_.enqueue(msg);
}


void TerminalTedium::displayLine(unsigned display, unsigned line, const std::string& str) {
    TTMsg msg(TTMsg::DISPLAY_LINE, display, line, str.c_str(), str.size() );
    messageQueue_.enqueue(msg);
}

void TerminalTedium::flipDisplay(unsigned /*display*/) {
    TTMsg msg(TTMsg::RENDER);
    messageQueue_.enqueue(msg);
}

void TerminalTedium::invertLine(unsigned display,unsigned line) {
    ;
}




void TerminalTedium::changePot(unsigned pot, float value) {
    // we dont go to the mode
    // Kontrol::Device::rack(src, rack);
    paramDisplay_->changePot(pot,value);
}

void TerminalTedium::changeEncoder(unsigned encoder, float value) {
    // if we press up/down, we also now ignore encoder down long hold
    if(encoderMenu_) {
        encoderLongHold_=false;
        KontrolDevice::changeEncoder(encoder,value);
    } else {
        encoderLongHold_=false;
        paramDisplay_->changeEncoder(encoder,value);
    }
}

static constexpr unsigned ENCODER_HOLD_MS=1000;

void TerminalTedium::encoderButton(unsigned encoder, bool value) {
    if(value) {
        // encoder pressed down transition 
        // dont do anything yet other than record time!
        encoderDown_=std::chrono::system_clock::now();
        encoderLongHold_ = true;
    } else {
        if(encoderLongHold_) {
            std::chrono::system_clock::time_point now=std::chrono::system_clock::now();
            int dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - encoderDown_).count();
            if(dur > ENCODER_HOLD_MS) {
                // long press = switch mode
                encoderMenu_= ! encoderMenu_;
                encoderDown_= now;
                encoderLongHold_ = false;
                // dont pass on to submodes
                return; 
            } else {
                // = shot press, so end long hold
                encoderLongHold_ = false;
                encoderDown_= now;
            }
        }
    }
    // pass thru encoder up/down except if long hold
    if(encoderMenu_) {
        KontrolDevice::encoderButton(encoder,value);
    } else {
        paramDisplay_->encoderButton(encoder,value);
    }
}



// send messages to both current mode, and paramdisplay
void TerminalTedium::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    KontrolDevice::rack(src, rack);
    paramDisplay_->rack(src, rack);
}

void TerminalTedium::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    KontrolDevice::module(src, rack, module);
    paramDisplay_->module(src, rack, module);
}

void TerminalTedium::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                      const Kontrol::Module &module, const Kontrol::Page &page) {
    KontrolDevice::page(src, rack, module, page);
    paramDisplay_->page(src, rack, module, page);
}

void TerminalTedium::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                       const Kontrol::Module &module, const Kontrol::Parameter &param) {
    KontrolDevice::param(src, rack, module, param);
    paramDisplay_->param(src, rack, module, param);
}

void TerminalTedium::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                         const Kontrol::Module &module, const Kontrol::Parameter &param) {
    KontrolDevice::changed(src, rack, module, param);
    paramDisplay_->changed(src, rack, module, param);
}

void TerminalTedium::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                          const std::string &res, const std::string &value) {
    KontrolDevice::resource(src, rack, res, value);
    paramDisplay_->resource(src, rack, res, value);

}

void TerminalTedium::deleteRack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    KontrolDevice::deleteRack(src, rack);
    paramDisplay_->deleteRack(src, rack);
}

void TerminalTedium::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                              const Kontrol::Module &module) {
    KontrolDevice::activeModule(src, rack, module);
    paramDisplay_->activeModule(src, rack, module);
}

void TerminalTedium::loadModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                            const Kontrol::EntityId &modId, const std::string &modType) {
    KontrolDevice::loadModule(src, rack, modId, modType);
    paramDisplay_->loadModule(src, rack, modId, modType);
}


//================

void *terminaltedium_write_thread_func(void *aObj) {
    post("start TerminalTedium write thead");
    auto *pThis = static_cast<TerminalTedium *>(aObj);
    pThis->writePoll();
    post("TerminalTedium write thread ended");
    return nullptr;
}

void TerminalTedium::writePoll() {
    static char lines [2][6][40];
    static bool dirty [2][6];
    bool render = false;
    for (int d = 0; d < 2; d++) {
        for (int i = 0; i < 6; i++) {
            dirty[d][i] = false;
            lines[d][i][0] = 0;
        }
    }
    while (running_) {
        TTMsg msg;

        if (messageQueue_.wait_dequeue_timed(msg, std::chrono::milliseconds(TT_WRITE_POLL_WAIT_TIMEOUT))) {
            // store lines to be displayed, and render only when queue is empty
            do {
                switch (msg.type_) {
                case TTMsg::RENDER: {
                    render = true;
                    break;
                }
                case TTMsg::DISPLAY_CLEAR: {
                    // do display clear immediately, otherwise we dont know order!
                    device_.displayClear(msg.display_);
                    for (int i = 0; i < 6; i++) {
                        dirty[msg.display_][i] = false;
                        lines[msg.display_][i][0] = 0;
                    }
                    break;
                }
                case TTMsg::DISPLAY_LINE: {
                    if (msg.display_ < 2 && msg.line_ < 6) {
                        strncpy(lines[msg.display_][msg.line_], msg.buffer_, msg.size_);
                        dirty[msg.display_][msg.line_] = true;
                    }
                    break;
                }
                default: {
                    ;
                }
                } // switch
            } while (messageQueue_.try_dequeue(msg));


            if (render) {
                for (int d = 0; d < 2; d++) {
                    for (int i = 0; i < 6; i++) {
                        if (dirty[i] && lines[d][i][0] > 0) {
                            device_.displayText(d, 1, i, 0, lines[d][i]);
                            dirty[d][i] = false;
                            lines[d][i][0] = 0;
                        }
                    }
                }
                device_.displayPaint();
                render = false;
            }
        } // if wait
    } // running
}

