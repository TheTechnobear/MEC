#include "SimpleOsc.h"

#include <osc/OscOutboundPacketStream.h>
#include <algorithm>

#include "../../m_pd.h"


const unsigned int SCREEN_WIDTH = 21;

static const int PAGE_SWITCH_TIMEOUT = 50;
static const int MODULE_SWITCH_TIMEOUT = 50;
//static const int PAGE_EXIT_TIMEOUT = 5;
static const auto MENU_TIMEOUT = 350;

static const float MAX_POT_VALUE = 1023.0F;

static const char *OSC_CLIENT_HOST = "127.0.0.1";
static const unsigned OSC_CLIENT_PORT = 8000;
static const unsigned OSC_WRITE_POLL_WAIT_TIMEOUT = 1000;

enum SimpleOscModes {
    OSM_MAINMENU,
    OSM_PRESETMENU,
    OSM_MODULEMENU,
    OSM_MODULESELECTMENU
};

char SimpleOsc::screenBuf_[SimpleOsc::OUTPUT_BUFFER_SIZE];

static const unsigned OSC_NUM_TEXTLINES = 5;
static const unsigned OSC_NUM_PARAMS = 8;


class OscBaseMode : public DeviceMode {
public:
    OscBaseMode(SimpleOsc &p) : parent_(p), popupTime_(-1) { ; }

    bool init() override { return true; }

    void poll() override;

    void changePot(unsigned, float) override { ; }

    void changeEncoder(unsigned, float) override { ; }

    void encoderButton(unsigned, bool) override { ; }

    void keyPress(unsigned, unsigned) override { ; }

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

    void displayPopup(const std::string &text, unsigned time, bool dblline);
protected:
    SimpleOsc &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    int popupTime_;
};


class OscParamMode : public DeviceMode {
public:
    OscParamMode(SimpleOsc &p) : parent_(p), pageIdx_(-1) { ; }

    bool init() override { return true; }

    void poll() override { ; }

    void activate() override;
    void changePot(unsigned pot, float value) override;
    void changeEncoder(unsigned encoder, float value) override;
//    void encoderButton(unsigned encoder, bool value) override;
//    void keyPress(unsigned, unsigned) override;
    void module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override;
    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override;
    void page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override;

    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::EntityId &,
                    const std::string &) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;


    void encoderButton(unsigned, bool) override { ; }
    void keyPress(unsigned, unsigned) override { ; }
    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }
    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; }
    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &,
                  const std::string &) override { ; };


private:
    void setCurrentPage(unsigned pageIdx, bool UI);
    void display();

    std::string moduleType_;
    int pageIdx_ = -1;
    Kontrol::EntityId pageId_;

    SimpleOsc &parent_;
    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }
};

class OscCompoundMode : public OscBaseMode {
public:
    OscCompoundMode(SimpleOsc& p, OscParamMode& param) : OscBaseMode(p), params_(param) {
    }

    void changePot(unsigned pot, float value) override {
        OscBaseMode::changePot(pot,value);
        params_.changePot(pot,value);
    }

    void module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override {
        OscBaseMode::module(source,rack,module);
        params_.module(source,rack,module);
    }
    void changed(Kontrol::ChangeSource source, const Kontrol::Rack & rack, const Kontrol::Module &module,
                 const Kontrol::Parameter &param) override {
        OscBaseMode::changed(source,rack,module,param);
        params_.changed(source,rack,module,param);

    }
    void page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override {
        OscBaseMode::page(source,rack,module,page);
        params_.page(source,rack,module,page);
    }

    void loadModule(Kontrol::ChangeSource source, const Kontrol::Rack & rack, const Kontrol::EntityId & moduleId,
                    const std::string &modType) override {
        OscBaseMode::loadModule(source,rack,moduleId,modType);
        params_.loadModule(source,rack,moduleId,modType);
    }

    void activeModule(Kontrol::ChangeSource source, const Kontrol::Rack & rack, const Kontrol::Module &module) override {
        OscBaseMode::activeModule(source,rack,module);
        params_.activeModule(source,rack,module);
    }



private:
    OscParamMode& params_;
};



class OscMenuMode : public OscCompoundMode {
public:
    OscMenuMode(SimpleOsc &p,OscParamMode& pa) : OscCompoundMode(p,pa), cur_(0), top_(0) { ; }

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


class OscFixedMenuMode : public OscMenuMode {
public:
    OscFixedMenuMode(SimpleOsc &p,OscParamMode& pa) : OscMenuMode(p,pa) { ; }

