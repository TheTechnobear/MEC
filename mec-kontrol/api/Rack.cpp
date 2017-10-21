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


bool Rack::applyPreset(std::string presetId) {
    bool ret = false;
    currentPreset_ = presetId;
    auto presetvalues = presets_[presetId];
    for (auto presetvalue : presetvalues) {
        auto module = getModule(presetvalue.moduleId());
        if (module != nullptr) {
            auto param = module->getParam(presetvalue.paramId());
            if (param != nullptr) {
                if (presetvalue.value().type() == ParamValue::T_Float) {
                    if (presetvalue.value() != param->current()) {
                        model()->changeParam(PS_PRESET, id(), presetvalue.moduleId(), presetvalue.paramId(), presetvalue.value());
                        ret = true;
                    } //if chg
                } //iffloat
            } // ifpara
        } // ifmod
    }// presets
    return ret;
}

bool Rack::changeMidiCC(unsigned midiCC, unsigned midiValue) {
    auto mm = midi_mapping_[midiCC];
    if (mm == nullptr) return false;

    auto module = getModule(mm->moduleId());
    if (module != nullptr) {
        auto param = module->getParam(mm->paramId());
        if (param != nullptr) {
            ParamValue pv = param->calcMidi(midiValue);
            if (pv != param->current()) {
                model()->changeParam(PS_MIDI, id(), module->id(), param->id(), pv);
                return true;
            }
        }
    }
    return false;
}


bool Rack::loadRackSettings(const std::string& filename) {
    rackSettings_ = std::make_shared<mec::Preferences>(filename);
    rackSettingsFile_ = filename;
    return loadRackSettings(*rackSettings_);
}


bool Rack::loadRackSettings(const mec::Preferences& prefs) {
    mec::Preferences presets(prefs.getSubTree("presets"));
    if (presets.valid()) { // just ignore if not present

        for (std::string presetId : presets.getKeys()) {

            mec::Preferences modules(presets.getSubTree(presetId));
            for (EntityId moduleId : modules.getKeys() ) {
                mec::Preferences params(presets.getSubTree(moduleId));

                std::vector<Preset> preset;

                for (EntityId paramId : params.getKeys()) {
                    mec::Preferences::Type t = params.getType(paramId);
                    switch (t) {
                    case mec::Preferences::P_BOOL:   preset.push_back(Preset(moduleId, paramId, ParamValue(params.getBool(paramId) ? 1.0f : 0.0f ))); break;
                    case mec::Preferences::P_NUMBER: preset.push_back(Preset(moduleId, paramId, ParamValue((float) params.getDouble(paramId)))); break;
                    case mec::Preferences::P_STRING: preset.push_back(Preset(moduleId, paramId, ParamValue(params.getString(paramId)))); break;
                    //ignore
                    case mec::Preferences::P_NULL:
                    case mec::Preferences::P_ARRAY:
                    case mec::Preferences::P_OBJECT:
                    default:
                        break;
                    }
                } //for param
                presets_[presetId] = preset;
            } // module
        } // preset
    }

    mec::Preferences midimapping(prefs.getSubTree("midi-mapping"));
    if (midimapping.valid()) { // just ignore if not present
        for (EntityId moduleId : midimapping.getKeys()) {
            mec::Preferences module(midimapping.getSubTree(moduleId));
            if (module.valid()) {
                mec::Preferences cc(module.getSubTree("cc"));
                // only currently handling CC midi learn
                if (cc.valid()) {
                    for (std::string ccstr : cc.getKeys()) {
                        unsigned ccnum = std::stoi(ccstr);
                        EntityId paramId = cc.getString(ccstr);
                        addMidiCCMapping(ccnum, moduleId, paramId);
                    }
                }
            }
        }
    }
    return true;
}


bool Rack::saveRackSettings() {
    // save to original module settings file
    // note: we do not save back to an preferences file, as this would not be complete
    if (!rackSettingsFile_.empty()) {
        saveRackSettings(rackSettingsFile_);
    }
    return false;
}

