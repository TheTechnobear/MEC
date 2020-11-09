#include "mec_electraone.h"

#include <algorithm>
#include <unordered_set>
#include <unistd.h>

#include <mec_log.h>

#include "electraone/e1_basemode.h"
#include "electraone/e1_menu.h"
#include "electraone/e1_param_1.h"
#include "electraone/e1_param_2.h"

namespace mec {

const unsigned SCREEN_HEIGHT = 64;
const unsigned SCREEN_WIDTH = 128;

static const unsigned E1_NUM_TEXTLINES = 5;
//static const unsigned E1_NUM_TEXTCHARS = (128 / 4); = 32
static const unsigned E1_NUM_TEXTCHARS = 30;

static const unsigned LINE_H=10;

ElectraOne::ElectraOne() :
        active_(false),
        modulationLearnActive_(false),
        midiLearnActive_(false) {
}

ElectraOne::~ElectraOne() {
    deinit();
}


//mec::Device

bool ElectraOne::init(void *arg) {
    Preferences prefs(arg);

    static const auto MENU_TIMEOUT = 2000;
    static const auto POLL_FREQ = 1;
    static const auto POLL_SLEEP = 1000;
    menuTimeout_ = prefs.getInt("menu timeout", MENU_TIMEOUT);
    std::string res = prefs.getString("resource path", "/home/we/norns/resources");
    std::string splash = prefs.getString("splash", "./oracsplash4.png");
    device_ = std::make_shared<ElectraOneLite::ElectraOneDevice>(res.c_str());
    pollFreq_ = prefs.getInt("poll freq",POLL_FREQ);
    pollSleep_ = prefs.getInt("poll sleep",POLL_SLEEP);
    pollCount_ = 0;
    if (!device_) return false;

    unsigned parammode = prefs.getInt("param display", 0);

    std::shared_ptr<ElectraOneLite::ElectraOneCallback> cb = std::make_shared<ElectraOneDeviceCallback>(*this);
    device_->addCallback(cb);
    device_->start();

    if (active_) {
        LOG_2("ElectraOne::init - already active deinit");
        deinit();
    }
    active_ = false;


    active_ = true;
    if (active_) {
        if (parammode == 1 || device_->numEncoders() == 3) {
            addMode(NM_PARAMETER, std::make_shared<ElectraOneParamMode2>(*this));

        } else {
            addMode(NM_PARAMETER, std::make_shared<ElectraOneParamMode1>(*this));
        }

        addMode(NM_MAINMENU, std::make_shared<ElectraOneMainMenu>(*this));
        addMode(NM_PRESETMENU, std::make_shared<ElectraOnePresetMenu>(*this));
        addMode(NM_MODULEMENU, std::make_shared<ElectraOneModuleMenu>(*this));
//        addMode(NM_MODULESELECTMENU, std::make_shared<ElectraOneModuleSelectMenu>(*this));

        changeMode(NM_PARAMETER);
    }
    // device_->drawPNG(0, 0, splash.c_str());
    // device_->displayText(15, 0, 1, "Connecting...");
    return active_;
}

void ElectraOne::deinit() {
    device_->stop();
    device_ = nullptr;
    active_ = false;
    return;
}

bool ElectraOne::isActive() {
    return active_;
}

// Kontrol::KontrolCallback
bool ElectraOne::process() {
    pollCount_++;
    if ( ( pollCount_ % pollFreq_) == 0) {
        modes_[currentMode_]->poll();
        if (device_) device_->process();
    }

    if(pollSleep_) {
        usleep(pollSleep_);
    }
    return true;
}

void ElectraOne::stop() {
    deinit();
}

//bool ElectraOne::connect(const std::string &hostname, unsigned port) {
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
//    changeMode(ElectraOneModes::NM_PARAMETER);
//    modes_[currentMode_]->activate();
//
//
//    return true;
//}


//--modes and forwarding
void ElectraOne::addMode(ElectraOneModes mode, std::shared_ptr<ElectraOneMode> m) {
    modes_[mode] = m;
}

void ElectraOne::changeMode(ElectraOneModes mode) {
    currentMode_ = mode;
    auto m = modes_[mode];
    m->activate();
}

void ElectraOne::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    modes_[currentMode_]->rack(src, rack);
}

void ElectraOne::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    if (currentModuleId_.empty()) {
        currentRackId_ = rack.id();
        currentModule(module.id());
    }

    modes_[currentMode_]->module(src, rack, module);
}

void ElectraOne::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
               const Kontrol::Module &module, const Kontrol::Page &page) {
    modes_[currentMode_]->page(src, rack, module, page);
}

void ElectraOne::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                const Kontrol::Module &module, const Kontrol::Parameter &param) {
    modes_[currentMode_]->param(src, rack, module, param);
}

void ElectraOne::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                  const Kontrol::Module &module, const Kontrol::Parameter &param) {
    modes_[currentMode_]->changed(src, rack, module, param);
}

void ElectraOne::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                   const std::string &res, const std::string &value) {
    modes_[currentMode_]->resource(src, rack, res, value);

    if (res == "moduleorder") {
        moduleOrder_.clear();
        if (value.length() > 0) {
            int lidx = 0;
            int idx = 0;
            int len = 0;
            while ((idx = value.find(" ", lidx)) != std::string::npos) {
                len = idx - lidx;
                std::string mid = value.substr(lidx, len);
                moduleOrder_.push_back(mid);
                lidx = idx + 1;
            }
            len = value.length() - lidx;
            if (len > 0) {
                std::string mid = value.substr(lidx, len);
                moduleOrder_.push_back(mid);
            }
        }
    }
}

