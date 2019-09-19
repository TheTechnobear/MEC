#include "mec_nui.h"

#include <algorithm>
#include <unordered_set>

#include <mec_log.h>

namespace mec {

static const float MAX_POT_VALUE = 1.0F;

const unsigned SCREEN_HEIGHT=64;
const unsigned SCREEN_WIDTH=128;

static const unsigned NUI_NUM_TEXTLINES = 5;
//static const unsigned NUI_NUM_TEXTCHARS = (128 / 4); = 32
static const unsigned NUI_NUM_TEXTCHARS = 30;
static const unsigned NUI_NUM_PARAMS = 4;
static constexpr unsigned NUI_NUM_BUTTONS = 3;

class NuiBaseMode : public NuiMode {
public:
    explicit NuiBaseMode(Nui &p) : parent_(p), popupTime_(-1) { ; }


    // Kontrol
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

    // NuiDevice
    void onButton(unsigned id, unsigned value) override { buttonState_[id]=value;}
    void onEncoder(unsigned id, int value) override  { ; }

    // Mode
    bool init() override { return true; }
    void poll() override;

    void activate() override { ; }

protected:
    Nui &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    int popupTime_;
    bool buttonState_[NUI_NUM_BUTTONS]= { false,false,false};
};


class NuiParamMode : public NuiBaseMode  {
public:
    explicit NuiParamMode(Nui &p) : NuiBaseMode(p),  pageIdx_(-1) { ; }

    bool init() override { return true; };
    void activate() override;

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
    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;


    void onButton(unsigned id, unsigned value) override;
    void onEncoder(unsigned id, int value) override;

    void nextPage();
    void prevPage();



private:
    void setCurrentPage(unsigned pageIdx, bool UI);
    void display();

    std::string moduleType_;
    int pageIdx_ = -1;
    Kontrol::EntityId pageId_;
};




class NuiMenuMode : public NuiBaseMode {
public:
    explicit NuiMenuMode(Nui &p) : NuiBaseMode(p), cur_(0), top_(0) { ; }

    virtual unsigned getSize() = 0;
    virtual std::string getItemText(unsigned idx) = 0;
    virtual void clicked(unsigned idx) = 0;

    bool init() override { return true; }

    void poll() override;
    void activate() override;
    virtual void navPrev();
    virtual void navNext();
    virtual void navActivate();

    void onButton(unsigned id, unsigned value) override;
    void onEncoder(unsigned id, int value) override;

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


class NuiFixedMenuMode : public NuiMenuMode {
public:
    explicit NuiFixedMenuMode(Nui &p) : NuiMenuMode(p) { ; }

    unsigned getSize() override { return items_.size(); };

    std::string getItemText(unsigned i) override { return items_[i]; }

protected:
    std::vector<std::string> items_;
};

class NuiMainMenu : public NuiMenuMode {
public:
    explicit NuiMainMenu(Nui &p) : NuiMenuMode(p) { ; }

    bool init() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
};

class NuiPresetMenu : public NuiMenuMode {
public:
    explicit NuiPresetMenu(Nui &p) : NuiMenuMode(p) { ; }