    unsigned getSize() override { return items_.size(); };

    std::string getItemText(unsigned i) override { return items_[i]; }

protected:
    std::vector<std::string> items_;
};

class OscMainMenu : public OscMenuMode {
public:
    OscMainMenu(SimpleOsc &p,OscParamMode& pa) : OscMenuMode(p,pa) { ; }

    bool init() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
};

class OscPresetMenu : public OscMenuMode {
public:
    OscPresetMenu(SimpleOsc &p,OscParamMode& pa) : OscMenuMode(p,pa) { ; }

    bool init() override;
    void activate() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
private:
    std::vector<std::string> presets_;
};


class OscModuleMenu : public OscFixedMenuMode {
public:
    OscModuleMenu(SimpleOsc &p,OscParamMode& pa) : OscFixedMenuMode(p,pa) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};

class OscModuleSelectMenu : public OscFixedMenuMode {
public:
    OscModuleSelectMenu(SimpleOsc &p,OscParamMode& pa) : OscFixedMenuMode(p,pa) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};

//------ OscBaseMode

void OscBaseMode::displayPopup(const std::string &text, unsigned time, bool dblline) {
    popupTime_ = time;
    parent_.displayPopup(text, dblline);
}

void OscBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

//------ OscParamMode

void OscParamMode::display() {
    parent_.clearDisplay();

    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
    auto page = parent_.model()->getPage(module, pageId_);
//    auto pages = parent_.model()->getPages(module);
    auto params = parent_.model()->getParams(module, page);


    unsigned int j = 0;
    for (auto param : params) {
        if (param != nullptr) {
            parent_.displayParamNum(j + 1, *param);
        }
        j++;
        if (j == OSC_NUM_PARAMS) break;
    }
    for (; j < OSC_NUM_PARAMS; j++) {
        parent_.clearParamNum(j + 1);
    }
}


void OscParamMode::activate() {
    display();
}

void OscParamMode::changePot(unsigned pot, float rawvalue) {
    try {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
        auto page = parent_.model()->getPage(module, pageId_);
//        auto pages = parent_.model()->getPages(module);
        auto params = parent_.model()->getParams(module, page);

        if (!(pot < params.size())) return;

        auto &param = params[pot];
        auto paramId = param->id();

        Kontrol::ParamValue calc;

        if (rawvalue != std::numeric_limits<float>::max()) {
            float value = rawvalue / MAX_POT_VALUE;
            calc = param->calcFloat(value);
            //std::cerr << "changePot " << pot << " " << value << " cv " << calc.floatValue() << " pv " << param->current().floatValue() << std::endl;
        }

        model()->changeParam(Kontrol::CS_LOCAL, parent_.currentRack(), parent_.currentModule(), paramId, calc);

    } catch (std::out_of_range) {
        return;
    }

}

void OscParamMode::setCurrentPage(unsigned pageIdx, bool UI) {
    auto module = model()->getModule(model()->getRack(parent_.currentRack()), parent_.currentModule());
    if (module == nullptr) return;

    try {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
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

//        if (UI) {
//            displayPopup(page->displayName(), PAGE_SWITCH_TIMEOUT, false);
//        }

    } catch (std::out_of_range) { ;
    }
}

void OscParamMode::changeEncoder(unsigned enc, float value) {
    if (pageIdx_ < 0) {
        setCurrentPage(0, false);
        return;
    }

    unsigned pagenum = (unsigned) pageIdx_;

    if (value > 0) {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
//        auto page = parent_.model()->getPage(module,pageId_);
        auto pages = parent_.model()->getPages(module);
//        auto params = parent_.model()->getParams(module,page);

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


void OscParamMode::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &rack, const Kontrol::Module &) {
    if (rack.id() == parent_.currentRack()) {
        pageIdx_ = -1;
        setCurrentPage(0, false);
    }
}


void OscParamMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                           const Kontrol::Parameter &param) {
    if (rack.id() != parent_.currentRack() || module.id() != parent_.currentModule()) return;

    auto prack = parent_.model()->getRack(parent_.currentRack());
    auto pmodule = parent_.model()->getModule(prack, parent_.currentModule());
    auto page = parent_.model()->getPage(pmodule, pageId_);
//    auto pages = parent_.model()->getPages(pmodule);
    auto params = parent_.model()->getParams(pmodule, page);


    unsigned sz = params.size();
    sz = sz < OSC_NUM_PARAMS ? sz : OSC_NUM_PARAMS;
    for (unsigned int i = 0; i < sz; i++) {
        try {
            auto &p = params.at(i);
            if (p->id() == param.id()) {
                p->change(param.current(), src == Kontrol::CS_PRESET);
                parent_.displayParamNum(i + 1, param);
                if (src != Kontrol::CS_LOCAL) {
                    changePot(i, param.current().floatValue());
                }
                return;
            }
        } catch (std::out_of_range) {
            return;
        }
    } // for
}

void OscParamMode::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    if (moduleType_ != module.type()) {
        pageIdx_ = -1;
    }
    moduleType_ = module.type();
}

