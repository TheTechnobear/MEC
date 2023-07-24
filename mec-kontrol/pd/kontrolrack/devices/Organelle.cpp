#include "Organelle.h"

#include <osc/OscOutboundPacketStream.h>
//#include <cmath>
#include <algorithm>

#include "../../m_pd.h"


static const unsigned SCREEN_WIDTH = 21;

static const unsigned PAGE_SWITCH_TIMEOUT = 50;
static const unsigned MODULE_SWITCH_TIMEOUT = 50;
//static const int PAGE_EXIT_TIMEOUT = 5;
static const unsigned MENU_TIMEOUT = 350;


const int8_t PATCH_SCREEN = 3;

static const float MAX_POT_VALUE = 1023.0F;

char Organelle::screenBuf_[Organelle::OUTPUT_BUFFER_SIZE];

static const char *OSC_MOTHER_HOST = "127.0.0.1";
static const unsigned OSC_MOTHER_PORT = 4001;
static const unsigned MOTHER_WRITE_POLL_WAIT_TIMEOUT = 1000;

static const unsigned ORGANELLE_NUM_TEXTLINES = 4;
static const unsigned ORGANELLE_NUM_PARAMS = 4;

enum OrganelleModes {
    OM_PARAMETER,
    OM_MAINMENU,
    OM_PRESETMENU,
    OM_MODULEMENU,
    OM_MODULESELECTMENU
};


class OBaseMode : public DeviceMode {
public:
    OBaseMode(Organelle &p) : parent_(p), popupTime_(-1) { ; }

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
    Organelle &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    int popupTime_;
};

struct Pots {
    enum {
        K_UNLOCKED,
        K_GT,
        K_LT,
        K_LOCKED
    } locked_[ORGANELLE_NUM_PARAMS];
    float rawValue[ORGANELLE_NUM_PARAMS];
};


class OParamMode : public OBaseMode {
public:
    OParamMode(Organelle &p) : OBaseMode(p), pageIdx_(-1) { ; }

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

class OMenuMode : public OBaseMode {
public:
    OMenuMode(Organelle &p) : OBaseMode(p), cur_(0), top_(0) { ; }

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


class OFixedMenuMode : public OMenuMode {
public:
    OFixedMenuMode(Organelle &p) : OMenuMode(p) { ; }

    unsigned getSize() override { return static_cast<unsigned int>(items_.size()); };

    std::string getItemText(unsigned i) override { return items_[i]; }

protected:
    std::vector<std::string> items_;
};

class OMainMenu : public OMenuMode {
public:
    OMainMenu(Organelle &p) : OMenuMode(p) { ; }

    bool init() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
};

class OPresetMenu : public OMenuMode {
public:
    OPresetMenu(Organelle &p) : OMenuMode(p) { ; }

    bool init() override;
    void activate() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
private:
    std::vector<std::string> presets_;
};


class OModuleMenu : public OFixedMenuMode {
public:
    OModuleMenu(Organelle &p) : OFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
private:
    void populateMenu(const std::string& catSel);
    std::string cat_;
};

class OModuleSelectMenu : public OFixedMenuMode {
public:
    OModuleSelectMenu(Organelle &p) : OFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};


void OBaseMode::displayPopup(const std::string &text, unsigned time, bool dblline) {
    popupTime_ = time;
    parent_.displayPopup(text, dblline);
}

void OBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

bool OParamMode::init() {
    OBaseMode::init();
    pots_ = std::make_shared<Pots>();
    for (int i = 0; i < ORGANELLE_NUM_PARAMS; i++) {
        pots_->rawValue[i] = std::numeric_limits<float>::max();
        pots_->locked_[i] = Pots::K_LOCKED;
    }
    return true;
}

void OParamMode::display() {
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
        if (j == ORGANELLE_NUM_PARAMS) break;
    }

