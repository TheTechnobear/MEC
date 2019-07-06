#include "mec_oscdisplay.h"

#include <algorithm>
#include <unordered_set>

#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>

#include <mec_log.h>

namespace mec {

static const float MAX_POT_VALUE = 1.0F;

static const unsigned OSC_WRITE_POLL_WAIT_TIMEOUT = 1000;


char OscDisplay::screenBuf_[OscDisplay::OUTPUT_BUFFER_SIZE];

static const unsigned OSC_NUM_TEXTLINES = 5;
static const unsigned OSC_NUM_PARAMS = 8;


class OscDisplayParamMode : public Kontrol::KontrolCallback {
public:
    OscDisplayParamMode(OscDisplay &p) : parent_(p), pageIdx_(-1) { ; }

    bool init() { return true; };
    void activate();
    void changePot(unsigned pot, float value);

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override;
    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override;
    void page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override;

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; }

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &, const std::string &) override { ; }

    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::EntityId &,
                    const std::string &) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;

    void nextPage();
    void prevPage();

private:
    void setCurrentPage(unsigned pageIdx, bool UI);
    void display();

    std::string moduleType_;
    int pageIdx_ = -1;
    Kontrol::EntityId pageId_;

    OscDisplay &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

};


class OscDisplayBaseMode : public OscDisplayMode {
public:
    OscDisplayBaseMode(OscDisplay &p) : parent_(p), popupTime_(-1) { ; }


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


    virtual bool init() { return true; } // override
    virtual void poll() override;

    virtual void activate() override { ; }

protected:
    OscDisplay &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    int popupTime_;
};


class OscDisplayMenuMode : public OscDisplayBaseMode {
public:
    OscDisplayMenuMode(OscDisplay &p) : OscDisplayBaseMode(p), cur_(0), top_(0) { ; }

    virtual unsigned getSize() = 0;
    virtual std::string getItemText(unsigned idx) = 0;
    virtual void clicked(unsigned idx) = 0;

    bool init() override { return true; }

    void poll() override;
    void activate() override;
    void navPrev() override;
    void navNext() override;
    void navActivate() override;

    void savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;
protected:
    void display();
    void displayItem(unsigned idx);
    unsigned cur_;
    unsigned top_;
};


class OscDisplayFixedMenuMode : public OscDisplayMenuMode {
public:
    OscDisplayFixedMenuMode(OscDisplay &p) : OscDisplayMenuMode(p) { ; }

    unsigned getSize() override { return items_.size(); };

    std::string getItemText(unsigned i) override { return items_[i]; }

protected:
    std::vector<std::string> items_;
};

class OscDisplayMainMenu : public OscDisplayMenuMode {
public:
    OscDisplayMainMenu(OscDisplay &p) : OscDisplayMenuMode(p) { ; }

    bool init() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
};

class OscDisplayPresetMenu : public OscDisplayMenuMode {
public:
    OscDisplayPresetMenu(OscDisplay &p) : OscDisplayMenuMode(p) { ; }

    bool init() override;
    void activate() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
private:
    std::vector<std::string> presets_;
};


class OscDisplayModuleMenu : public OscDisplayFixedMenuMode {
public:
    OscDisplayModuleMenu(OscDisplay &p) : OscDisplayFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
private:
    void populateMenu(const std::string& catSel);
    std::string cat_;
};

class OscDisplayModuleSelectMenu : public OscDisplayFixedMenuMode {
public:
    OscDisplayModuleSelectMenu(OscDisplay &p) : OscDisplayFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};

//------ OscDisplayBaseMode

void OscDisplayBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

//------ OscDisplayParamMode

void OscDisplayParamMode::display() {
    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
    auto page = parent_.model()->getPage(module, pageId_);
//    auto pages = parent_.model()->getPages(module);
    auto params = parent_.model()->getParams(module, page);


    unsigned int j = 0;
    for (auto param : params) {
        if (param != nullptr) {
            parent_.displayParamNum(j + 1, *param, true);
        }
        j++;
        if (j == OSC_NUM_PARAMS) break;
    }
    for (; j < OSC_NUM_PARAMS; j++) {
        parent_.clearParamNum(j + 1);
    }
}


void OscDisplayParamMode::activate() {
    display();
}