void OscParamMode::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                        const Kontrol::Page &page) {
    if (pageIdx_ < 0) setCurrentPage(0, false);
}


void OscParamMode::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                              const Kontrol::EntityId &moduleId, const std::string &modType) {
    if (parent_.currentModule() == moduleId) {
        if (moduleType_ != modType) {
            pageIdx_ = -1;
            moduleType_ = modType;
        }
    }
}
//---- OscMenuMode
void OscMenuMode::activate() {
    display();
    popupTime_ = MENU_TIMEOUT;
    clickedDown_ = false;
}

void OscMenuMode::poll() {
    OscBaseMode::poll();
    if (popupTime_ == 0) {
        parent_.changeMode(OSM_MAINMENU);
        popupTime_ = -1;
    }
}

void OscMenuMode::display() {
    parent_.clearDisplay();
    for (unsigned i = top_; i < top_ + OSC_NUM_TEXTLINES; i++) {
        displayItem(i);
    }
}

void OscMenuMode::displayItem(unsigned i) {
    if (i < getSize()) {
        std::string item = getItemText(i);
        unsigned line = i - top_ + 1;
        parent_.displayLine(line, item.c_str());
        if (i == cur_) {
            parent_.invertLine(line);
        }
    }
}


void OscMenuMode::changeEncoder(unsigned, float value) {
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
        } else if (cur >= top_ + OSC_NUM_TEXTLINES) {
            top_ = cur - (OSC_NUM_TEXTLINES - 1);
            cur_ = cur;
            display();
        } else {
            line = cur_ - top_ + 1;
            if (line <= OSC_NUM_TEXTLINES) parent_.invertLine(line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= OSC_NUM_TEXTLINES) parent_.invertLine(line);
        }
    }
    popupTime_ = MENU_TIMEOUT;
}


void OscMenuMode::encoderButton(unsigned, bool value) {
    if (clickedDown_ && value < 1.0) {
        clicked(cur_);
    }
    clickedDown_ = value;
}


/// main menu
enum OscMainMenuItms {
    OSC_MMI_MODULE,
    OSC_MMI_PRESET,
    OSC_MMI_MIDILEARN,
    OSC_MMI_MODLEARN,
    OSC_MMI_SAVE,
    OSC_MMI_SIZE
};

bool OscMainMenu::init() {
    return true;
}


unsigned OscMainMenu::getSize() {
    return (unsigned) OSC_MMI_SIZE;
}

std::string OscMainMenu::getItemText(unsigned idx) {
    switch (idx) {
        case OSC_MMI_MODULE: {
            auto rack = model()->getRack(parent_.currentRack());
            auto module = model()->getModule(rack, parent_.currentModule());
            if (module == nullptr)
                return parent_.currentModule();
            else
                return parent_.currentModule() + ":" + module->displayName();
        }
        case OSC_MMI_PRESET: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                return rack->currentPreset();
            }
            return "No Preset";
        }
        case OSC_MMI_SAVE:
            return "Save";
        case OSC_MMI_MIDILEARN: {
            if (parent_.midiLearn()) {
                return "Midi Learn        [X]";
            }
            return "Midi Learn        [ ]";
        }
        case OSC_MMI_MODLEARN: {
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


void OscMainMenu::clicked(unsigned idx) {
    switch (idx) {
        case OSC_MMI_MODULE: {
            parent_.changeMode(OSM_MODULEMENU);
            break;
        }
        case OSC_MMI_PRESET: {
            parent_.changeMode(OSM_PRESETMENU);
            break;
        }
        case OSC_MMI_MIDILEARN: {
            parent_.midiLearn(!parent_.midiLearn());
            displayItem(OSC_MMI_MIDILEARN);
            displayItem(OSC_MMI_MODLEARN);
            // parent_.changeMode(OSM_PARAMETER);
            break;
        }
        case OSC_MMI_MODLEARN: {
            parent_.modulationLearn(!parent_.modulationLearn());
            displayItem(OSC_MMI_MIDILEARN);
            displayItem(OSC_MMI_MODLEARN);
            // parent_.changeMode(OSM_PARAMETER);
            break;
        }
        case OSC_MMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->saveSettings();
            }
            parent_.changeMode(OSM_MAINMENU);
            break;
        }
        default:
            break;
    }
}