    bool init() override;
    void activate() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
private:
    std::vector<std::string> presets_;
};


class NuiModuleMenu : public NuiFixedMenuMode {
public:
    explicit NuiModuleMenu(Nui &p) : NuiFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
private:
    void populateMenu(const std::string& catSel);
    std::string cat_;
};

class NuiModuleSelectMenu : public NuiFixedMenuMode {
public:
    explicit NuiModuleSelectMenu(Nui &p) : NuiFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};

//------ NuiBaseMode

void NuiBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

//------ NuiParamMode

void NuiParamMode::display() {
    parent_.clearDisplay();
    auto rack = parent_.model()->getRack(parent_.currentRack());
    auto module = parent_.model()->getModule(rack, parent_.currentModule());
    auto page = parent_.model()->getPage(module, pageId_);
//    auto pages = parent_.model()->getPages(module);
    auto params = parent_.model()->getParams(module, page);

    std::string md = "";
    std::string pd = "";
    if (module) md = module->id() + " : " +module->displayName();
    if (page) pd = page->displayName();
    parent_.displayTitle(md, pd);


    unsigned int j = 0;
    for (auto param : params) {
        if (param != nullptr) {
            parent_.displayParamNum(j + 1, *param, true);
        }
        j++;
        if (j == NUI_NUM_PARAMS) break;
    }

    for (; j < NUI_NUM_PARAMS; j++) {
        parent_.clearParamNum(j + 1);
    }
}


void NuiParamMode::activate() {
    NuiBaseMode::activate();
    display();
}

void NuiParamMode::onButton(unsigned id, unsigned value) {
    NuiBaseMode::onButton(id,value);
    switch (id) {
        case 0 : {
            if(!value) {
                // on release of button
                parent_.changeMode(NM_MAINMENU);
            }
            break;
        }
        case 1 : {
            break;
        }
        case 2 : {
            break;
        }
        default:
		 ;
    }
}

void NuiParamMode::onEncoder(unsigned idx, int v) {
    NuiBaseMode::onEncoder(idx,v);
    if(idx==2 && buttonState_[2]) {
        // if holding button 3. then turning encoder 3 changed page
        if(v>0) {
            nextPage();
        } else {
            prevPage();
        }

    } else if(idx==1 && buttonState_[2]) {
        // if holding button 3. then turning encoder 2 changed module
        if(v>0) {
            parent_.nextModule();
        } else {
            parent_.prevModule();
	}
    } else {
        try {
            auto pRack = model()->getRack(parent_.currentRack());
            auto pModule = model()->getModule(pRack, parent_.currentModule());
            auto pPage = model()->getPage(pModule, parent_.currentPage());
            auto pParams = model()->getParams(pModule, pPage);

            if (idx >= pParams.size()) return;

            auto &param = pParams[idx];
            // auto page = pages_[currentPage_]
            if (param != nullptr) {
                const float steps = 128.0f;
            float value = float(v) / steps;
                Kontrol::ParamValue calc = param->calcRelative(value);
                //std::cerr << "onEncoder " << idx << " " << value << " cv " << calc.floatValue() << " pv " << param->current().floatValue() << std::endl;
                model()->changeParam(Kontrol::CS_LOCAL, parent_.currentRack(), pModule->id(), param->id(), calc);
            }
        } catch (std::out_of_range) {
        }
   }
}



void NuiParamMode::setCurrentPage(unsigned pageIdx, bool UI) {
    auto module = model()->getModule(model()->getRack(parent_.currentRack()), parent_.currentModule());

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
	   	parent_.clearDisplay();
		std::string md = "";
		std::string pd = "";
		if (module) md = module->id() + " : " +module->displayName();
		parent_.displayTitle(md, "none");
            }
        }

    } catch (std::out_of_range) { ;
    }
}


void NuiParamMode::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module & module) {
    if (rack.id() == parent_.currentRack()) {
        if(src!= Kontrol::CS_LOCAL &&  module.id() != parent_.currentModule()) {
            parent_.currentModule(module.id());
        }
        pageIdx_ = -1;
        setCurrentPage(0, false);
    }
}


void NuiParamMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                                  const Kontrol::Parameter &param) {
    if (rack.id() != parent_.currentRack() || module.id() != parent_.currentModule()) return;

    auto prack = parent_.model()->getRack(parent_.currentRack());
    auto pmodule = parent_.model()->getModule(prack, parent_.currentModule());
    auto page = parent_.model()->getPage(pmodule, pageId_);
//    auto pages = parent_.model()->getPages(pmodule);
    auto params = parent_.model()->getParams(pmodule, page);


    unsigned sz = params.size();
    sz = sz < NUI_NUM_PARAMS ? sz : NUI_NUM_PARAMS;
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

void NuiParamMode::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                 const Kontrol::Module &module) {
    if (moduleType_ != module.type()) {
        pageIdx_ = -1;
    }
    moduleType_ = module.type();
}

void NuiParamMode::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                               const Kontrol::Page &page) {
    if (pageIdx_ < 0) setCurrentPage(0, false);
}


void NuiParamMode::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                     const Kontrol::EntityId &moduleId, const std::string &modType) {
    if (parent_.currentModule() == moduleId) {
        if (moduleType_ != modType) {
            pageIdx_ = -1;
            moduleType_ = modType;
        }
    }
}