void OscDisplayParamMode::changePot(unsigned pot, float rawvalue) {
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

void OscDisplayParamMode::setCurrentPage(unsigned pageIdx, bool UI) {
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
                    parent_.currentPage(pageId_);
                    display();
                } catch (std::out_of_range) { ;
                }
            } else {
                for (int j=0; j < OSC_NUM_PARAMS; j++) {
                    parent_.clearParamNum(j + 1);
                }
            }
        }

        std::string md = "";
        std::string pd = "";
        if (module) md = module->id() + " : " +module->displayName();
        if (page) pd = page->displayName();
        parent_.displayTitle(md, pd);
    } catch (std::out_of_range) { ;
    }
}


void OscDisplayParamMode::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module & module) {
    if (rack.id() == parent_.currentRack()) {
        if(src!= Kontrol::CS_LOCAL &&  module.id() != parent_.currentModule()) {
            parent_.currentModule(module.id());
        }
        pageIdx_ = -1;
        setCurrentPage(0, false);
    }
}


void OscDisplayParamMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
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
                parent_.displayParamNum(i + 1, param, src != Kontrol::CS_LOCAL);
                return;
            }
        } catch (std::out_of_range) {
            return;
        }
    } // for
}

void OscDisplayParamMode::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                 const Kontrol::Module &module) {
    if (moduleType_ != module.type()) {
        pageIdx_ = -1;
    }
    moduleType_ = module.type();
}

void OscDisplayParamMode::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                               const Kontrol::Page &page) {
    if (pageIdx_ < 0) setCurrentPage(0, false);
}


void OscDisplayParamMode::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                     const Kontrol::EntityId &moduleId, const std::string &modType) {
    if (parent_.currentModule() == moduleId) {
        if (moduleType_ != modType) {
            pageIdx_ = -1;
            moduleType_ = modType;
        }
    }
}

void OscDisplayParamMode::nextPage() {
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

void OscDisplayParamMode::prevPage() {
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


//---- OscDisplayMenuMode
void OscDisplayMenuMode::activate() {
    display();
    popupTime_ = parent_.menuTimeout();
}

void OscDisplayMenuMode::poll() {
    OscDisplayBaseMode::poll();
    if (popupTime_ == 0) {
        parent_.changeMode(OSM_MAINMENU);
        popupTime_ = -1;
    }
}

void OscDisplayMenuMode::display() {
    parent_.clearDisplay();
    for (unsigned i = top_; i < top_ + OSC_NUM_TEXTLINES; i++) {
        displayItem(i);
    }
}

void OscDisplayMenuMode::displayItem(unsigned i) {
    if (i < getSize()) {
        std::string item = getItemText(i);
        unsigned line = i - top_ + 1;
        parent_.displayLine(line, item.c_str());
        if (i == cur_) {
            parent_.invertLine(line);
        }
    }
}


void OscDisplayMenuMode::navPrev() {
    unsigned cur = cur_;
    if (cur_ > 0) {
        cur--;
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
    popupTime_ = parent_.menuTimeout();
}


void OscDisplayMenuMode::navNext() {
    unsigned cur = cur_;
    cur++;
    cur = std::min(cur, getSize() - 1);
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
    popupTime_ = parent_.menuTimeout();
}


void OscDisplayMenuMode::navActivate() {
    clicked(cur_);
}

void OscDisplayMenuMode::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    display();
    KontrolCallback::savePreset(source, rack, preset);
}

void OscDisplayMenuMode::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    display();
    KontrolCallback::loadPreset(source, rack, preset);
}

void OscDisplayMenuMode::midiLearn(Kontrol::ChangeSource src, bool b) {
    display();
    KontrolCallback::midiLearn(src, b);
}

void OscDisplayMenuMode::modulationLearn(Kontrol::ChangeSource src, bool b) {
    display();
    KontrolCallback::modulationLearn(src, b);
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

bool OscDisplayMainMenu::init() {
    return true;
}


unsigned OscDisplayMainMenu::getSize() {
    return (unsigned) OSC_MMI_SIZE;
}

std::string OscDisplayMainMenu::getItemText(unsigned idx) {
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


void OscDisplayMainMenu::clicked(unsigned idx) {
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
                model()->saveSettings(Kontrol::CS_LOCAL, rack->id());
            }
            parent_.changeMode(OSM_MAINMENU);
            break;
        }
        default:
            break;
    }
}

