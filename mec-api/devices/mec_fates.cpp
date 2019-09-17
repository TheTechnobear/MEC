#include "mec_fates.h"

#include <algorithm>
#include <unordered_set>

#include <mec_log.h>

namespace mec {

static const float MAX_POT_VALUE = 1.0F;

const unsigned SCREEN_HEIGHT=64;
const unsigned SCREEN_WIDTH=128;

static const unsigned FATES_NUM_TEXTLINES = 5;
//static const unsigned FATES_NUM_TEXTCHARS = (128 / 4); = 32
static const unsigned FATES_NUM_TEXTCHARS = 30;
static const unsigned FATES_NUM_PARAMS = 4;
static constexpr unsigned FATES_NUM_BUTTONS = 3;

class FatesBaseMode : public FatesMode {
public:
    explicit FatesBaseMode(Fates &p) : parent_(p), popupTime_(-1) { ; }


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

    // FatesDevice
    void onButton(unsigned id, unsigned value) override { buttonState_[id]=value;}
    void onEncoder(unsigned id, int value) override  { ; }

    // Mode
    bool init() override { return true; }
    void poll() override;

    void activate() override { ; }

protected:
    Fates &parent_;

    std::shared_ptr<Kontrol::KontrolModel> model() { return parent_.model(); }

    int popupTime_;
    bool buttonState_[FATES_NUM_BUTTONS]= { false,false,false};
};


class FatesParamMode : public FatesBaseMode  {
public:
    explicit FatesParamMode(Fates &p) : FatesBaseMode(p),  pageIdx_(-1) { ; }

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




class FatesMenuMode : public FatesBaseMode {
public:
    explicit FatesMenuMode(Fates &p) : FatesBaseMode(p), cur_(0), top_(0) { ; }

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


class FatesFixedMenuMode : public FatesMenuMode {
public:
    explicit FatesFixedMenuMode(Fates &p) : FatesMenuMode(p) { ; }

    unsigned getSize() override { return items_.size(); };

    std::string getItemText(unsigned i) override { return items_[i]; }

protected:
    std::vector<std::string> items_;
};

class FatesMainMenu : public FatesMenuMode {
public:
    explicit FatesMainMenu(Fates &p) : FatesMenuMode(p) { ; }

    bool init() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
};

class FatesPresetMenu : public FatesMenuMode {
public:
    explicit FatesPresetMenu(Fates &p) : FatesMenuMode(p) { ; }

    bool init() override;
    void activate() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
private:
    std::vector<std::string> presets_;
};


class FatesModuleMenu : public FatesFixedMenuMode {
public:
    explicit FatesModuleMenu(Fates &p) : FatesFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
private:
    void populateMenu(const std::string& catSel);
    std::string cat_;
};

class FatesModuleSelectMenu : public FatesFixedMenuMode {
public:
    explicit FatesModuleSelectMenu(Fates &p) : FatesFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};

//------ FatesBaseMode

void FatesBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

//------ FatesParamMode

void FatesParamMode::display() {
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
        if (j == FATES_NUM_PARAMS) break;
    }