void ElectraOne::deleteRack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    modes_[currentMode_]->deleteRack(src, rack);
}

void ElectraOne::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                       const Kontrol::Module &module) {
    modes_[currentMode_]->activeModule(src, rack, module);
}

void ElectraOne::loadModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                     const Kontrol::EntityId &modId, const std::string &modType) {
    modes_[currentMode_]->loadModule(src, rack, modId, modType);
}

std::vector<std::shared_ptr<Kontrol::Module>> ElectraOne::getModules(const std::shared_ptr<Kontrol::Rack> &pRack) {
    std::vector<std::shared_ptr<Kontrol::Module>> ret;
    auto modulelist = model()->getModules(pRack);
    std::unordered_set<std::string> done;

    for (auto mid : moduleOrder_) {
        auto pModule = model()->getModule(pRack, mid);
        if (pModule != nullptr) {
            ret.push_back(pModule);
            done.insert(mid);
        }

    }
    for (auto pModule : modulelist) {
        if (done.find(pModule->id()) == done.end()) {
            ret.push_back(pModule);
        }
    }

    return ret;
}


void ElectraOne::nextModule() {
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

void ElectraOne::prevModule() {
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

void ElectraOne::midiLearn(Kontrol::ChangeSource src, bool b) {
    if (b) modulationLearnActive_ = false;
    midiLearnActive_ = b;
    modes_[currentMode_]->midiLearn(src, b);

}

void ElectraOne::modulationLearn(Kontrol::ChangeSource src, bool b) {
    if (b) midiLearnActive_ = false;
    modulationLearnActive_ = b;
    modes_[currentMode_]->modulationLearn(src, b);
}

void ElectraOne::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    modes_[currentMode_]->savePreset(source, rack, preset);
}

void ElectraOne::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    modes_[currentMode_]->loadPreset(source, rack, preset);
}


void ElectraOne::onButton(unsigned id, unsigned value) {
    modes_[currentMode_]->onButton(id, value);

}

void ElectraOne::onEncoder(unsigned id, int value) {
    modes_[currentMode_]->onEncoder(id, value);
}


void ElectraOne::midiLearn(bool b) {
    model()->midiLearn(Kontrol::CS_LOCAL, b);
}

void ElectraOne::modulationLearn(bool b) {
    model()->modulationLearn(Kontrol::CS_LOCAL, b);
}


//--- display functions

void ElectraOne::displayPopup(const std::string &text, bool) {
    // not used currently
}


void ElectraOne::clearDisplay() {
    if (!device_) return;
    device_->displayClear();
}

void ElectraOne::clearParamNum(unsigned num) {
    if (!device_) return;

    unsigned row, col;
    switch (num) {
        case 0 :
            row = 0, col = 0;
            break;
        case 1 :
            row = 0, col = 1;
            break;
        case 2 :
            row = 1, col = 0;
            break;
        case 3 :
            row = 1, col = 1;
            break;
        default :
            return;
    }
    // unsigned x = col * 64;
    // unsigned y1 = (row + 1) * 20;
    // unsigned y2 = y1 + LINE_H;
    // device_->clearRect(0, x, y1 , 62 + (col * 2), LINE_H);
    // device_->clearRect(0, x, y2 , 62 + (col * 2), LINE_H);

}


void ElectraOne::displayParamNum(unsigned num, const Kontrol::Parameter &param, bool dispCtrl, bool selected) {
    if (!device_) return;
    const std::string &dName = param.displayName();
    std::string value = param.displayValue();
    std::string unit = param.displayUnit();

    unsigned row, col;
    switch (num) {
        case 0 :
            row = 0, col = 0;
            break;
        case 1 :
            row = 0, col = 1;
            break;
        case 2 :
            row = 1, col = 0;
            break;
        case 3 :
            row = 1, col = 1;
            break;
        default :
            return;
    }

    // unsigned x = col * 64;
    // unsigned y1 = (row + 1) * 20;
    // unsigned y2 = y1 + LINE_H;
    // unsigned clr = selected ? 15 : 0;
    // device_->clearRect(5, x, y1-LINE_H , 62 + (col * 2), LINE_H);
    // device_->drawText(clr, x + 1, y1 - 1, dName.c_str());
    // device_->clearRect(0, x, y2-LINE_H, 62 + (col * 2), LINE_H);
    // device_->drawText(15, x + 1, y2 - 1, value);
    // device_->drawText(15, x + 1 + 40, y2 - 1, unit);
}

void ElectraOne::displayLine(unsigned line, const char *disp) {
    if (!device_) return;
    device_->clearText(0, line);
    device_->displayText(15, line, 0, disp);
}

void ElectraOne::invertLine(unsigned line) {
    if (!device_) return;
    device_->invertText(line);
}

void ElectraOne::displayTitle(const std::string &module, const std::string &page) {
    if (!device_) return;
    if (module.size() == 0 || page.size() == 0) return;
    std::string title = module + " > " + page;

    // device_->clearRect(1, 0, 0, 128, LINE_H);
    // device_->drawText(15, 0, 8, title.c_str());
}


void ElectraOne::currentModule(const Kontrol::EntityId &modId) {
    currentModuleId_ = modId;
    model()->activeModule(Kontrol::CS_LOCAL, currentRackId_, currentModuleId_);
}


}