void OscDisplayMainMenu::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) {
    display();
}

// preset menu
enum OscPresetMenuItms {
    OSC_PMI_SAVE,
    OSC_PMI_NEW,
    OSC_PMI_SEP,
    OSC_PMI_LAST
};


bool OscDisplayPresetMenu::init() {
    return true;
}

void OscDisplayPresetMenu::activate() {
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
    OscDisplayMenuMode::activate();
}


unsigned OscDisplayPresetMenu::getSize() {
    return (unsigned) OSC_PMI_LAST + presets_.size();
}

std::string OscDisplayPresetMenu::getItemText(unsigned idx) {
    switch (idx) {
        case OSC_PMI_SAVE:
            return "Save Preset";
        case OSC_PMI_NEW:
            return "New Preset";
        case OSC_PMI_SEP:
            return "--------------------";
        default:
            return presets_[idx - OSC_PMI_LAST];
    }
}


void OscDisplayPresetMenu::clicked(unsigned idx) {
    switch (idx) {
        case OSC_PMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->savePreset(rack->currentPreset());
            }
            parent_.changeMode(OSM_MAINMENU);
            break;
        }
        case OSC_PMI_NEW: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = "new-" + std::to_string(presets_.size());
                model()->savePreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
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
                model()->loadPreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
            }
            break;
        }
    }
}