    for (; j < FATES_NUM_PARAMS; j++) {
        parent_.clearParamNum(j + 1);
    }
}


void FatesParamMode::activate() {
    FatesBaseMode::activate();
    display();
}

void FatesParamMode::onButton(unsigned id, unsigned value) {
    FatesBaseMode::onButton(id,value);
    switch (id) {
        case 0 : {
            if(!value) {
                // on release of button
                parent_.changeMode(FM_MAINMENU);
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

void FatesParamMode::onEncoder(unsigned idx, int v) {
    FatesBaseMode::onEncoder(idx,v);
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



void FatesParamMode::setCurrentPage(unsigned pageIdx, bool UI) {
    auto module = model()->getModule(model()->getRack(parent_.currentRack()), parent_.currentModule());

    try {
        auto rack = parent_.model()->getRack(parent_.currentRack());
        auto module = parent_.model()->getModule(rack, parent_.currentModule());
        auto page = parent_.model()->getPage(module, pageId_);
        auto pages = parent_.model()->getPages(module);
//        auto params = parent_.model()->getParams(module,page);

        if (pageIdx_ != pageIdx) {
            pageIdx_ = pageIdx;
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


void FatesParamMode::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module & module) {
    if (rack.id() == parent_.currentRack()) {
        if(src!= Kontrol::CS_LOCAL &&  module.id() != parent_.currentModule()) {
            parent_.currentModule(module.id());
        }
        pageIdx_ = -1;
        setCurrentPage(0, false);
    }
}


void FatesParamMode::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                                  const Kontrol::Parameter &param) {
    if (rack.id() != parent_.currentRack() || module.id() != parent_.currentModule()) return;

    auto prack = parent_.model()->getRack(parent_.currentRack());
    auto pmodule = parent_.model()->getModule(prack, parent_.currentModule());
    auto page = parent_.model()->getPage(pmodule, pageId_);
//    auto pages = parent_.model()->getPages(pmodule);
    auto params = parent_.model()->getParams(pmodule, page);


    unsigned sz = params.size();
    sz = sz < FATES_NUM_PARAMS ? sz : FATES_NUM_PARAMS;
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

void FatesParamMode::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                 const Kontrol::Module &module) {
    if (moduleType_ != module.type()) {
        pageIdx_ = -1;
    }
    moduleType_ = module.type();
}

void FatesParamMode::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                               const Kontrol::Page &page) {
    if (pageIdx_ < 0) setCurrentPage(0, false);
}


void FatesParamMode::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                                     const Kontrol::EntityId &moduleId, const std::string &modType) {
    if (parent_.currentModule() == moduleId) {
        if (moduleType_ != modType) {
            pageIdx_ = -1;
            moduleType_ = modType;
        }
    }
}

void FatesParamMode::nextPage() {
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

void FatesParamMode::prevPage() {
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


//---- FatesMenuMode
void FatesMenuMode::activate() {
    display();
    popupTime_ = parent_.menuTimeout();
}

void FatesMenuMode::poll() {
    FatesBaseMode::poll();
    if (popupTime_ == 0) {
        parent_.changeMode(FM_PARAMETER);
        popupTime_ = -1;
    }
}

void FatesMenuMode::display() {
    parent_.clearDisplay();
    for (unsigned i = top_; i < top_ + FATES_NUM_TEXTLINES; i++) {
        displayItem(i);
    }
}

void FatesMenuMode::displayItem(unsigned i) {
    if (i < getSize()) {
        std::string item = getItemText(i);
        unsigned line = i - top_ + 1;
        parent_.displayLine(line, item.c_str());
        if (i == cur_) {
            parent_.invertLine(line);
        }
    }
}

void FatesMenuMode::onButton(unsigned id, unsigned value) {
    FatesBaseMode::onButton(id,value);
    switch (id) {
        case 0 : {
            if(!value) {
                // on release of button
                parent_.changeMode(FM_PARAMETER);
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

void FatesMenuMode::onEncoder(unsigned id, int value) {
    if(id==0) {
        if(value>0) {
            navNext();
        } else {
            navPrev();
        }
    }

}


void FatesMenuMode::navPrev() {
    unsigned cur = cur_;
    if (cur_ > 0) {
        cur--;
        unsigned int line = 0;
        if (cur < top_) {
            top_ = cur;
            cur_ = cur;
            display();
        } else if (cur >= top_ + FATES_NUM_TEXTLINES) {
            top_ = cur - (FATES_NUM_TEXTLINES - 1);
            cur_ = cur;
            display();
        } else {
            line = cur_ - top_ + 1;
            if (line <= FATES_NUM_TEXTLINES) parent_.invertLine(line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= FATES_NUM_TEXTLINES) parent_.invertLine(line);
        }
    }
    popupTime_ = parent_.menuTimeout();
}


void FatesMenuMode::navNext() {
    unsigned cur = cur_;
    cur++;
    cur = std::min(cur, getSize() - 1);
    if (cur != cur_) {
        unsigned int line = 0;
        if (cur < top_) {
            top_ = cur;
            cur_ = cur;
            display();
        } else if (cur >= top_ + FATES_NUM_TEXTLINES) {
            top_ = cur - (FATES_NUM_TEXTLINES - 1);
            cur_ = cur;
            display();
        } else {
            line = cur_ - top_ + 1;
            if (line <= FATES_NUM_TEXTLINES) parent_.invertLine(line);
            cur_ = cur;
            line = cur_ - top_ + 1;
            if (line <= FATES_NUM_TEXTLINES) parent_.invertLine(line);
        }
    }
    popupTime_ = parent_.menuTimeout();
}


void FatesMenuMode::navActivate() {
    clicked(cur_);
}

void FatesMenuMode::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    display();
    KontrolCallback::savePreset(source, rack, preset);
}

void FatesMenuMode::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    display();
    KontrolCallback::loadPreset(source, rack, preset);
}

void FatesMenuMode::midiLearn(Kontrol::ChangeSource src, bool b) {
    display();
    KontrolCallback::midiLearn(src, b);
}

void FatesMenuMode::modulationLearn(Kontrol::ChangeSource src, bool b) {
    display();
    KontrolCallback::modulationLearn(src, b);
}


/// main menu
enum OscMainMenuItms {
    FATES_MMI_MODULE,
    FATES_MMI_PRESET,
    FATES_MMI_MIDILEARN,
    FATES_MMI_MODLEARN,
    FATES_MMI_SAVE,
    FATES_MMI_SIZE
};

bool FatesMainMenu::init() {
    return true;
}


unsigned FatesMainMenu::getSize() {
    return (unsigned) FATES_MMI_SIZE;
}

std::string FatesMainMenu::getItemText(unsigned idx) {
    switch (idx) {
        case FATES_MMI_MODULE: {
            auto rack = model()->getRack(parent_.currentRack());
            auto module = model()->getModule(rack, parent_.currentModule());
            if (module == nullptr)
                return parent_.currentModule();
            else
                return parent_.currentModule() + ":" + module->displayName();
        }
        case FATES_MMI_PRESET: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                return rack->currentPreset();
            }
            return "No Preset";
        }
        case FATES_MMI_SAVE:
            return "Save";
        case FATES_MMI_MIDILEARN: {
            if (parent_.midiLearn()) {
                return "Midi Learn        [X]";
            }
            return "Midi Learn        [ ]";
        }
        case FATES_MMI_MODLEARN: {
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


void FatesMainMenu::clicked(unsigned idx) {
    switch (idx) {
        case FATES_MMI_MODULE: {
            parent_.changeMode(FM_MODULEMENU);
            break;
        }
        case FATES_MMI_PRESET: {
            parent_.changeMode(FM_PRESETMENU);
            break;
        }
        case FATES_MMI_MIDILEARN: {
            parent_.midiLearn(!parent_.midiLearn());
            displayItem(FATES_MMI_MIDILEARN);
            displayItem(FATES_MMI_MODLEARN);
            // parent_.changeMode(FM_PARAMETER);
            break;
        }
        case FATES_MMI_MODLEARN: {
            parent_.modulationLearn(!parent_.modulationLearn());
            displayItem(FATES_MMI_MIDILEARN);
            displayItem(FATES_MMI_MODLEARN);
            // parent_.changeMode(FM_PARAMETER);
            break;
        }
        case FATES_MMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                model()->saveSettings(Kontrol::CS_LOCAL, rack->id());
            }
            parent_.changeMode(FM_MAINMENU);
            break;
        }
        default:
            break;
    }
}

void FatesMainMenu::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) {
    display();
}

// preset menu
enum OscPresetMenuItms {
    FATES_PMI_SAVE,
    FATES_PMI_NEW,
    FATES_PMI_SEP,
    FATES_PMI_LAST
};


bool FatesPresetMenu::init() {
    return true;
}

void FatesPresetMenu::activate() {
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
    FatesMenuMode::activate();
}


unsigned FatesPresetMenu::getSize() {
    return (unsigned) FATES_PMI_LAST + presets_.size();
}

std::string FatesPresetMenu::getItemText(unsigned idx) {
    switch (idx) {
        case FATES_PMI_SAVE:
            return "Save Preset";
        case FATES_PMI_NEW:
            return "New Preset";
        case FATES_PMI_SEP:
            return "--------------------";
        default:
            return presets_[idx - FATES_PMI_LAST];
    }
}


void FatesPresetMenu::clicked(unsigned idx) {
    switch (idx) {
        case FATES_PMI_SAVE: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                rack->savePreset(rack->currentPreset());
            }
            parent_.changeMode(FM_MAINMENU);
            break;
        }
        case FATES_PMI_NEW: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = "new-" + std::to_string(presets_.size());
                model()->savePreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
            }
            parent_.changeMode(FM_MAINMENU);
            break;
        }
        case FATES_PMI_SEP: {
            break;
        }
        default: {
            auto rack = model()->getRack(parent_.currentRack());
            if (rack != nullptr) {
                std::string newPreset = presets_[idx - FATES_PMI_LAST];
                parent_.changeMode(FM_MAINMENU);
                model()->loadPreset(Kontrol::CS_LOCAL, rack->id(), newPreset);
            }
            break;
        }
    }
}


void FatesModuleMenu::populateMenu(const std::string& catSel) {
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



void FatesModuleMenu::activate() {
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
    FatesFixedMenuMode::activate();
}


void FatesModuleMenu::clicked(unsigned idx) {
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
                    parent_.changeMode(FM_MAINMENU);
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
    parent_.changeMode(FM_MAINMENU);
}


void FatesModuleSelectMenu::activate() {
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
    FatesFixedMenuMode::activate();
}


void FatesModuleSelectMenu::clicked(unsigned idx) {
    parent_.changeMode(FM_MAINMENU);
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


Fates::Fates() :
        active_(false),
        modulationLearnActive_(false),
        midiLearnActive_(false) {
}

Fates::~Fates() {
    deinit();
}


//mec::Device

bool Fates::init(void *arg) {
    Preferences prefs(arg);
    std::shared_ptr<FatesLite::FatesCallback> cb=std::make_shared<FatesDeviceCallback>(*this);
    device_.addCallback(cb);
    device_.start();

    if (active_) {
        LOG_2("Fates::init - already active deinit");
        deinit();
    }
    active_ = false;
    static const auto MENU_TIMEOUT = 350;

    menuTimeout_ = prefs.getInt("menu timeout", MENU_TIMEOUT);


    active_ = true;
    if (active_) {
        // add modes before KD init
        addMode(FM_PARAMETER, std::make_shared<FatesParamMode>(*this));
        addMode(FM_MAINMENU, std::make_shared<FatesMainMenu>(*this));
        addMode(FM_PRESETMENU, std::make_shared<FatesPresetMenu>(*this));
        addMode(FM_MODULEMENU, std::make_shared<FatesModuleMenu>(*this));
        addMode(FM_MODULESELECTMENU, std::make_shared<FatesModuleSelectMenu>(*this));

        changeMode(FM_PARAMETER);
    }
    return active_;
}

void Fates::deinit() {
    device_.stop();
    active_ = false;
    return;
}

bool Fates::isActive() {
    return active_;
}

// Kontrol::KontrolCallback
bool Fates::process() {
    modes_[currentMode_]->poll();
    device_.process();
    return true;
}

void Fates::stop() {
    deinit();
}

bool Fates::connect(const std::string &hostname, unsigned port) {
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

    changeMode(FatesModes::FM_PARAMETER);
    modes_[currentMode_]->activate();


    return true;
}


//--modes and forwarding
void Fates::addMode(FatesModes mode, std::shared_ptr<FatesMode> m) {
    modes_[mode] = m;
}

void Fates::changeMode(FatesModes mode) {
    currentMode_ = mode;
    auto m = modes_[mode];
    m->activate();
}

void Fates::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    modes_[currentMode_]->rack(src, rack);
}

void Fates::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    if (currentModuleId_.empty()) {
        currentRackId_ = rack.id();
        currentModule(module.id());
    }

    modes_[currentMode_]->module(src, rack, module);
}

void Fates::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                      const Kontrol::Module &module, const Kontrol::Page &page) {
    modes_[currentMode_]->page(src, rack, module, page);
}

void Fates::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                       const Kontrol::Module &module, const Kontrol::Parameter &param) {
    modes_[currentMode_]->param(src, rack, module, param);
}

void Fates::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                         const Kontrol::Module &module, const Kontrol::Parameter &param) {
    modes_[currentMode_]->changed(src, rack, module, param);
}

void Fates::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
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

void Fates::deleteRack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    modes_[currentMode_]->deleteRack(src, rack);
}

