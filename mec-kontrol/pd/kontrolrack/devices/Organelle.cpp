#include "Organelle.h"

#include <osc/OscOutboundPacketStream.h>
//#include <cmath>

#include "../../m_pd.h"


const unsigned int SCREEN_WIDTH = 21;

static const int PAGE_SWITCH_TIMEOUT = 50;
//static const int PAGE_EXIT_TIMEOUT = 5;
static const auto MENU_TIMEOUT = 350;


const int8_t PATCH_SCREEN = 3;


static const unsigned int OUTPUT_BUFFER_SIZE = 1024;
static char screenosc[OUTPUT_BUFFER_SIZE];

static const float MAX_POT_VALUE = 1023.0F;

enum OrganelleModes {
    OM_PARAMETER,
    OM_MAINMENU,
    OM_PRESETMENU
};


class OBaseMode : public DeviceMode {
public:
    OBaseMode(Organelle &p) : parent_(p), popupTime_(-1) { ; }

    virtual bool init() { return true; }

    virtual void poll();

    virtual void changePot(unsigned, float) { ; }

    virtual void changeEncoder(unsigned, float) { ; }

    virtual void encoderButton(unsigned, bool) { ; }

    virtual void rack(Kontrol::ParameterSource, const Kontrol::Rack &) { ; }

    virtual void module(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &) { ; }

    virtual void page(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                      const Kontrol::Page &) { ; }

    virtual void param(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                       const Kontrol::Parameter &) { ; }

    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                         const Kontrol::Parameter &) { ; }

    void displayPopup(const std::string &text, unsigned time);
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
    } locked_[4];
    float rawValue[4];
};


class OParamMode : public OBaseMode {
public:
    OParamMode(Organelle &p) : OBaseMode(p), currentPageNum_(0) { ; }

    virtual bool init() override;
    virtual void poll() override;
    virtual void activate() override;
    virtual void changePot(unsigned pot, float value) override;
    virtual void changeEncoder(unsigned encoder, float value) override;
    virtual void encoderButton(unsigned encoder, bool value) override;
    void module(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override;
    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                         const Kontrol::Parameter &) override;
    void page(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override;
private:
    void setPage(unsigned pagenum, bool UI);
    void display();
    int currentPageNum_;
    std::shared_ptr<Pots> pots_;
    std::vector<std::shared_ptr<Kontrol::Parameter>> currentParams_;
    std::vector<std::shared_ptr<Kontrol::Page>> pages_;
};

class OMenuMode : public OBaseMode {
public:
    OMenuMode(Organelle &p) : OBaseMode(p), cur_(0), top_(0) { ; }

    virtual unsigned getSize() = 0;
    virtual std::string getItemText(unsigned idx) = 0;
    virtual void clicked(unsigned idx) = 0;

    virtual bool init() { return true; }

    virtual void poll();
    virtual void activate();
    virtual void changeEncoder(unsigned encoder, float value);
    virtual void encoderButton(unsigned encoder, bool value);

protected:
    void display();
    void displayItem(unsigned idx);
    unsigned cur_;
    unsigned top_;
};


//class OFixedMenuMode : public OMenuMode {
//public:
//    OFixedMenuMode(Organelle &p) : OMenuMode(p) { ; }
//
//    virtual unsigned getSize() { return items_.size(); };
//
//    virtual std::string getItemText(unsigned i) { return items_[i]; }
//
//protected:
//    std::vector<std::string> items_;
//};

class OMainMenu : public OMenuMode {
public:
    OMainMenu(Organelle &p) : OMenuMode(p) { ; }

    virtual bool init();
    virtual unsigned getSize();
    virtual std::string getItemText(unsigned idx);
    virtual void clicked(unsigned idx);
};

class OPresetMenu : public OMenuMode {
public:
    OPresetMenu(Organelle &p) : OMenuMode(p) { ; }