// preset menu
enum OscPresetMenuItms {
    OSC_PMI_UPDATE,
    OSC_PMI_NEW,
    OSC_PMI_SEP,
    OSC_PMI_LAST
};


bool OscPresetMenu::init() {
    auto rack = model()->getRack(parent_.currentRack());
    if (rack != nullptr) {
        presets_ = rack->getPresetList();
    } else {
        presets_.clear();
    }
    return true;
}

void OscPresetMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    if (rack != nullptr) {
        presets_ = rack->getPresetList();
    } else {
        presets_.clear();
    }
    OscMenuMode::activate();
}


unsigned OscPresetMenu::getSize() {
    return (unsigned) OSC_PMI_LAST + presets_.size();
}

std::string OscPresetMenu::getItemText(unsigned idx) {
    switch (idx) {
        case OSC_PMI_UPDATE:
            return "Update Preset";
        case OSC_PMI_NEW:
            return "New Preset";
        case OSC_PMI_SEP:
            return "--------------------";
        default:
            return presets_[idx - OSC_PMI_LAST];
    }
}


void OscPresetMenu::clicked(unsigned idx) {
    switch (idx) {
        case OSC_PMI_UPDATE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->updatePreset(rack->currentPreset());
            }
            parent_.changeMode(OSM_MAINMENU);
            break;
        }
        case OSC_PMI_NEW: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = "New " + std::to_string(presets_.size());
                rack->updatePreset(newPreset);
            }
            parent_.changeMode(OSM_MAINMENU);
            break;
        }
        case OSC_PMI_SEP: {
            break;
        }
        default: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = presets_[idx - OSC_PMI_LAST];
                parent_.changeMode(OSM_MAINMENU);
                rack->applyPreset(newPreset);
            }
            break;
        }
    }
}


void OscModuleMenu::activate() {
    auto rack = model()->getRack(parent_.currentRack());
    auto module = model()->getModule(rack, parent_.currentModule());
    if (module == nullptr) return;
    unsigned idx = 0;
    auto res = rack->getResources("module");
    items_.clear();
    for (auto modtype : res) {
        items_.push_back(modtype);
        if (modtype == module->type()) {
            cur_ = idx;
            top_ = idx;
        }
        idx++;
    }
    OscFixedMenuMode::activate();
}


void OscModuleMenu::clicked(unsigned idx) {
    if (idx < getSize()) {
        auto rack = model()->getRack(parent_.currentRack());
        auto module = model()->getModule(rack, parent_.currentModule());
        if (module == nullptr) return;

        auto modtype = items_[idx];
        if (modtype != module->type()) {
            //FIXME, workaround since changing the module needs to tell params to change page
            parent_.changeMode(OSM_MAINMENU);
            model()->loadModule(Kontrol::CS_LOCAL, rack->id(), module->id(), modtype);
        }
    }
    parent_.changeMode(OSM_MAINMENU);
}


void OscModuleSelectMenu::activate() {
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
    OscFixedMenuMode::activate();
}