bool Rack::saveRackSettings(const std::string& filename) {
    // do in cJSON for now
    std::ofstream outfile(filename);
    cJSON *root = cJSON_CreateObject();
    cJSON *presets = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "presets", presets);
    for (auto p : presets_) {
        cJSON* preset = cJSON_CreateObject();
        cJSON_AddItemToObject(presets, p.first.c_str(), preset);
        for (auto v : p.second) {
            switch (v.value().type()) {
            case ParamValue::T_String: {
                cJSON_AddStringToObject(preset, v.paramId().c_str(), v.value().stringValue().c_str());
                break;
            }
            case ParamValue::T_Float: {
                cJSON_AddNumberToObject(preset, v.paramId().c_str(), v.value().floatValue());
                break;
            }
            }//switch
        }
    }

    cJSON *midi = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "midi-mapping", midi);
    for (auto mm : midi_mapping_) {
        cJSON *module = cJSON_CreateObject();
        cJSON_AddItemToObject(midi, mm.second->moduleId().c_str(), module);
        cJSON *ccs = cJSON_CreateObject();
        cJSON_AddItemToObject(module, "cc", ccs);
        std::string ccstr = std::to_string(mm.first);
        cJSON_AddStringToObject(ccs, ccstr.c_str(), mm.second->paramId().c_str());
    }

    // const char* text = cJSON_PrintUnformatted(root);
    const char* text = cJSON_Print(root);
    outfile << text << std::endl;
    outfile.close();

    return true;
}


bool Rack::savePreset(std::string presetId) {
    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            std::vector<std::shared_ptr<Parameter>> params = module->getParams();
            currentPreset_ = presetId;
            std::vector<Preset> presets;
            for (auto p : params) {
                presets.push_back(Preset(module->id(), p->id(), p->current()));
            }
            presets_[presetId] = presets;
        }
    }
    return true;
}

std::vector<std::string> Rack::getPresetList() {
    std::vector<std::string> presets;
    for (auto p : presets_) {
        presets.push_back(p.first);
    }
    return presets;
}

void Rack::addMidiCCMapping(unsigned ccnum, const EntityId& moduleId, const EntityId& paramId) {
    midi_mapping_[ccnum] = std::make_shared<MidiMapping>(ccnum, moduleId, paramId);
}

void Rack::publishMetaData(const std::shared_ptr<Module>& module) const {
    // if (module != nullptr) {
    //     std::vector<std::shared_ptr<Parameter>> params = module->getParams();
    //     std::vector<std::shared_ptr<Page>> pages = module->getPages();
    //     for (auto p : params) {
    //         const Parameter& param = *(p.second);
    //         model()->param(PS_LOCAL, *this, *module, param);
    //         model()->changed(PS_LOCAL, *this, *module, param);
    //     }
    //     for (auto p : pages) {
    //         if (p != nullptr) {
    //             model()->page(PS_LOCAL, *this, *module,  *p);
    //         }
    //     }
    // }
}

void Rack::publishMetaData() const {
    for (auto p : modules_) {
        if (p.second != nullptr) publishMetaData(p.second);
    }
}


void Rack::dumpRackSettings() const {
    LOG_1("Rack Settings Dump");
    LOG_1("-------------------");
    LOG_1("Presets");
    for (auto preset : presets_) {
        LOG_1(preset.first);
        for (auto presetvalue : preset.second) {
            std::string d = presetvalue.moduleId();
            d += " : " + presetvalue.paramId();
            ParamValue pv = presetvalue.value();
            switch (pv.type()) {
            case ParamValue::T_Float :
                d += "  " + std::to_string(pv.floatValue()) + " [F],";
                break;
            case ParamValue::T_String :
            default:
                d += pv.stringValue() + " [S],";
                break;
            }
            LOG_1(d);
        }
    }
    LOG_1("Midi Mapping");
    for (auto cc : midi_mapping_) {
        LOG_1("CC : " <<  cc.first  << " to " << cc.second);
    }
}

} //namespace