    virtual bool init();
    virtual void activate();
    virtual unsigned getSize();
    virtual std::string getItemText(unsigned idx);
    virtual void clicked(unsigned idx);
private:
    std::vector<std::string> presets_;
};


void OBaseMode::displayPopup(const std::string &text, unsigned time) {
    popupTime_ = time;
    parent_.displayPopup(text);
}

void OBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

bool OParamMode::init() {
    OBaseMode::init();
    auto module = model()->getModule(model()->getRack(parent_.currentRack()), parent_.currentModule());
    if (module) pages_ = module->getPages();

    if (pages_.size() > 0) {
        currentPageNum_ = 0;
        setPage(0, false);
    } else currentPageNum_ = -1;


    pots_ = std::make_shared<Pots>();
    for (int i = 0; i < 4; i++) {
        pots_->rawValue[i] = std::numeric_limits<float>::max();
        pots_->locked_[i] = Pots::K_LOCKED;
    }
    return true;
}

void OParamMode::display() {
    parent_.clearDisplay();
    unsigned sz = currentParams_.size();
    sz = sz < 4 ? sz : 4;
    for (unsigned int i = 0; i < sz; i++) {
        try {
            auto &param = currentParams_.at(i);
            if (param != nullptr) parent_.displayParamLine(i + 1, *param);
        } catch (std::out_of_range) {
            return;
        }
    } // for
    parent_.flipDisplay();
}


void OParamMode::activate() {
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
        auto &param = currentParams_.at(pot);
        auto paramId = param->id();

        Kontrol::ParamValue calc;

        if (rawvalue!= std::numeric_limits<float>::max()) {
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
            model()->changeParam(Kontrol::PS_LOCAL, parent_.currentRack(), parent_.currentModule(), paramId, calc);
        }
    } catch (std::out_of_range) {
        return;
    }

}

void OParamMode::setPage(unsigned pagenum, bool UI) {
    auto module = model()->getModule(model()->getRack(parent_.currentRack()), parent_.currentModule());
    if (module == nullptr) return;

    try {
        if (currentPageNum_ < 0) {
            // initialisation
            pages_ = module->getPages();
            if (pages_.size() == 0) {
                return;
            }
            auto &page = pages_.at(0);
            currentParams_ = module->getParams(page);
            if (currentParams_.size() == 0) {
                return;
            }
            currentPageNum_ = 0;
            display();
        }

        auto &page = pages_.at(pagenum);
        currentPageNum_ = pagenum;
        currentParams_ = module->getParams(page);
        if (UI) {
            displayPopup(page->displayName(), PAGE_SWITCH_TIMEOUT);
            parent_.flipDisplay();
        }

        for (int i = 0; i < 4; i++) {
            pots_->locked_[i] = Pots::K_LOCKED;
            changePot(i, pots_->rawValue[i]);
        }
    } catch (std::out_of_range) { ;
    }
}

void OParamMode::changeEncoder(unsigned enc, float value) {
    OBaseMode::changeEncoder(enc, value);
    if (currentPageNum_ < 0) {
        setPage(0, false);
        return;
    }

    unsigned pagenum = (unsigned) currentPageNum_;

    if (value > 0) {
        // clockwise
        pagenum++;
        pagenum = std::min(pagenum, (unsigned) pages_.size() - 1);
    } else {
        // anti clockwise
        if (pagenum > 0) pagenum--;
    }

    if (pagenum != currentPageNum_) {
        setPage(pagenum, true);
    }
}

void OParamMode::encoderButton(unsigned enc, bool value) {
    OBaseMode::encoderButton(enc, value);
    if (value < 1.0) {
        parent_.changeMode(OM_MAINMENU);
    }
}