    parent_.flipDisplay();
}


void OParamMode::activate() {
    auto module = model()->getModule(model()->getRack(parent_.currentRack()), parent_.currentModule());
    if (module != nullptr) {
        auto page = parent_.model()->getPage(module, pageId_);
        parent_.sendPdMessage("activePage", module->id(), (page == nullptr ? "none" : page->id()));
    }
    display();
}


void OParamMode::poll() {
    OBaseMode::poll();
    // release pop, redraw display
    if (popupTime_ == 0) {
        display();

        // cancel timing
        popupTime_ = -1;
    }
}

void OParamMode::changePot(unsigned pot, float rawvalue) {
    OBaseMode::changePot(pot, rawvalue);
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
            if (rawvalue != pots_->rawValue[pot]) {
                pots_->locked_[pot] = Pots::K_UNLOCKED;
            }
            //std::cerr << "changePot " << pot << " " << value << " cv " << calc.floatValue() << " pv " << param->current().floatValue() << std::endl;
        }

        pots_->rawValue[pot] = rawvalue;


        // if (pots_->locked_[pot] != Pots::K_UNLOCKED) {
        //     //if pot is locked, determined if we can unlock it
        //     if (calc == param->current()) {
        //         pots_->locked_[pot] = Pots::K_UNLOCKED;
        //         //std::cerr << "unlock condition met == " << pot << std::endl;
        //     } else if (pots_->locked_[pot] == Pots::K_GT) {
        //         if (calc > param->current()) {
        //             pots_->locked_[pot] = Pots::K_UNLOCKED;
        //             //std::cerr << "unlock condition met gt " << pot << std::endl;
        //         }
        //     } else if (pots_->locked_[pot] == Pots::K_LT) {
        //         if (calc < param->current()) {
        //             pots_->locked_[pot] = Pots::K_UNLOCKED;
        //             //std::cerr << "unlock condition met lt " << pot << std::endl;
        //         }
        //     } else if (pots_->locked_[pot] == Pots::K_LOCKED) {
        //         //std::cerr << "pot locked " << pot << " pv " << param->current().floatValue() << " cv " << calc.floatValue() << std::endl;
        //         // initial locked, determine unlock condition
        //         if (calc == param->current()) {
        //             // pot value at current value, unlock it
        //             pots_->locked_[pot] = Pots::K_UNLOCKED;
        //             //std::cerr << "set unlock condition == " << pot << std::endl;
        //         } else if (rawvalue == std::numeric_limits<float>::max()) {
        //             // stay locked , we need a real value ;)
        //             // init state
        //             //std::cerr << "cannot set unlock condition " << pot << std::endl;
        //         } else if (calc > param->current()) {
        //             // pot starts greater than param, so wait for it to go less than
        //             pots_->locked_[pot] = Pots::K_LT;
        //             //std::cerr << "set unlock condition lt " << pot << std::endl;
        //         } else {
        //             // pot starts less than param, so wait for it to go greater than
        //             pots_->locked_[pot] = Pots::K_GT;
        //             //std::cerr << "set unlock condition gt " << pot << std::endl;
        //         }
        //     }
        // }

        if (pots_->locked_[pot] == Pots::K_UNLOCKED) {
            model()->changeParam(Kontrol::CS_LOCAL, parent_.currentRack(), parent_.currentModule(), paramId, calc);
        }
    } catch (std::out_of_range) {
        return;
    }

}

void OParamMode::setCurrentPage(unsigned pageIdx, bool UI) {
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

        for (unsigned int i = 0; i < ORGANELLE_NUM_PARAMS; i++) {
            pots_->locked_[i] = Pots::K_LOCKED;
            changePot(i, pots_->rawValue[i]);
        }
    } catch (std::out_of_range) { ;
    }
}

void OParamMode::selectPage(unsigned page) {
    setCurrentPage(page,true);
}


