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

    static const auto MENU_TIMEOUT = 2000;
    menuTimeout_ = prefs.getInt("menu timeout", MENU_TIMEOUT);
    std::string res = prefs.getString("resource path", "/home/we/norns/resources");
    if(!device_) return;

    device_=std::make_shared<NuiLite::NuiDevice>(res.c_str());

    unsigned parammode = prefs.getInt("param display",0);

    std::shared_ptr<NuiLite::NuiCallback> cb=std::make_shared<NuiDeviceCallback>(*this);
    device_->addCallback(cb);
    device_->start();

    if (active_) {
        LOG_2("Nui::init - already active deinit");
        deinit();
    }
    active_ = false;



    active_ = true;
    if (active_) {
        switch(parammode) {
            case 0:
            default:
                addMode(NM_PARAMETER, std::make_shared<NuiParamMode1>(*this));
                break;
        }
        addMode(NM_MAINMENU, std::make_shared<NuiMainMenu>(*this));
        addMode(NM_PRESETMENU, std::make_shared<NuiPresetMenu>(*this));
        addMode(NM_MODULEMENU, std::make_shared<NuiModuleMenu>(*this));
//        addMode(NM_MODULESELECTMENU, std::make_shared<NuiModuleSelectMenu>(*this));

        changeMode(NM_PARAMETER);
    }
    device_->drawPNG(0,0,splash.c_str());
    device_->displayText(0,"Connecting to ORAC...");
    return active_;
}

void Nui::deinit() {
    device_->stop();
    device_= nullptr;
    active_ = false;
    return;
}

bool Nui::isActive() {
    return active_;
}

// Kontrol::KontrolCallback
bool Nui::process() {
    modes_[currentMode_]->poll();
    if(device_) device_->process();
    return true;
}

void Nui::stop() {
    deinit();
}

//bool Nui::connect(const std::string &hostname, unsigned port) {
//    clearDisplay();
//
//    // send out current module and page
//    auto rack = model()->getRack(currentRack());
//    auto module = model()->getModule(rack, currentModule());
//    auto page = model()->getPage(module, currentPage());
//    std::string md = "";
//    std::string pd = "";
//    if (module) md = module->id() + " : " +module->displayName();
//    if (page) pd = page->displayName();
//    displayTitle(md, pd);
//
//    changeMode(NuiModes::NM_PARAMETER);
//    modes_[currentMode_]->activate();
//
//
//    return true;
//}


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
    if(!device_) return;
    device_->displayClear();
}

void Nui::clearParamNum(unsigned num) {
    if(!device_) return;
    device_->clearText(num);
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
    if(!device_) return;
    const std::string &dName = param.displayName();
    std::string value = param.displayValue();
    std::string unit = param.displayUnit();
    device_->clearText(num);
    device_->displayText(num,0,dName.c_str());
    device_->displayText(num,17,value.c_str());
    device_->displayText(num,27,unit.c_str());
}

void Nui::displayLine(unsigned line, const char *disp) {
    if(!device_) return;
    device_->clearText(line);
    device_->displayText(line,disp);
}

void Nui::invertLine(unsigned line) {
    if(!device_) return;
    device_->invertText(line);
}

void Nui::displayTitle(const std::string &module, const std::string &page) {
    if(!device_) return;
    if(module.size() == 0 || page.size()==0) return;
    std::string title= module + " > " + page ;
    device_->clearText(0);
    device_->displayText(0,title);
}


void Nui::currentModule(const Kontrol::EntityId &modId) {
    currentModuleId_ = modId;
    model()->activeModule(Kontrol::CS_LOCAL, currentRackId_, currentModuleId_);
}


}