void OscDisplayModuleMenu::populateMenu(const std::string& catSel) {
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



void OscDisplayModuleMenu::activate() {
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
    OscDisplayFixedMenuMode::activate();
}


void OscDisplayModuleMenu::clicked(unsigned idx) {
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
                    parent_.changeMode(OSM_MAINMENU);
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
    parent_.changeMode(OSM_MAINMENU);
}


void OscDisplayModuleSelectMenu::activate() {
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
    OscDisplayFixedMenuMode::activate();
}


void OscDisplayModuleSelectMenu::clicked(unsigned idx) {
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


// OscDisplay implmentation
class OscDisplayPacketListener : public PacketListener {
public:
    OscDisplayPacketListener(moodycamel::ReaderWriterQueue<OscDisplay::OscMsg> &queue) : queue_(queue) {
    }

    virtual void ProcessPacket(const char *data, int size,
                               const IpEndpointName &remoteEndpoint) {

        OscDisplay::OscMsg msg;
//        msg.origin_ = remoteEndpoint;
        msg.size_ = (size > OscDisplay::OscMsg::MAX_OSC_MESSAGE_SIZE ? OscDisplay::OscMsg::MAX_OSC_MESSAGE_SIZE
                                                                     : size);
        memcpy(msg.buffer_, data, (size_t) msg.size_);
        msg.origin_ = remoteEndpoint;
        queue_.enqueue(msg);
    }

private:
    moodycamel::ReaderWriterQueue<OscDisplay::OscMsg> &queue_;
};


class OscDisplayListener : public osc::OscPacketListener {
public:
    OscDisplayListener(OscDisplay &recv) : receiver_(recv) { ; }


    virtual void ProcessMessage(const osc::ReceivedMessage &m,
                                const IpEndpointName &remoteEndpoint) {
        try {
            char host[IpEndpointName::ADDRESS_STRING_LENGTH];
            remoteEndpoint.AddressAsString(host);
            if (std::strcmp(m.AddressPattern(), "/Connect") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int port = arg->AsInt32();
                if (port > 0) {
                    receiver_.connect(host, port);
                }
            } else if (std::strcmp(m.AddressPattern(), "/NavPrev") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                if (isArgFalse(arg)) return;
                receiver_.navPrev();
            } else if (std::strcmp(m.AddressPattern(), "/NavNext") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                if (isArgFalse(arg)) return;
                receiver_.navNext();
            } else if (std::strcmp(m.AddressPattern(), "/NavActivate") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                if (isArgFalse(arg)) return;
                receiver_.navActivate();
            } else if (std::strcmp(m.AddressPattern(), "/PageNext") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                if (isArgFalse(arg)) return;
                receiver_.nextPage();
            } else if (std::strcmp(m.AddressPattern(), "/PagePrev") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                if (isArgFalse(arg)) return;
                receiver_.prevPage();
            } else if (std::strcmp(m.AddressPattern(), "/ModuleNext") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                if (isArgFalse(arg)) return;
                receiver_.nextModule();
            } else if (std::strcmp(m.AddressPattern(), "/ModulePrev") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                if (isArgFalse(arg)) return;
                receiver_.prevModule();
            } else if (std::strcmp(m.AddressPattern(), "/P1Ctrl") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                float val = 0;
                if (arg->IsFloat()) val = arg->AsFloat();
                else if (arg->IsInt32()) val = arg->AsInt32();
                receiver_.changePot(0, val);
            } else if (std::strcmp(m.AddressPattern(), "/P2Ctrl") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                float val = 0;
                if (arg->IsFloat()) val = arg->AsFloat();
                else if (arg->IsInt32()) val = arg->AsInt32();
                receiver_.changePot(1, val);
            } else if (std::strcmp(m.AddressPattern(), "/P3Ctrl") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                float val = 0;
                if (arg->IsFloat()) val = arg->AsFloat();
                else if (arg->IsInt32()) val = arg->AsInt32();
                receiver_.changePot(2, val);
            } else if (std::strcmp(m.AddressPattern(), "/P4Ctrl") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                float val = 0;
                if (arg->IsFloat()) val = arg->AsFloat();
                else if (arg->IsInt32()) val = arg->AsInt32();
                receiver_.changePot(3, val);
            } else if (std::strcmp(m.AddressPattern(), "/P5Ctrl") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                float val = 0;
                if (arg->IsFloat()) val = arg->AsFloat();
                else if (arg->IsInt32()) val = arg->AsInt32();
                receiver_.changePot(4, val);
            } else if (std::strcmp(m.AddressPattern(), "/P6Ctrl") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                float val = 0;
                if (arg->IsFloat()) val = arg->AsFloat();
                else if (arg->IsInt32()) val = arg->AsInt32();
                receiver_.changePot(5, val);
            } else if (std::strcmp(m.AddressPattern(), "/P7Ctrl") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                float val = 0;
                if (arg->IsFloat()) val = arg->AsFloat();
                else if (arg->IsInt32()) val = arg->AsInt32();
                receiver_.changePot(6, val);
            } else if (std::strcmp(m.AddressPattern(), "/P8Ctrl") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                float val = 0;
                if (arg->IsFloat()) val = arg->AsFloat();
                else if (arg->IsInt32()) val = arg->AsInt32();
                receiver_.changePot(7, val);
            }
        } catch (osc::Exception &e) {
            LOG_0("display osc message exception " << m.AddressPattern() << " : " << e.what());
        }
    }

private:
    bool isArgFalse(osc::ReceivedMessage::const_iterator arg) {
        if(
                (arg->IsFloat() && (arg->AsFloat() >= 0.5))
                || (arg->IsInt32() && (arg->AsInt32() > 0  ))
                ) return false;
        return true;
    }
    OscDisplay &receiver_;
};


OscDisplay::OscDisplay() :
        writeMessageQueue_(OscMsg::MAX_N_OSC_MSGS),
        readMessageQueue_(OscMsg::MAX_N_OSC_MSGS),
        active_(false),
        writeRunning_(false),
        listenRunning_(false),
        modulationLearnActive_(false),
        midiLearnActive_(false) {
    packetListener_ = std::make_shared<OscDisplayPacketListener>(readMessageQueue_);
    oscListener_ = std::make_shared<OscDisplayListener>(*this);
}

OscDisplay::~OscDisplay() {
    deinit();
}


//mec::Device

bool OscDisplay::init(void *arg) {
    Preferences prefs(arg);

    if (active_) {
        LOG_2("OscDisplay::init - already active deinit");
        deinit();
    }
    active_ = false;
    writeRunning_ = false;
    listenRunning_ = false;
    static const auto MENU_TIMEOUT = 350;

    unsigned listenPort = prefs.getInt("listen port", 6100);
    menuTimeout_ = prefs.getInt("menu timeout", MENU_TIMEOUT);


    active_ = true;
    if (active_) {
        paramDisplay_ = std::make_shared<OscDisplayParamMode>(*this);

        // add modes before KD init
        addMode(OSM_MAINMENU, std::make_shared<OscDisplayMainMenu>(*this));
        addMode(OSM_PRESETMENU, std::make_shared<OscDisplayPresetMenu>(*this));
        addMode(OSM_MODULEMENU, std::make_shared<OscDisplayModuleMenu>(*this));
        addMode(OSM_MODULESELECTMENU, std::make_shared<OscDisplayModuleSelectMenu>(*this));

        paramDisplay_->init();
        paramDisplay_->activate();
        changeMode(OSM_MAINMENU);


        listen(listenPort);
    }
    return active_;
}

void OscDisplay::deinit() {
    writeRunning_ = false;
    listenRunning_ = false;

    if (writeSocket_) {
        writer_thread_.join();
        OscMsg msg;
        while (writeMessageQueue_.try_dequeue(msg));
    }
    writeSocket_.reset();

    if (readSocket_) {
        readSocket_->AsynchronousBreak();
        receive_thread_.join();
        OscMsg msg;
        while (readMessageQueue_.try_dequeue(msg));
    }
    listenPort_ = 0;
    readSocket_.reset();
    active_ = false;
    return;
}

bool OscDisplay::isActive() {
    return active_;
}

// Kontrol::KontrolCallback
bool OscDisplay::process() {
    OscMsg msg;
    while (readMessageQueue_.try_dequeue(msg)) {
        oscListener_->ProcessPacket(msg.buffer_, msg.size_, msg.origin_);
    }
    modes_[currentMode_]->poll();
    return true;
}

void OscDisplay::stop() {
    deinit();
}


void *displayosc_write_thread_func(void *aObj) {
    LOG_2("start display osc write thead");
    OscDisplay *pThis = static_cast<OscDisplay *>(aObj);
    pThis->writePoll();
    LOG_2("display osc write thread ended");
    return nullptr;
}

bool OscDisplay::connect(const std::string &hostname, unsigned port) {
    if (writeSocket_) {
        writeRunning_ = false;
        writer_thread_.join();
        OscMsg msg;
        while (writeMessageQueue_.try_dequeue(msg));
    }
    writeSocket_.reset();

    try {
        LOG_0("connecting to client on " << hostname << " : " << port);
        writeSocket_ = std::shared_ptr<UdpTransmitSocket>(
                new UdpTransmitSocket(IpEndpointName(hostname.c_str(), port)));
    } catch (const std::runtime_error &e) {
        LOG_2("could not connect to display osc client for screen updates");
        writeSocket_.reset();
        return false;
    }

    writeRunning_ = true;
#ifdef __COBALT__
    pthread_t ph = writer_thread_.native_handle();
    pthread_create(&ph, 0,displayosc_write_thread_func,this);
#else
    writer_thread_ = std::thread(displayosc_write_thread_func, this);
#endif

    clearDisplay();
    changeMode(OscDisplayModes::OSM_MAINMENU);
    modes_[currentMode_]->activate();
    paramDisplay_->activate();

    // send out current module and page
    auto rack = model()->getRack(currentRack());
    auto module = model()->getModule(rack, currentModule());
    auto page = model()->getPage(module, currentPage());
    std::string md = "";
    std::string pd = "";
    if (module) md = module->id() + " : " +module->displayName();
    if (page) pd = page->displayName();
    displayTitle(md, pd);

    return true;
}


void OscDisplay::writePoll() {
    while (writeRunning_) {
        OscMsg msg;
        if (writeMessageQueue_.wait_dequeue_timed(msg, std::chrono::milliseconds(OSC_WRITE_POLL_WAIT_TIMEOUT))) {
            writeSocket_->Send(msg.buffer_, (size_t) msg.size_);
        }
    }
}


void OscDisplay::send(const char *data, unsigned size) {
    OscMsg msg;
    msg.size_ = (size > OscMsg::MAX_OSC_MESSAGE_SIZE ? OscMsg::MAX_OSC_MESSAGE_SIZE : size);
    memcpy(msg.buffer_, data, (size_t) msg.size_);
    writeMessageQueue_.enqueue(msg);
}


void *displayosc_read_thread_func(void *pReceiver) {
    OscDisplay *pThis = static_cast<OscDisplay *>(pReceiver);
    pThis->readSocket()->Run();
    return nullptr;
}

bool OscDisplay::listen(unsigned port) {

    if (readSocket_) {
        listenRunning_ = false;
        readSocket_->AsynchronousBreak();
        receive_thread_.join();
        OscMsg msg;
        while (readMessageQueue_.try_dequeue(msg));
    }
    listenPort_ = 0;
    readSocket_.reset();


    listenPort_ = port;
    try {
        LOG_0("listening for clients on " << port);
        readSocket_ = std::make_shared<UdpListeningReceiveSocket>(
                IpEndpointName(IpEndpointName::ANY_ADDRESS, listenPort_),
                packetListener_.get());

        listenRunning_ = true;
#ifdef __COBALT__
    pthread_t ph = receive_thread_.native_handle();
    pthread_create(&ph, 0,displayosc_read_thread_func,this);
#else
        receive_thread_ = std::thread(displayosc_read_thread_func, this);
#endif
    } catch (const std::runtime_error &e) {
        listenPort_ = 0;
        return false;
    }
    return true;
}

//--modes and forwarding

void OscDisplay::addMode(OscDisplayModes mode, std::shared_ptr<OscDisplayMode> m) {
    modes_[mode] = m;
}

void OscDisplay::changeMode(OscDisplayModes mode) {
    currentMode_ = mode;
    auto m = modes_[mode];
    m->activate();
}

void OscDisplay::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    modes_[currentMode_]->rack(src, rack);
    paramDisplay_->rack(src, rack);
}

void OscDisplay::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    if (currentModuleId_.empty()) {
        currentRackId_ = rack.id();
        currentModule(module.id());
    }

    modes_[currentMode_]->module(src, rack, module);
    paramDisplay_->module(src, rack, module);
}