void OParamMode::changeEncoder(unsigned enc, float value) {
    OBaseMode::changeEncoder(enc, value);


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

void OParamMode::encoderButton(unsigned enc, bool value) {
    OBaseMode::encoderButton(enc, value);

    if(parent_.enableMenu()) {
        if (encoderAction_ && !value) {
            parent_.changeMode(OM_MAINMENU);
        }
    } else {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
        parent_.sendPdModuleMessage("encbut", module->id(), value);
    }

    encoderDown_ = value;
    encoderAction_ = value;
}


void OParamMode::keyPress(unsigned key, unsigned value) {
    if (value == 0 && encoderDown_) {
        activateShortcut(key);
        encoderAction_ = false;
    }
}

void OParamMode::activateShortcut(unsigned key) {
    if (key == 0) {
        if(parent_.enableMenu()) {
            // normal op = select menu
            encoderDown_ = false;
            encoderAction_ = false;
            parent_.changeMode(OM_MODULESELECTMENU);
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
            parent_.changeMode(OM_MAINMENU);
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

void OParamMode::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &rack, const Kontrol::Module &) {
    if (rack.id() == parent_.currentRack()) {
        pageIdx_ = -1;
        setCurrentPage(0, false);
        parent_.flipDisplay();
    }
}


void OParamMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                         const Kontrol::Parameter &param) {
    OBaseMode::changed(src, rack, module, param);
    if (popupTime_ > 0) return;

    if (rack.id() != parent_.currentRack() || module.id() != parent_.currentModule()) return;

    auto prack = parent_.model()->getRack(parent_.currentRack());
    auto pmodule = parent_.model()->getModule(prack, parent_.currentModule());
    auto page = parent_.model()->getPage(pmodule, pageId_);
//    auto pages = parent_.model()->getPages(pmodule);
    auto params = parent_.model()->getParams(pmodule, page);


    auto sz = static_cast<unsigned int>(params.size());
    sz = sz < ORGANELLE_NUM_PARAMS ? sz : ORGANELLE_NUM_PARAMS;
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

void OParamMode::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    OBaseMode::module(source, rack, module);
    if (moduleType_ != module.type()) {
        pageIdx_ = -1;
    }
    moduleType_ = module.type();
}

void OParamMode::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                      const Kontrol::Page &page) {
    OBaseMode::page(source, rack, module, page);
    if (pageIdx_ < 0) setCurrentPage(0, false);
}


void OParamMode::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                            const Kontrol::EntityId &moduleId, const std::string &modType) {
    OBaseMode::loadModule(source, rack, moduleId, modType);
    if (parent_.currentModule() == moduleId) {
        if (moduleType_ != modType) {
            pageIdx_ = -1;
            moduleType_ = modType;
        }
    }
}

void OMenuMode::activate() {
    parent_.sendPdMessage("activePage", "none","none");
    display();
    popupTime_ = MENU_TIMEOUT;
    clickedDown_ = false;
}

void OMenuMode::poll() {
    OBaseMode::poll();
    if (popupTime_ == 0) {
        parent_.changeMode(OM_PARAMETER);
        popupTime_ = -1;
    }
}

void OMenuMode::display() {
    parent_.clearDisplay();
    for (unsigned i = top_; i < top_ + ORGANELLE_NUM_TEXTLINES; i++) {
        displayItem(i);
    }
}

void OMenuMode::displayItem(unsigned i) {
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


void OMenuMode::changeEncoder(unsigned, float value) {
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
        } else if (cur >= top_ + ORGANELLE_NUM_TEXTLINES) {
            top_ = cur - (ORGANELLE_NUM_TEXTLINES - 1);
            cur_ = cur;
            display();
        } else {
            line = cur_ - top_ + 1;
            if (line <= ORGANELLE_NUM_TEXTLINES) parent_.invertLine(line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= ORGANELLE_NUM_TEXTLINES) parent_.invertLine(line);
            parent_.flipDisplay();
        }
    }
    popupTime_ = MENU_TIMEOUT;
}


void OMenuMode::encoderButton(unsigned, bool value) {
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
    MMI_HOME,
    MMI_SIZE
};

bool OMainMenu::init() {
    return true;
}


unsigned OMainMenu::getSize() {
    return (unsigned) MMI_SIZE;
}

std::string OMainMenu::getItemText(unsigned idx) {
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
        case MMI_HOME:
            return "Home";
        default:
            break;
    }
    return "";
}


