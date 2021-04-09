#include "mec_electraone.h"

#include <algorithm>
#include <unordered_set>
#include <unistd.h>

#include <mec_log.h>

#include "electraone/e1_basemode.h"
#include "electraone/e1_main.h"

namespace mec {


ElectraOne::ElectraOne(ICallback &cb) :
        callback_(cb),
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

    static const auto POLL_FREQ = 1;
    static const auto POLL_SLEEP = 1000;
    static const char *E1_Midi_Device_Ctrl = "Electra Controller Electra CTRL";
    static const char *E1_Midi_Device_P1 = "Electra Controller Electra Port 1";
    std::string electramidi = prefs.getString("midi device", E1_Midi_Device_Ctrl);
    std::string electramidip1 = prefs.getString("midi device p1", E1_Midi_Device_P1);

    device_ = std::make_shared<ElectraLite::ElectraDevice>(electramidi);
    mididevice_ = std::make_shared<ElectraLite::RtMidiDevice>();
    mididevice_->init(electramidip1.c_str(), electramidip1.c_str());

    pollFreq_ = prefs.getInt("poll freq", POLL_FREQ);
    pollSleep_ = prefs.getInt("poll sleep", POLL_SLEEP);
    pollCount_ = 0;
    if (!device_) return false;

    std::shared_ptr<ElectraLite::ElectraCallback> cb = std::make_shared<ElectraOneDeviceCallback>(*this);
    device_->addCallback(cb);
    midiCB_ = std::make_shared<ElectraOneMidiCallback>(*this);

    if (active_) {
        LOG_2("ElectraOne::init - already active deinit");
        deinit();
    }
    active_ = false;

    device_->start();

    active_ = true;
    if (active_) {
        addMode(E1_MAIN, std::make_shared<ElectraOneMainMode>(*this));
        changeMode(E1_MAIN);
    }
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
    if ((pollCount_ % pollFreq_) == 0) {
        if (mididevice_ && mididevice_->isActive()) {
            mididevice_->processIn(*midiCB_);
            mididevice_->processOut();
        }

        modes_[currentMode_]->poll();
        if (device_) device_->process();
    }

    if (pollSleep_) {
        usleep(pollSleep_);
    }
    return true;
}

void ElectraOne::stop() {
    deinit();
}


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


void ElectraOne::publishStart(Kontrol::ChangeSource src, unsigned numRacks) {
    modes_[currentMode_]->publishStart(src, numRacks);
}

void ElectraOne::publishRackFinished(Kontrol::ChangeSource src, const Kontrol::Rack &r) {
    modes_[currentMode_]->publishRackFinished(src, r);
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
//    std::cerr << "ElectraOne::onButton " << id << " " << value << std::endl;
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


static void removeNullsFromJson(nlohmann::json &json) {
    if (!json.is_object() && !json.is_array()) return;
    std::vector<nlohmann::json::object_t::key_type> keys;
    for (auto &it : json.items()) {
        if (it.value().is_null())
            keys.push_back(it.key());
        else
            removeNullsFromJson(it.value());
    }
    for (auto key : keys) json.erase(key);
}

void ElectraOne::send(ElectraOnePreset::Preset &preset) {
    nlohmann::json j;
    nlohmann::to_json(j, preset);
    removeNullsFromJson(j);

    nlohmann::ordered_json oj;
    static const std::string presetName = "name";
    auto i = j.find(presetName);
    if (i != j.end()) {
        oj[i.key()] = i.value();
    }

    for (auto &i : j.items()) {
        if (i.key() != presetName) {
            oj[i.key()] = i.value();
        }
    }

    if (device_) {
        std::string msg = j.dump();
        if (msg != lastMessageSent_) {
            std::cout << oj.dump() << std::endl;
//            std::cout << oj.dump(4) << std::endl;
            device_->uploadPreset(oj.dump());
        }
        lastMessageSent_ = msg;
    }
}


void ElectraOne::currentModule(const Kontrol::EntityId &modId) {
    currentModuleId_ = modId;
    model()->activeModule(Kontrol::CS_LOCAL, currentRackId_, currentModuleId_);
}


void ElectraOne::outputCC(unsigned cc, unsigned v) {
//    callback_.control(cc,v);
}

void ElectraOne::outputNoteOn(unsigned n, unsigned v) {
//    if(v==0) outputNoteOff(n,100);
//    callback_.touchOn(0,n,0,0,v);
}

void ElectraOne::outputNoteOff(unsigned n, unsigned v) {
//    callback_.touchOff(0,n,0,0,v);
}

}
