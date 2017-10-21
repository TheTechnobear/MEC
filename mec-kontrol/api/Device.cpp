#include "Device.h"
#include "Patch.h"
#include "ParameterModel.h"



#include <algorithm>
#include <limits>
#include <string.h>
#include <iostream>
#include <map>
#include <fstream>


// for saving presets only , later moved to Preferences
#include <cJSON.h>
#include <mec_log.h>
#include <mec_prefs.h>

namespace Kontrol {


inline std::shared_ptr<ParameterModel> Device::model() {
    return ParameterModel::model();
}

void Device::addPatch(const std::shared_ptr<Patch>& patch) {
    if (patch != nullptr) {
        patches_[patch->id()] = patch;
    }
}

std::vector<std::shared_ptr<Patch>>  Device::getPatches() {
    std::vector<std::shared_ptr<Patch>> ret;
    for (auto p : patches_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::shared_ptr<Patch> Device::getPatch(const EntityId& patchId) {
    return patches_[patchId];
}

bool Device::loadParameterDefinitions(const EntityId& patchId, const std::string& filename) {
    paramDefinitions_ = std::make_shared<mec::Preferences>(filename);
    return loadParameterDefinitions(patchId, *paramDefinitions_);
}


bool Device::loadParameterDefinitions(const EntityId& patchId, const mec::Preferences& prefs) {
    auto patch  = getPatch(patchId);
    if (patch != nullptr) {
        if (patch->loadParameterDefinitions(prefs)) {
            publishMetaData(patch);
            return true;
        }
    }
    return false;
}


bool Device::applyPreset(std::string presetId) {
    bool ret = false;
    EntityId patchId; // TODO
    auto patch = getPatch(patchId);
    if (patch != nullptr) {
        currentPreset_ = presetId;
        auto presetvalues = presets_[presetId];
        for (auto presetvalue : presetvalues) {
            auto param = patch->getParam(presetvalue.paramId());
            if (param != nullptr) {
                if (presetvalue.value().type() == ParamValue::T_Float) {
                    if (presetvalue.value() != param->current()) {
                        model()->changeParam(PS_PRESET, id(), patchId, presetvalue.paramId(), presetvalue.value());
                        ret = true;
                    }
                }
            }
        }
    }
    return ret;
}

bool Device::changeMidiCC(unsigned midiCC, unsigned midiValue) {
    EntityId patchId; //TODO
    auto patch = getPatch(patchId);
    if (patch != nullptr) {
        auto paramId = midi_mapping_[midiCC];
        if (!paramId.empty()) {
            auto param = patch->getParam(paramId);
            if (param != nullptr) {
                ParamValue pv = param->calcMidi(midiValue);
                if (pv != param->current()) {
                    model()->changeParam(PS_MIDI, id(), patchId, paramId, pv);
                    return true;
                }
            }
        }
    }

    return false;
}


bool Device::loadPatchSettings(const std::string& filename) {
    patchSettings_ = std::make_shared<mec::Preferences>(filename);
    patchSettingsFile_ = filename;
    return loadPatchSettings(*patchSettings_);
}


bool Device::loadPatchSettings(const mec::Preferences& prefs) {
    mec::Preferences presets(prefs.getSubTree("presets"));
    if (presets.valid()) { // just ignore if not present

        for (std::string presetId : presets.getKeys()) {

            mec::Preferences params(presets.getSubTree(presetId));
            std::vector<Preset> preset;

            for (EntityId paramID : params.getKeys()) {
                mec::Preferences::Type t = params.getType(paramID);
                switch (t) {
                case mec::Preferences::P_BOOL:   preset.push_back(Preset(paramID, ParamValue(params.getBool(paramID) ? 1.0f : 0.0f ))); break;
                case mec::Preferences::P_NUMBER: preset.push_back(Preset(paramID, ParamValue((float) params.getDouble(paramID)))); break;
                case mec::Preferences::P_STRING: preset.push_back(Preset(paramID, ParamValue(params.getString(paramID)))); break;
                //ignore
                case mec::Preferences::P_NULL:
                case mec::Preferences::P_ARRAY:
                case mec::Preferences::P_OBJECT:
                default:
                    break;
                }
            } //for param
            presets_[presetId] = preset;
        }
    }

    mec::Preferences midimapping(prefs.getSubTree("midi-mapping"));
    if (midimapping.valid()) { // just ignore if not present
        mec::Preferences cc(midimapping.getSubTree("cc"));
        // only currently handling CC midi learn
        if (cc.valid()) {
            for (std::string ccstr : cc.getKeys()) {
                unsigned ccnum = std::stoi(ccstr);
                EntityId patchId; // TODO
                EntityId paramId = cc.getString(ccstr);
                addMidiCCMapping(ccnum, patchId, paramId);
            }
        }
    }
    return true;
}


bool Device::savePatchSettings() {
    // save to original patch settings file
    // note: we do not save back to an preferences file, as this would not be complete
    if (!patchSettingsFile_.empty()) {
        savePatchSettings(patchSettingsFile_);
    }
    return false;
}

bool Device::savePatchSettings(const std::string& filename) {
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
    cJSON *ccs = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "midi-mapping", midi);
    cJSON_AddItemToObject(midi, "cc", ccs);
    for (auto c : midi_mapping_) {
        std::string ccstr = std::to_string(c.first);
        cJSON_AddStringToObject(ccs, ccstr.c_str(), c.second.c_str());
    }

    // const char* text = cJSON_PrintUnformatted(root);
    const char* text = cJSON_Print(root);
    outfile << text << std::endl;
    outfile.close();

    return true;
}


bool Device::savePreset(std::string presetId) {
    EntityId patchId; //TODO
    auto patch = getPatch(patchId);
    if (patch != nullptr) {
        std::vector<std::shared_ptr<Parameter>> params = patch->getParams();
        currentPreset_ = presetId;
        std::vector<Preset> presets;
        for (auto p : params) {
            presets.push_back(Preset(p->id(), p->current()));
        }
        presets_[presetId] = presets;
    }
    return true;
}

std::vector<std::string> Device::getPresetList() {
    std::vector<std::string> presets;
    for (auto p : presets_) {
        presets.push_back(p.first);
    }
    return presets;
}

void Device::addMidiCCMapping(unsigned ccnum, const EntityId& patchId, const EntityId& paramId) {
    midi_mapping_[ccnum] = paramId;
}

void Device::publishMetaData(const std::shared_ptr<Patch>& patch) const {
    // if (patch != nullptr) {
    //     std::vector<std::shared_ptr<Parameter>> params = patch->getParams();
    //     std::vector<std::shared_ptr<Page>> pages = patch->getPages();
    //     for (auto p : params) {
    //         const Parameter& param = *(p.second);
    //         model()->param(PS_LOCAL, *this, *patch, param);
    //         model()->changed(PS_LOCAL, *this, *patch, param);
    //     }
    //     for (auto p : pages) {
    //         if (p != nullptr) {
    //             model()->page(PS_LOCAL, *this, *patch,  *p);
    //         }
    //     }
    // }
}

void Device::publishMetaData() const {
    for (auto p : patches_) {
        if (p.second != nullptr) publishMetaData(p.second);
    }
}


void Device::dumpPatchSettings() const {
    LOG_1("Patch Settings Dump");
    LOG_1("-------------------");
    LOG_1("Presets");
    for (auto preset : presets_) {
        LOG_1(preset.first);
        for (auto presetvalue : preset.second) {
            std::string d = presetvalue.paramId();
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