void OMainMenu::clicked(unsigned idx) {
    switch (idx) {
        case MMI_MODULE: {
            parent_.changeMode(OM_MODULEMENU);
            break;
        }
        case MMI_PRESET: {
            parent_.changeMode(OM_PRESETMENU);
            break;
        }
        case MMI_MIDILEARN: {
            parent_.midiLearn(!parent_.midiLearn());
            displayItem(MMI_MIDILEARN);
            displayItem(MMI_MODLEARN);
            parent_.flipDisplay();
            // parent_.changeMode(OM_PARAMETER);
            break;
        }
        case MMI_MODLEARN: {
            parent_.modulationLearn(!parent_.modulationLearn());
            displayItem(MMI_MIDILEARN);
            displayItem(MMI_MODLEARN);
            parent_.flipDisplay();
            // parent_.changeMode(OM_PARAMETER);
            break;
        }
        case MMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->saveSettings();
            }
            parent_.changeMode(OM_PARAMETER);
            break;
        }
        case MMI_HOME: {
            parent_.changeMode(OM_PARAMETER);
            parent_.sendGoHome();
//            parent_.sendPdMessage("goHome", 1.0);
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


bool OPresetMenu::init() {
    auto rack = model()->getRack(parent_.currentRack());
    if (rack != nullptr) {
        presets_ = rack->getPresetList();
    } else {
        presets_.clear();
    }
    return true;
}

void OPresetMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    if (rack != nullptr) {
        presets_ = rack->getPresetList();
    } else {
        presets_.clear();
    }
    OMenuMode::activate();
}


unsigned OPresetMenu::getSize() {
    return static_cast<unsigned int>((unsigned) PMI_LAST + presets_.size());
}

std::string OPresetMenu::getItemText(unsigned idx) {
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


void OPresetMenu::clicked(unsigned idx) {
    switch (idx) {
        case PMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->savePreset(rack->currentPreset());
            }
            parent_.changeMode(OM_PARAMETER);
            break;
        }
        case PMI_NEW: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = "new-" + std::to_string(presets_.size());
                rack->savePreset(newPreset);
            }
            parent_.changeMode(OM_MAINMENU);
            break;
        }
        case PMI_SEP: {
            break;
        }
        default: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = presets_[idx - PMI_LAST];
                parent_.changeMode(OM_PARAMETER);
                rack->loadPreset(newPreset);
            }
            break;
        }
    }
}

