#include "Rack.h"
#include "Module.h"
#include "KontrolModel.h"



#include <algorithm>
#include <limits>
#include <string.h>
#include <iostream>
#include <map>
#include <fstream>


// TODO
// preset, need to be stored per module
// midi cc... potentially mapped to muliple modules and multi parameters  e.g. cc73 : (module1 : t_mix, r_mix , module2:o_transpose)
// both these imply a certain json tree stucture, the current implmentatin below wont work ;)

// for saving presets only , later moved to Preferences
#include <cJSON.h>
#include <mec_log.h>
#include <mec_prefs.h>

namespace Kontrol {


inline std::shared_ptr<KontrolModel> Rack::model() {
    return KontrolModel::model();
}

void Rack::addModule(const std::shared_ptr<Module>& module) {
    if (module != nullptr) {
        modules_[module->id()] = module;
    }
}

std::vector<std::shared_ptr<Module>>  Rack::getModules() {
    std::vector<std::shared_ptr<Module>> ret;
    for (auto p : modules_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::shared_ptr<Module> Rack::getModule(const EntityId& moduleId) {
    return modules_[moduleId];
}


bool Rack::loadModuleDefinitions(const EntityId& moduleId, const mec::Preferences& prefs) {
    auto module  = getModule(moduleId);
    if (module != nullptr) {
        if (module->loadModuleDefinitions(prefs)) {
            publishMetaData(module);
            return true;
        }
    }
    return false;
}



bool Rack::loadSettings(const std::string& filename) {
    settings_ = std::make_shared<mec::Preferences>(filename);
    settingsFile_ = filename;
    return loadSettings(*settings_);
}


bool Rack::loadSettings(const mec::Preferences& prefs) {
    bool ret = false;
    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            mec::Preferences modpref(prefs.getSubTree(module->id()));
            if (modpref.valid()) {
                ret |= module->loadSettings(prefs);
                // publishCurrentValues(module);
            }
        }
    }
    return ret;
}


bool Rack::saveSettings() {
    // save to original module settings file
    // note: we do not save back to an preferences file, as this would not be complete
    if (!settingsFile_.empty()) {
        saveSettings(settingsFile_);
    }
    return false;
}

bool Rack::saveSettings(const std::string & filename) {
    // do in cJSON for now
    std::ofstream outfile(filename);
    cJSON *root = cJSON_CreateObject();
    for (auto m : modules_) {
        auto module = m.second;
        cJSON *mjson = cJSON_CreateObject();
        cJSON_AddItemToObject(root, module->id().c_str(), mjson);
        module->saveSettings(mjson);
    }

    // const char* text = cJSON_PrintUnformatted(root);
    const char* text = cJSON_Print(root);
    outfile << text << std::endl;
    outfile.close();
    cJSON_Delete(root);

    return true;
}


bool Rack::updatePreset(std::string presetId) {
    bool ret = false;
    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            ret |= module->updatePreset(presetId);
        }
    }
    return ret;
}

std::vector<std::string> Rack::getPresetList() {
    std::vector<std::string> presets;
    for (auto m : modules_) {
        auto module = m.second;
        if (module) {
            std::vector<std::string> mp = module->getPresetList();
            for (auto p : mp) {
                presets.push_back(p);
            }
        }
    }
    return presets;
}


bool Rack::applyPreset(std::string presetId) {
    bool ret = false;
    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            std::vector<Preset> presetvalues = module->getPreset(presetId);
            for (auto p : presetvalues) {
                auto param = module->getParam(p.paramId());
                if (param != nullptr) {
                    if (p.value().type() == ParamValue::T_Float) {
                        if (p.value() != param->current()) {
                            model()->changeParam(PS_PRESET, id(), module->id(), p.paramId(), p.value());
                            ret = true;
                        } //if chg
                    } //iffloat
                } // ifpara
            }
        }
    }
    return ret;
}

bool Rack::changeMidiCC(unsigned midiCC, unsigned midiValue) {
    bool ret = false;
    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            std::vector<EntityId>  mmvec = module->getParamsForCC(midiCC);
            for (auto paramId : mmvec) {
                auto param = module->getParam(paramId);
                if (param != nullptr) {
                    ParamValue pv = param->calcMidi(midiValue);
                    if (pv != param->current()) {
                        model()->changeParam(PS_MIDI, id(), module->id(), param->id(), pv);
                        ret = true;
                    }
                }
            }
        }
    }
    return ret;
}



void Rack::addMidiCCMapping(unsigned ccnum, const EntityId & moduleId, const EntityId & paramId) {
    auto module = getModule(moduleId);
    if (module != nullptr) module->addMidiCCMapping(ccnum, paramId);
}

void Rack::publishCurrentValues(const std::shared_ptr<Module>& module) const {
    if (module != nullptr) {
        std::vector<std::shared_ptr<Parameter>> params = module->getParams();
        std::vector<std::shared_ptr<Page>> pages = module->getPages();
        for (auto p : params) {
            model()->publishChanged(PS_LOCAL, *this, *module, *p);
        }
    }
}


void Rack::publishCurrentValues() const {
    for (auto p : modules_) {
        if (p.second != nullptr) publishCurrentValues(p.second);
    }
}


void Rack::publishMetaData(const std::shared_ptr<Module>& module) const {
    if (module != nullptr) {
        std::vector<std::shared_ptr<Parameter>> params = module->getParams();
        std::vector<std::shared_ptr<Page>> pages = module->getPages();
        for (auto p : params) {
            model()->publishParam(PS_LOCAL, *this, *module, *p);
        }
        for (auto p : pages) {
            if (p != nullptr) {
                model()->publishPage(PS_LOCAL, *this, *module,  *p);
            }
        }
    }
}

void Rack::publishMetaData() const {
    for (auto p : modules_) {
        if (p.second != nullptr) publishMetaData(p.second);
    }
}


void Rack::dumpSettings() const {
    LOG_1("Rack Settings Dump");
    LOG_1("-------------------");
    for(auto m : modules_) {
        if(m.second!=nullptr) m.second->dumpSettings();
    }
}

} //namespace