void NuiParamMode::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
	pageIdx_ = -1;
	setCurrentPage(0,false);
}

void NuiParamMode::nextPage() {
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

void NuiParamMode::prevPage() {
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
    NuiBaseMode::onButton(id,value);
    switch (id) {
        case 0 : {
            if(!value) {
                // on release of button
                parent_.changeMode(NM_PARAMETER);
            }
            break;
        }
        case 1 : {
            break;
        }
        case 2 : {
            if(!value) {
                navActivate();
            }
            break;
        }
        default:
		 ;
    }
}

void NuiMenuMode::onEncoder(unsigned id, int value) {
    if(id==0) {
        if(value>0) {
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


void NuiModuleMenu::populateMenu(const std::string& catSel) {
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



void NuiModuleMenu::activate() {
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
            if(cat_.length()) {
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


Nui::Nui() :
        active_(false),
        modulationLearnActive_(false),
        midiLearnActive_(false) {
}

Nui::~Nui() {
    deinit();
}


//mec::Device

bool Nui::init(void *arg) {
    Preferences prefs(arg);
    std::shared_ptr<NuiLite::NuiCallback> cb=std::make_shared<NuiDeviceCallback>(*this);
    device_.addCallback(cb);
    device_.start();

    if (active_) {
        LOG_2("Nui::init - already active deinit");
        deinit();
    }
    active_ = false;
    static const auto MENU_TIMEOUT = 2000;

    // TOODO - prefs : font directory, slashscreen
    menuTimeout_ = prefs.getInt("menu timeout", MENU_TIMEOUT);


    active_ = true;
    if (active_) {
        // add modes before KD init
        addMode(NM_PARAMETER, std::make_shared<NuiParamMode>(*this));
        addMode(NM_MAINMENU, std::make_shared<NuiMainMenu>(*this));
        addMode(NM_PRESETMENU, std::make_shared<NuiPresetMenu>(*this));
        addMode(NM_MODULEMENU, std::make_shared<NuiModuleMenu>(*this));
        addMode(NM_MODULESELECTMENU, std::make_shared<NuiModuleSelectMenu>(*this));

        changeMode(NM_PARAMETER);
    }
    device_.drawPNG(0,0,"./oracsplash4.png");
    device_.displayText(0,"Connecting to ORAC...");
    return active_;
}

void Nui::deinit() {
    device_.stop();
    active_ = false;
    return;
}

bool Nui::isActive() {
    return active_;
}

// Kontrol::KontrolCallback
bool Nui::process() {
    modes_[currentMode_]->poll();
    device_.process();
    return true;
}

void Nui::stop() {
    deinit();
}

bool Nui::connect(const std::string &hostname, unsigned port) {
    clearDisplay();

    // send out current module and page
    auto rack = model()->getRack(currentRack());
    auto module = model()->getModule(rack, currentModule());
    auto page = model()->getPage(module, currentPage());
    std::string md = "";
    std::string pd = "";
    if (module) md = module->id() + " : " +module->displayName();
    if (page) pd = page->displayName();
    displayTitle(md, pd);

    changeMode(NuiModes::NM_PARAMETER);
    modes_[currentMode_]->activate();


    return true;
}


//--modes and forwarding
void Nui::addMode(NuiModes mode, std::shared_ptr<NuiMode> m) {
    modes_[mode] = m;
}

void Nui::changeMode(NuiModes mode) {
    currentMode_ = mode;
    auto m = modes_[mode];
    m->activate();
}

void Nui::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    modes_[currentMode_]->rack(src, rack);
}

void Nui::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    if (currentModuleId_.empty()) {
        currentRackId_ = rack.id();
        currentModule(module.id());
    }

    modes_[currentMode_]->module(src, rack, module);
}

void Nui::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                      const Kontrol::Module &module, const Kontrol::Page &page) {
    modes_[currentMode_]->page(src, rack, module, page);
}

void Nui::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                       const Kontrol::Module &module, const Kontrol::Parameter &param) {
    modes_[currentMode_]->param(src, rack, module, param);
}

void Nui::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                         const Kontrol::Module &module, const Kontrol::Parameter &param) {
    modes_[currentMode_]->changed(src, rack, module, param);
}

void Nui::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                          const std::string &res, const std::string &value) {
    modes_[currentMode_]->resource(src, rack, res, value);

    if(res=="moduleorder") {
        moduleOrder_.clear();
        if(value.length()>0) {
            int lidx =0;
            int idx = 0;
            int len = 0;
            while((idx=value.find(" ",lidx)) != std::string::npos) {
                len = idx - lidx;
		std::string mid = value.substr(lidx,len);
                moduleOrder_.push_back(mid);
                lidx = idx + 1;
            }
            len = value.length() - lidx;
            if(len>0)  {
		std::string mid = value.substr(lidx,len);
		moduleOrder_.push_back(mid);
	    }
        }
    }
}

void Nui::deleteRack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    modes_[currentMode_]->deleteRack(src, rack);
}