void Fates::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                              const Kontrol::Module &module) {
    modes_[currentMode_]->activeModule(src, rack, module);
}

void Fates::loadModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                            const Kontrol::EntityId &modId, const std::string &modType) {
    modes_[currentMode_]->loadModule(src, rack, modId, modType);
}

std::vector<std::shared_ptr<Kontrol::Module>> Fates::getModules(const std::shared_ptr<Kontrol::Rack>& pRack) {
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


void Fates::nextModule() {
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

void Fates::prevModule() {
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

void Fates::midiLearn(Kontrol::ChangeSource src, bool b) {
    if(b) modulationLearnActive_ = false;
    midiLearnActive_ = b;
    modes_[currentMode_]->midiLearn(src, b);

}

void Fates::modulationLearn(Kontrol::ChangeSource src, bool b) {
    if(b) midiLearnActive_ = false;
    modulationLearnActive_ = b;
    modes_[currentMode_]->modulationLearn(src, b);
}

void Fates::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    modes_[currentMode_]->savePreset(source, rack, preset);
}

void Fates::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    modes_[currentMode_]->loadPreset(source, rack, preset);
}


void Fates::onButton(unsigned id, unsigned value) {
    modes_[currentMode_]->onButton(id,value);

}
void Fates::onEncoder(unsigned id, int value) {
    modes_[currentMode_]->onEncoder(id,value);
}



void Fates::midiLearn(bool b) {
    model()->midiLearn(Kontrol::CS_LOCAL, b);
}

void Fates::modulationLearn(bool b) {
    model()->modulationLearn(Kontrol::CS_LOCAL, b);
}


//--- display functions

void Fates::displayPopup(const std::string &text, bool) {
    // temp
    std::string txt="|      ";
    txt=txt+ "     |";
    displayLine(1, "----------------------------------");
    displayLine(2, "|");
    displayLine(3, "----------------------------------");
}



void Fates::clearDisplay() {
    device_.displayClear();
}

void Fates::clearParamNum(unsigned num) {
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


void Fates::displayParamNum(unsigned num, const Kontrol::Parameter &param, bool dispCtrl) {
    //std::string disp = asDisplayString(param, FATES_NUM_TEXTCHARS);
    //displayLine(num, disp.c_str());
    const std::string &dName = param.displayName();
    std::string value = param.displayValue();
    std::string unit = param.displayUnit();
    device_.clearText(num);
    device_.displayText(num,0,dName.c_str());
    device_.displayText(num,17,value.c_str());
    device_.displayText(num,27,unit.c_str());
}

void Fates::displayLine(unsigned line, const char *disp) {
    device_.clearText(line);
    device_.displayText(line,disp);
}

void Fates::invertLine(unsigned line) {
    device_.invertText(line);
}

void Fates::displayTitle(const std::string &module, const std::string &page) {
    if(module.size() == 0 || page.size()==0) return;
    std::string title= module + " > " + page ;
    device_.clearText(0);
    device_.displayText(0,title);
}


void Fates::currentModule(const Kontrol::EntityId &modId) {
    currentModuleId_ = modId;
    model()->activeModule(Kontrol::CS_LOCAL, currentRackId_, currentModuleId_);
}


}