void OscDisplay::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                      const Kontrol::Module &module, const Kontrol::Page &page) {
    modes_[currentMode_]->page(src, rack, module, page);
    paramDisplay_->page(src, rack, module, page);
}

void OscDisplay::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                       const Kontrol::Module &module, const Kontrol::Parameter &param) {
    modes_[currentMode_]->param(src, rack, module, param);
    paramDisplay_->param(src, rack, module, param);
}

void OscDisplay::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                         const Kontrol::Module &module, const Kontrol::Parameter &param) {
    modes_[currentMode_]->changed(src, rack, module, param);
    paramDisplay_->changed(src, rack, module, param);
}

void OscDisplay::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                          const std::string &res, const std::string &value) {
    modes_[currentMode_]->resource(src, rack, res, value);
    paramDisplay_->resource(src, rack, res, value);

    if(res=="moduleorder") {
        moduleOrder_.clear();
        if(value.length()>0) {
            int lidx =0;
            int idx = 0;
            int len = 0;
            while((idx=value.find(" ",lidx)) != std::string::npos) {
                len = idx - lidx;
                moduleOrder_.push_back(value.substr(lidx,len));
                lidx = idx + 1;
            }
            len = value.length() - lidx;
            if(len>0) moduleOrder_.push_back(value.substr(lidx,len));
        }
    }
}