void OscModuleSelectMenu::clicked(unsigned idx) {
    parent_.changeMode(OSM_MAINMENU);
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



// SimpleOsc implmentation

SimpleOsc::SimpleOsc() : messageQueue_(OscMsg::MAX_N_OSC_MSGS) {
}

SimpleOsc::~SimpleOsc() {
    stop();
}

void SimpleOsc::stop() {
    running_ = false;
    if (socket_) {
        writer_thread_.join();
        OscMsg msg;
        while (messageQueue_.try_dequeue(msg));
    }
    socket_.reset();
}


bool SimpleOsc::init() {

    paramDisplay_ = std::make_shared<OscParamMode>(*this);

    // add modes before KD init
    addMode(OSM_MAINMENU, std::make_shared<OscMainMenu>(*this,*paramDisplay_));
    addMode(OSM_PRESETMENU, std::make_shared<OscPresetMenu>(*this,*paramDisplay_));
    addMode(OSM_MODULEMENU, std::make_shared<OscModuleMenu>(*this,*paramDisplay_));
    addMode(OSM_MODULESELECTMENU, std::make_shared<OscModuleSelectMenu>(*this,*paramDisplay_));

    if (KontrolDevice::init()) {
        connect();
        paramDisplay_->init();
        paramDisplay_->activate();
        changeMode(OSM_MAINMENU);
        return true;
    }
    return false;
}

void *simpleosc_write_thread_func(void *aObj) {
    post("start simple osc write thead");
    SimpleOsc *pThis = static_cast<SimpleOsc *>(aObj);
    pThis->writePoll();
    post("simple osc write thread ended");
    return nullptr;
}

bool SimpleOsc::connect() {
    try {
        socket_ = std::shared_ptr<UdpTransmitSocket>(
                new UdpTransmitSocket(IpEndpointName(OSC_CLIENT_HOST, OSC_CLIENT_PORT)));
    } catch (const std::runtime_error &e) {
        post("could not connect to simple osc host for screen updates");
        socket_.reset();
        return false;
    }
    running_ = true;
#ifdef __COBALT__
    post("organelle use pthread for COBALT");
    pthread_t ph = writer_thread_.native_handle();
    pthread_create(&ph, 0,simpleosc_write_thread_func,this);
#else
    writer_thread_ = std::thread(simpleosc_write_thread_func, this);
#endif

    return true;
}


void SimpleOsc::writePoll() {
    while (running_) {
        OscMsg msg;
        if (messageQueue_.wait_dequeue_timed(msg, std::chrono::milliseconds(OSC_WRITE_POLL_WAIT_TIMEOUT))) {
            socket_->Send(msg.buffer_, (size_t) msg.size_);
        }
    }
}


void SimpleOsc::send(const char *data, unsigned size) {
    OscMsg msg;
    msg.size_ = (size > OscMsg::MAX_OSC_MESSAGE_SIZE ? OscMsg::MAX_OSC_MESSAGE_SIZE : size);
    memcpy(msg.buffer_, data, (size_t) msg.size_);
    messageQueue_.enqueue(msg);
}


void SimpleOsc::displayPopup(const std::string &text, bool) {

    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/text")
            << (int8_t) 3
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }
}


void SimpleOsc::clearDisplay() {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginMessage("/clearText")
        << osc::EndMessage;
    send(ops.Data(), ops.Size());
}

void SimpleOsc::clearParamNum(unsigned num) {
    std::string p = "P" + std::to_string(num);
    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        std::string field = "/" + p + "Desc";
        const char *addr = field.c_str();
        ops << osc::BeginMessage(addr)
            << ""
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }

    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        std::string field = "/" + p + "Ctrl";
        const char *addr = field.c_str();
        float v = 0.0;
        ops << osc::BeginMessage(addr)
            << v
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }

    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        std::string field = "/" + p + "Value";
        const char *addr = field.c_str();
        ops << osc::BeginMessage(addr)
            << ""
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }

}

void SimpleOsc::displayParamNum(unsigned num, const Kontrol::Parameter &param) {
    std::string p = "P" + std::to_string(num);
    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        std::string field = "/" + p + "Desc";
        const char *addr = field.c_str();
        ops << osc::BeginMessage(addr)
            << param.displayName().c_str()
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }

    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        std::string field = "/" + p + "Ctrl";
        const char *addr = field.c_str();
        float v = param.asFloat(param.current());
        ops << osc::BeginMessage(addr)
            << v
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }

    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        std::string field = "/" + p + "Value";
        const char *addr = field.c_str();
        ops << osc::BeginMessage(addr)
            << (param.displayValue() + " " + param.displayUnit()).c_str()
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }
}

void SimpleOsc::displayLine(unsigned line, const char *disp) {
    if (socket_ == nullptr) return;

    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/text")
            << (int8_t) line
            << disp
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }


}

void SimpleOsc::invertLine(unsigned line) {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginMessage("/selectText")
        << (int8_t) line
        << osc::EndMessage;

    send(ops.Data(), ops.Size());

}