void OParamMode::changed(Kontrol::ParameterSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                         const Kontrol::Parameter &param) {
    OBaseMode::changed(src, rack, module, param);
    if (popupTime_ > 0) return;

    if (rack.id() != parent_.currentRack() || module.id() != parent_.currentModule()) return;
    unsigned sz = currentParams_.size();
    sz = sz < 4 ? sz : 4;
    for (unsigned int i = 0; i < sz; i++) {
        try {
            auto &p = currentParams_.at(i);
            if (p->id() == param.id()) {
                p->change(param.current(), src == Kontrol::PS_PRESET);
                parent_.displayParamLine(i + 1, param);
                if (src != Kontrol::PS_LOCAL) {
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

void OParamMode::module(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    OBaseMode::module(source, rack, module);
    if (currentPageNum_ < 0) setPage(0, false);
}

void OParamMode::page(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                      const Kontrol::Page &page) {
    OBaseMode::page(source, rack, module, page);
    if (currentPageNum_ < 0) setPage(0, false);
}

void OMenuMode::activate() {
    display();
    popupTime_ = MENU_TIMEOUT;
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
    for (unsigned i = top_; i < top_ + 4; i++) {
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
        } else if (cur >= top_ + 4) {
            top_ = cur - 3;
            cur_ = cur;
            display();
        } else {
            line = cur_ - top_ + 1;
            if (line <= 4) parent_.invertLine(line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= 4) parent_.invertLine(line);
        }
    }
    popupTime_ = MENU_TIMEOUT;
}


void OMenuMode::encoderButton(unsigned, bool value) {
    if (value < 1.0) {
        clicked(cur_);
    }
}


/// main menu
enum MainMenuItms {
    MMI_HOME,
    MMI_MODULE,
    MMI_PRESET,
    MMI_LEARN,
    MMI_SAVE,
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
        case MMI_HOME:
            return "Home";
        case MMI_MODULE:
            return parent_.currentModule();
        case MMI_PRESET: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                return rack->currentPreset();
            }
            return "No Preset";
        }
        case MMI_SAVE:
            return "Save Settings";
        case MMI_LEARN: {
            if (parent_.midiLearn()) {
                return "Midi Learn        [X]";
            }
            return "Midi Learn        [ ]";
        }
        default:
            break;
    }
    return "";
}


void OMainMenu::clicked(unsigned idx) {
    switch (idx) {
        case MMI_HOME: {
            parent_.changeMode(OM_PARAMETER);
            parent_.sendPdMessage("goHome", 1.0);
            break;
        }
        case MMI_MODULE: {
            parent_.changeMode(OM_PARAMETER);
            break;
        }
        case MMI_PRESET: {
            parent_.changeMode(OM_PRESETMENU);
            break;
        }
        case MMI_LEARN: {
            parent_.midiLearn(!parent_.midiLearn());
            displayItem(MMI_LEARN);
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
    return (unsigned) PMI_LAST + presets_.size();
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
                rack->updatePreset(rack->currentPreset());
            }
            parent_.changeMode(OM_PARAMETER);
            break;
        }
        case PMI_NEW: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = "New " + std::to_string(presets_.size());
                rack->updatePreset(newPreset);
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
                rack->applyPreset(newPreset);
            }
            break;
        }
    }
}




// Organelle implmentation

Organelle::Organelle() {
    PaUtil_InitializeRingBuffer(&messageQueue_, sizeof(OscMsg), OscMsg::MAX_N_OSC_MSGS, msgData_);
}

Organelle::~Organelle() {
    stop();
}

void Organelle::stop() {
    running_ = false;
    if (socket_) {
        writer_thread_.join();
        PaUtil_FlushRingBuffer(&messageQueue_);
    }
    socket_.reset();
}


bool Organelle::init() {
    // add modes before KD init
    addMode(OM_PARAMETER, std::make_shared<OParamMode>(*this));
    addMode(OM_MAINMENU, std::make_shared<OMainMenu>(*this));
    addMode(OM_PRESETMENU, std::make_shared<OPresetMenu>(*this));

    if (KontrolDevice::init()) {
        midiLearn(false);
        lastParamId_ = "";

        // setup mother.pd for reasonable behaviour, basically takeover
        sendPdMessage("midiOutGate", 0.0f);
        // sendPdMessage("midiInGate",0.0f);
        sendPdMessage("enableSubMenu", 1.0f);
        connect();
        changeMode(OM_PARAMETER);
        return true;
    }
    return false;
}

void *organelle_write_thread_func(void *aObj) {
    Organelle *pThis = static_cast<Organelle *>(aObj);
    pThis->writePoll();
    return nullptr;
}

bool Organelle::connect() {
    try {
        socket_ = std::shared_ptr<UdpTransmitSocket>(new UdpTransmitSocket(IpEndpointName("127.0.0.1", 4001)));
    } catch (const std::runtime_error &e) {
        post("could not connect to mother host for screen updates");
        socket_.reset();
        return false;
    }
    running_ = true;
    writer_thread_ = std::thread(organelle_write_thread_func, this);
    return true;
}


void Organelle::writePoll() {
    std::unique_lock<std::mutex> lock(write_lock_);
    while (running_) {
        while (PaUtil_GetRingBufferReadAvailable(&messageQueue_)) {
            OscMsg msg;
            PaUtil_ReadRingBuffer(&messageQueue_, &msg, 1);
            socket_->Send(msg.buffer_, msg.size_);
        }
        write_cond_.wait_for(lock, std::chrono::milliseconds(1000));
    }
}