void OscDisplay::deleteRack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    modes_[currentMode_]->deleteRack(src, rack);
    paramDisplay_->deleteRack(src, rack);
}

void OscDisplay::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                              const Kontrol::Module &module) {
    modes_[currentMode_]->activeModule(src, rack, module);
    paramDisplay_->activeModule(src, rack, module);
}

void OscDisplay::loadModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                            const Kontrol::EntityId &modId, const std::string &modType) {
    modes_[currentMode_]->loadModule(src, rack, modId, modType);
    paramDisplay_->loadModule(src, rack, modId, modType);
}


void OscDisplay::navPrev() {
    modes_[currentMode_]->navPrev();
}

void OscDisplay::navNext() {
    modes_[currentMode_]->navNext();
}

void OscDisplay::navActivate() {
    modes_[currentMode_]->navActivate();
}


void OscDisplay::nextPage() {
    paramDisplay_->nextPage();
}

void OscDisplay::prevPage() {
    paramDisplay_->prevPage();
}

std::vector<std::shared_ptr<Kontrol::Module>> OscDisplay::getModules(const std::shared_ptr<Kontrol::Rack>& pRack) {
    std::vector<std::shared_ptr<Kontrol::Module>> ret;
    auto modulelist = model()->getModules(pRack);
    std::unordered_set<std::string> done;

    for (auto mid : moduleOrder_) {
        auto pModule = model()->getModule(pRack, mid);
        if(pModule!=nullptr) {
            ret.push_back(pModule);
            done.insert(mid);
        }
    }
    for (auto pModule : modulelist) {
        if(done.find(pModule->id()) == done.end()) {
            ret.push_back(pModule);
        }
    }

    return ret;
}