void Nui::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                              const Kontrol::Module &module) {
    modes_[currentMode_]->activeModule(src, rack, module);
}

void Nui::loadModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                            const Kontrol::EntityId &modId, const std::string &modType) {
    modes_[currentMode_]->loadModule(src, rack, modId, modType);
}

std::vector<std::shared_ptr<Kontrol::Module>> Nui::getModules(const std::shared_ptr<Kontrol::Rack>& pRack) {
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


void Nui::nextModule() {
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

void Nui::prevModule() {
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

void Nui::midiLearn(Kontrol::ChangeSource src, bool b) {
    if(b) modulationLearnActive_ = false;
    midiLearnActive_ = b;
    modes_[currentMode_]->midiLearn(src, b);

}

void Nui::modulationLearn(Kontrol::ChangeSource src, bool b) {
    if(b) midiLearnActive_ = false;
    modulationLearnActive_ = b;
    modes_[currentMode_]->modulationLearn(src, b);
}

void Nui::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    modes_[currentMode_]->savePreset(source, rack, preset);
}

void Nui::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    modes_[currentMode_]->loadPreset(source, rack, preset);
}


void Nui::onButton(unsigned id, unsigned value) {
    modes_[currentMode_]->onButton(id,value);

}
void Nui::onEncoder(unsigned id, int value) {
    modes_[currentMode_]->onEncoder(id,value);
}



void Nui::midiLearn(bool b) {
    model()->midiLearn(Kontrol::CS_LOCAL, b);
}

void Nui::modulationLearn(bool b) {
    model()->modulationLearn(Kontrol::CS_LOCAL, b);
}


//--- display functions

void Nui::displayPopup(const std::string &text, bool) {
    // temp
    std::string txt="|      ";
    txt=txt+ "     |";
    displayLine(1, "----------------------------------");
    displayLine(2, "|");
    displayLine(3, "----------------------------------");
}



void Nui::clearDisplay() {
    device_.displayClear();
}

void Nui::clearParamNum(unsigned num) {
    device_.clearText(num);
}


std::string asDisplayString(const Kontrol::Parameter &param, unsigned width){
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


void Nui::displayParamNum(unsigned num, const Kontrol::Parameter &param, bool dispCtrl) {
    //std::string disp = asDisplayString(param, NUI_NUM_TEXTCHARS);
    //displayLine(num, disp.c_str());
    const std::string &dName = param.displayName();
    std::string value = param.displayValue();
    std::string unit = param.displayUnit();
    device_.clearText(num);
    device_.displayText(num,0,dName.c_str());
    device_.displayText(num,17,value.c_str());
    device_.displayText(num,27,unit.c_str());
}

void Nui::displayLine(unsigned line, const char *disp) {
    device_.clearText(line);
    device_.displayText(line,disp);
}

void Nui::invertLine(unsigned line) {
    device_.invertText(line);
}

void Nui::displayTitle(const std::string &module, const std::string &page) {
    if(module.size() == 0 || page.size()==0) return;
    std::string title= module + " > " + page ;
    device_.clearText(0);
    device_.displayText(0,title);
}


void Nui::currentModule(const Kontrol::EntityId &modId) {
    currentModuleId_ = modId;
    model()->activeModule(Kontrol::CS_LOCAL, currentRackId_, currentModuleId_);
}


}