void Organelle::send(const char *data, unsigned size) {
    OscMsg msg;
    msg.size_ = (size > OscMsg::MAX_OSC_MESSAGE_SIZE ? OscMsg::MAX_OSC_MESSAGE_SIZE : size);
    memcpy(msg.buffer_, data, (size_t) msg.size_);
    PaUtil_WriteRingBuffer(&messageQueue_, (void *) &msg, 1);
    write_cond_.notify_one();
}


void Organelle::displayPopup(const std::string &text) {
    {
        osc::OutboundPacketStream ops(screenosc, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/oled/gFillArea")
            << PATCH_SCREEN
            << 14 << 14
            << 100 << 34
            << 0
            << osc::EndMessage;
        socket_->Send(ops.Data(), ops.Size());
    }

    {
        osc::OutboundPacketStream ops(screenosc, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/oled/gBox")
            << PATCH_SCREEN
            << 14 << 14
            << 100 << 34
            << 1
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }

    {
        osc::OutboundPacketStream ops(screenosc, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/oled/gPrintln")
            << PATCH_SCREEN
            << 20 << 24
            << 16 << 1
            << text.c_str()
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }
}


std::string Organelle::asDisplayString(const Kontrol::Parameter &param, unsigned width) const {
    std::string pad = "";
    std::string ret;
    std::string value = param.displayValue();
    std::string unit = std::string(param.displayUnit() + "  ").substr(0, 2);
    const std::string &dName = param.displayName();
    int fillc = width - (dName.length() + value.length() + 1 + unit.length());
    for (; fillc > 0; fillc--) pad += " ";
    ret = dName + pad + value + " " + unit;
    if (ret.length() > width) ret = ret.substr(width - ret.length(), width);
    return ret;
}

void Organelle::clearDisplay() {
    osc::OutboundPacketStream ops(screenosc, OUTPUT_BUFFER_SIZE);
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
        osc::OutboundPacketStream ops(screenosc, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/oled/gFillArea")
            << PATCH_SCREEN
            << 0 << x
            << 128 << 10
            << 0
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }
    {
        osc::OutboundPacketStream ops(screenosc, OUTPUT_BUFFER_SIZE);
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
    osc::OutboundPacketStream ops(screenosc, OUTPUT_BUFFER_SIZE);

    int x = ((line - 1) * 11) + ((line > 0) * 9);
    ops << osc::BeginMessage("/oled/gInvertArea")
        << PATCH_SCREEN
        << 0 << x - 1
        << 128 << 10
        << osc::EndMessage;

    send(ops.Data(), ops.Size());

}

void Organelle::changed(Kontrol::ParameterSource src,
                        const Kontrol::Rack &rack,
                        const Kontrol::Module &module,
                        const Kontrol::Parameter &param) {

    if (midiLearnActive_
        && rack.id() == currentRackId_
        && module.id() == currentModuleId_) {
        lastParamId_ = param.id();
    }
    KontrolDevice::changed(src, rack, module, param);
}

void Organelle::rack(Kontrol::ParameterSource source, const Kontrol::Rack &rack) {
    KontrolDevice::rack(source, rack);
}

void Organelle::module(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    if (currentModuleId_.empty()) {
        currentRackId_ = rack.id();
        currentModuleId_ = module.id();
    }
    KontrolDevice::module(source, rack, module);
}

void Organelle::midiLearn(bool b) {
    lastParamId_ = "";
    midiLearnActive_ = b;
}


void Organelle::midiCC(unsigned num, unsigned value) {
    //std::cerr << "midiCC " << num << " " << value << std::endl;
    if (midiLearnActive_) {
        if (!lastParamId_.empty()) {
            auto rack = model()->getRack(currentRackId_);
            if (rack != nullptr) {
                if (value > 0) {
                    rack->addMidiCCMapping(num, currentModuleId_, lastParamId_);
                    lastParamId_ = "";
                } else {
                    //std::cerr << "midiCC unlearn" << num << " " << lastParamId_ << std::endl;
                    rack->removeMidiCCMapping(num, currentModuleId_, lastParamId_);
                    lastParamId_ = "";
                }
            }
        }
    }
    // update param model
    KontrolDevice::midiCC(num, value);
}

void Organelle::flipDisplay() {
    osc::OutboundPacketStream ops(screenosc, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginMessage("/oled/gFlip")
        << PATCH_SCREEN
        << osc::EndMessage;
    send(ops.Data(), ops.Size());
}