void OscDisplay::nextModule() {
    auto rack = model()->getRack(currentRack());
    auto modules = getModules(rack);
    bool found = false;
    for (auto module:modules) {
        if (found) {
            currentModule(module->id());
            return;
        }
        if (module->id() == currentModule()) {
            found = true;
        }
    }
}

void OscDisplay::prevModule() {
    auto rack = model()->getRack(currentRack());
    auto modules = getModules(rack);
    Kontrol::EntityId prevId;
    for (auto module:modules) {
        if (module->id() == currentModule()) {
            if (!prevId.empty()) {
                currentModule(prevId);
                return;
            }
        }
        prevId = module->id();
    }
}

void OscDisplay::changePot(unsigned pot, float value) {
    paramDisplay_->changePot(pot, value);
}

void OscDisplay::midiLearn(Kontrol::ChangeSource src, bool b) {
    if(b) modulationLearnActive_ = false;
    midiLearnActive_ = b;
    modes_[currentMode_]->midiLearn(src, b);

}

void OscDisplay::modulationLearn(Kontrol::ChangeSource src, bool b) {
    if(b) midiLearnActive_ = false;
    modulationLearnActive_ = b;
    modes_[currentMode_]->modulationLearn(src, b);
}

void OscDisplay::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    modes_[currentMode_]->savePreset(source, rack, preset);
}

void OscDisplay::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    modes_[currentMode_]->loadPreset(source, rack, preset);
}


void OscDisplay::midiLearn(bool b) {
    model()->midiLearn(Kontrol::CS_LOCAL, b);
}

void OscDisplay::modulationLearn(bool b) {
    model()->modulationLearn(Kontrol::CS_LOCAL, b);
}


//--- display functions

//void OscDisplay::displayPopup(const std::string &text, bool) {
//
//    {
//        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
//        ops << osc::BeginMessage("/text")
//            << (int8_t) 3
//            << osc::EndMessage;
//        send(ops.Data(), ops.Size());
//    }
//}


void OscDisplay::sendOscString(const std::string& topic, std::string value) {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginMessage(topic.c_str());
    if(value.length()) {
        ops << value.c_str();
    } else {
        ops << osc::Nil;
    }
    ops << osc::EndMessage;
    send(ops.Data(), ops.Size());
}

void OscDisplay::clearDisplay() {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginMessage("/clearText")
        << osc::EndMessage;
    send(ops.Data(), ops.Size());
}

void OscDisplay::clearParamNum(unsigned num) {
    std::string p = "P" + std::to_string(num);
    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        std::string field = "/" + p + "Desc";
        const char *addr = field.c_str();
        ops << osc::BeginMessage(addr)
            << osc::Nil
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
            << osc::Nil
            << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }

}

void OscDisplay::displayParamNum(unsigned num, const Kontrol::Parameter &param, bool dispCtrl) {
    std::string p = "P" + std::to_string(num);
    {
        std::string field = "/" + p + "Desc";
        sendOscString(field,param.displayName());
    }

    if (dispCtrl) {
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
        std::string field = "/" + p + "Value";
        std::string val= param.displayValue() + " " + param.displayUnit();
        sendOscString(field,val);

    }
}

void OscDisplay::displayLine(unsigned line, const char *disp) {
    if (writeSocket_ == nullptr) return;

    {
        osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);
        ops << osc::BeginMessage("/text");
        ops << (int8_t) line;
        if(disp && strlen(disp)>0) {
            ops << disp;
        } else {
            ops << osc::Nil;
        }
        ops << osc::EndMessage;
        send(ops.Data(), ops.Size());
    }
}

void OscDisplay::invertLine(unsigned line) {
    osc::OutboundPacketStream ops(screenBuf_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginMessage("/selectText")
        << (int8_t) line
        << osc::EndMessage;

    send(ops.Data(), ops.Size());

}

void OscDisplay::displayTitle(const std::string &module, const std::string &page) {
    sendOscString("/module",module);
    sendOscString("/page",page);
}


void OscDisplay::currentModule(const Kontrol::EntityId &modId) {
    currentModuleId_ = modId;
    model()->activeModule(Kontrol::CS_LOCAL, currentRackId_, currentModuleId_);
}


}