void OModuleMenu::populateMenu(const std::string& catSel) {
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


void OModuleMenu::activate() {
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

    OFixedMenuMode::activate();
}


void OModuleMenu::clicked(unsigned idx) {
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
                    parent_.changeMode(OM_PARAMETER);
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
    parent_.changeMode(OM_PARAMETER);
}


void OModuleSelectMenu::activate() {
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
    OFixedMenuMode::activate();
}


void OModuleSelectMenu::clicked(unsigned idx) {
    parent_.changeMode(OM_PARAMETER);
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



// Organelle implmentation

Organelle::Organelle() : messageQueue_(OscMsg::MAX_N_OSC_MSGS) {
}

Organelle::~Organelle() {
    stop();
}

void Organelle::stop() {
    running_ = false;
    if (socket_) {
        writer_thread_.join();
        OscMsg msg;
        while (messageQueue_.try_dequeue(msg));
    }
    socket_.reset();
}


bool Organelle::init() {
    // add modes before KD init
    addMode(OM_PARAMETER, std::make_shared<OParamMode>(*this));
    addMode(OM_MAINMENU, std::make_shared<OMainMenu>(*this));
    addMode(OM_PRESETMENU, std::make_shared<OPresetMenu>(*this));
    addMode(OM_MODULEMENU, std::make_shared<OModuleMenu>(*this));
    addMode(OM_MODULESELECTMENU, std::make_shared<OModuleSelectMenu>(*this));

    if (KontrolDevice::init()) {
        // setup mother.pd for reasonable behaviour, basically takeover
//        sendPdMessage("enableSubMenu", 1.0f);
        connect();
        sendEnableSubMenu();
        changeMode(OM_PARAMETER);
        return true;
    }
    return false;
}

void *organelle_write_thread_func(void *aObj) {
    post("start organelle write thead");
    auto *pThis = static_cast<Organelle *>(aObj);
    pThis->writePoll();
    post("organelle write thread ended");
    return nullptr;
}

bool Organelle::connect() {
    try {
        socket_ = std::shared_ptr<UdpTransmitSocket>(
                new UdpTransmitSocket(IpEndpointName(OSC_MOTHER_HOST, OSC_MOTHER_PORT)));
    } catch (const std::runtime_error &e) {
        post("could not connect to mother host for screen updates");
        socket_.reset();
        return false;
    }
    running_ = true;
#ifdef __COBALT__
    post("organelle use pthread for COBALT");
    pthread_t ph = writer_thread_.native_handle();
    pthread_create(&ph, 0,organelle_write_thread_func,this);
#else
    writer_thread_ = std::thread(organelle_write_thread_func, this);
#endif

    return true;
}


void Organelle::writePoll() {
    while (running_) {
        OscMsg msg;
        if (messageQueue_.wait_dequeue_timed(msg, std::chrono::milliseconds(MOTHER_WRITE_POLL_WAIT_TIMEOUT))) {
            socket_->Send(msg.buffer_, (size_t) msg.size_);
        }
    }
}


void Organelle::send(const char *data, unsigned size) {
    OscMsg msg;
    msg.size_ = (size > OscMsg::MAX_OSC_MESSAGE_SIZE ? OscMsg::MAX_OSC_MESSAGE_SIZE : size);
    memcpy(msg.buffer_, data, (size_t) msg.size_);
    messageQueue_.enqueue(msg);
}


void Organelle::sendEnableSubMenu() {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginMessage("/enablepatchsub")
        << 1
        << osc::EndMessage;
    send(ops.Data(), ops.Size());
}

void Organelle::sendGoHome() {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginMessage("/gohome")
        << 1
        << osc::EndMessage;
    send(ops.Data(), ops.Size());
}



void Organelle::displayPopup(const std::string &text, bool dblline) {
    if (dblline) {
        {
            osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
            ops << osc::BeginMessage("/oled/gFillArea")
                << PATCH_SCREEN
                << 2 << 12
                << 118 << 38
                << 0
                << osc::EndMessage;
            send(ops.Data(), ops.Size());
        }

        {
            osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
            ops << osc::BeginMessage("/oled/gBox")
                << PATCH_SCREEN
                << 2 << 12
                << 118 << 38
                << 1
                << osc::EndMessage;
            send(ops.Data(), ops.Size());
        }

    } else {
        {
            osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
            ops << osc::BeginMessage("/oled/gFillArea")
                << PATCH_SCREEN
                << 4 << 14
                << 114 << 34
                << 0
                << osc::EndMessage;
            send(ops.Data(), ops.Size());
        }
    }

    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/oled/gBox")
            << PATCH_SCREEN
            << 4 << 14
            << 114 << 34
            << 1
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }

    {
        int txtsize = 16;
        if (text.length() > 12) txtsize = 8;
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/oled/gPrintln")
            << PATCH_SCREEN
            << 10 << 24
            << txtsize << 1
            << text.c_str()
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }
}


std::string Organelle::asDisplayString(const Kontrol::Parameter &param, unsigned width) const {
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

void Organelle::clearDisplay() {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
    //ops << osc::BeginMessage( "/oled/gClear" )
    //    << 1
    //    << osc::EndMessage;
    ops << osc::BeginMessage("/oled/gFillArea")
        << PATCH_SCREEN
        << 0 << 8
        << 128 << 45
        << 0
        << osc::EndMessage;
    send(ops.Data(), ops.Size());
}

void Organelle::displayParamLine(unsigned line, const Kontrol::Parameter &param) {
    std::string disp = asDisplayString(param, SCREEN_WIDTH);
    displayLine(line, disp.c_str());
}

void Organelle::displayLine(unsigned line, const char *disp) {
    if (socket_ == nullptr) return;

    int x = ((line - 1) * 11) + ((line > 0) * 9);
    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/oled/gFillArea")
            << PATCH_SCREEN
            << 0 << x
            << 128 << 10
            << 0
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }
    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/oled/gPrintln")
            << PATCH_SCREEN
            << 2 << x
            << 8 << 1
            << disp
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }


}

void Organelle::invertLine(unsigned line) {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);

    int x = ((line - 1) * 11) + ((line > 0) * 9);
    ops << osc::BeginMessage("/oled/gInvertArea")
        << PATCH_SCREEN
        << 0 << x - 1
        << 128 << 10
        << osc::EndMessage;

    send(ops.Data(), ops.Size());

}

void Organelle::flipDisplay() {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginMessage("/oled/gFlip")
        << PATCH_SCREEN
        << osc::EndMessage;
    send(ops.Data(), ops.Size());
}

