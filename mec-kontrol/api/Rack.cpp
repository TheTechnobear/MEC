#include "Rack.h"
#include "Module.h"
#include "KontrolModel.h"


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


inline std::shared_ptr<KontrolModel> Rack::model() {
    return KontrolModel::model();
}

void Rack::addModule(const std::shared_ptr<Module> &module) {
    if (module != nullptr) {
        modules_[module->id()] = module;
    }
}

std::vector<std::shared_ptr<Module>> Rack::getModules() {
    std::vector<std::shared_ptr<Module>> ret;
    for (auto p : modules_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::shared_ptr<Module> Rack::getModule(const EntityId &moduleId) {
    return modules_[moduleId];
}


bool Rack::loadModuleDefinitions(const EntityId &moduleId, const mec::Preferences &prefs) {
    bool ret = false;
    auto module = getModule(moduleId);
    if (module != nullptr) {
        if (module->loadModuleDefinitions(prefs)) {
            publishMetaData(module);
            ret = true;
        }
    }
    return ret;
}


bool Rack::loadSettings(const std::string &filename) {
    settings_ = std::make_shared<mec::Preferences>(filename);
    settingsFile_ = filename;
    return loadSettings(*settings_);
}


bool Rack::saveSettings() {
    // save to original module settings file
    // note: we do not save back to an preferences file, as this would not be complete
    if (!settingsFile_.empty()) {
        saveSettings(settingsFile_);
    }
    return false;
}

bool Rack::loadSettings(const mec::Preferences &prefs) {
    bool ret = false;
    presets_.clear();
    mec::Preferences presetspref(prefs.getSubTree("presets"));
    if (presetspref.valid()) {
        for (std::string presetId :presetspref.getKeys()) {
            mec::Preferences rackpresetspref(presetspref.getSubTree(presetId));
            if (rackpresetspref.valid()) {
                RackPreset rackPreset;
                for (std::string moduleId :rackpresetspref.getKeys()) {
                    mec::Preferences modulepresetspref(rackpresetspref.getSubTree(moduleId));
                    if (modulepresetspref.valid()) {
                        ret |= loadModulePreset(rackPreset, moduleId, modulepresetspref);
                    }
                }
                presets_[presetId] = rackPreset;
            }
        }
    }

    return ret;
}


bool Rack::saveSettings(const std::string &filename) {
    // do in cJSON for now
    std::ofstream outfile(filename);
    cJSON *root = cJSON_CreateObject();
    cJSON *presets = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "presets", presets);

    for (auto rackPreset : presets_) {

        cJSON *preset = cJSON_CreateObject();
        cJSON_AddItemToObject(presets, rackPreset.first.c_str(), preset);


        for (auto mp : rackPreset.second) {

            auto moduleId = mp.first;
            auto modulePreset = mp.second;
            cJSON *mjson = cJSON_CreateObject();
            cJSON_AddItemToObject(preset, moduleId.c_str(), mjson);
            saveModulePreset(modulePreset, mjson);
        }
    }

    // const char* text = cJSON_PrintUnformatted(root);
    const char *text = cJSON_Print(root);
    outfile << text << std::endl;
    outfile.close();
    cJSON_Delete(root);

    return true;
}

std::vector<std::string> Rack::getPresetList() {
    std::vector<std::string> presets;
    for (auto p : presets_) {
        presets.push_back(p.first);
    }
    return presets;
}


bool Rack::updatePreset(std::string presetId) {
    bool ret = false;
    RackPreset rackPreset = presets_[presetId];

    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            auto moduleId = module->id();
            ModulePreset modulePreset = rackPreset[moduleId];
            ret |= updateModulePreset(module, modulePreset);
            rackPreset[moduleId] = modulePreset;
        }
    }
    presets_[presetId] = rackPreset;
    currentPreset_ = presetId;

    dumpSettings();

    return ret;
}

bool Rack::applyPreset(std::string presetId) {
    bool ret = false;
    if (presets_.count(presetId) == 0) return false;
    RackPreset rackPreset = presets_[presetId];

    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            auto moduleId = module->id();
            if (rackPreset.count(moduleId) > 0) {
                ModulePreset modulePreset = rackPreset[moduleId];
                if (module->type() != modulePreset.moduleType()) {
                    model()->loadModule(CS_PRESET, id(), module->id(), modulePreset.moduleType());
                    module = getModule(moduleId);
                }

                ret |= applyModulePreset(module, modulePreset);
            }
        }
    }
    currentPreset_ = presetId;
    return ret;
}

bool Rack::changeMidiCC(unsigned midiCC, unsigned midiValue) {
    bool ret = false;
    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            std::vector<EntityId> mmvec = module->getParamsForCC(midiCC);
            for (auto paramId : mmvec) {
                auto param = module->getParam(paramId);
                if (param != nullptr) {
                    ParamValue pv = param->calcMidi(midiValue);
                    if (pv != param->current()) {
                        model()->changeParam(CS_MIDI, id(), module->id(), param->id(), pv);
                        ret = true;
                    }
                }
            }
        }
    }
    return ret;
}

void Rack::addMidiCCMapping(unsigned ccnum, const EntityId &moduleId, const EntityId &paramId) {
    auto module = getModule(moduleId);
    if (module != nullptr) module->addMidiCCMapping(ccnum, paramId);
}

void Rack::removeMidiCCMapping(unsigned ccnum, const EntityId &moduleId, const EntityId &paramId) {
    auto module = getModule(moduleId);
    if (module != nullptr) module->removeMidiCCMapping(ccnum, paramId);
}


bool Rack::changeModulation(unsigned bus, float value) {
    bool ret = false;
    for (auto m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            std::vector<EntityId> mmvec = module->getParamsForModulation(bus);
            for (auto paramId : mmvec) {
                auto param = module->getParam(paramId);
                if (param != nullptr) {
                    ParamValue pv = param->calcFloat(value);
                    if (pv != param->current()) {
                        model()->changeParam(CS_MODULATION, id(), module->id(), param->id(), pv);
                        ret = true;
                    }
                }
            }
        }
    }
    return ret;
}

void Rack::addModulationMapping(unsigned bus, const EntityId &moduleId, const EntityId &paramId) {
    auto module = getModule(moduleId);
    if (module != nullptr) module->addModulationMapping(bus, paramId);
}

void Rack::removeModulationMapping(unsigned bus, const EntityId &moduleId, const EntityId &paramId) {
    auto module = getModule(moduleId);
    if (module != nullptr) module->removeModulationMapping(bus, paramId);
}



void Rack::publishCurrentValues(const std::shared_ptr<Module> &module) const {
    if (module != nullptr) {
        std::vector<std::shared_ptr<Parameter>> params = module->getParams();
        std::vector<std::shared_ptr<Page>> pages = module->getPages();
        for (auto p : params) {
            model()->publishChanged(CS_LOCAL, *this, *module, *p);
        }
    }
}


void Rack::publishCurrentValues() const {
    for (auto p : modules_) {
        if (p.second != nullptr) publishCurrentValues(p.second);
    }
}


void Rack::publishMetaData(const std::shared_ptr<Module> &module) const {
    if (module != nullptr) {
        model()->publishModule(CS_LOCAL, *this, *module);
        std::vector<std::shared_ptr<Parameter>> params = module->getParams();
        std::vector<std::shared_ptr<Page>> pages = module->getPages();
        for (auto p : params) {
            model()->publishParam(CS_LOCAL, *this, *module, *p);
        }
        for (auto p : pages) {
            if (p != nullptr) {
                model()->publishPage(CS_LOCAL, *this, *module, *p);
            }
        }
    }
    for (auto rt : resources_) {
        for (auto res : rt.second) {
            model()->publishResource(CS_LOCAL, *this, rt.first, res);
        }
    }
}

std::set<std::string> Rack::getResourceTypes() {
    std::set<std::string> resTypes;
    for (auto r : resources_) {
        resTypes.insert(r.first);
    }
    return resTypes;
}


void Rack::addResource(const std::string &type, const std::string &resource) {
    resources_[type].insert(resource);
}


const std::set<std::string> &Rack::getResources(const std::string &type) {
    return resources_[type];
}


void Rack::publishMetaData() const {
    for (auto p : modules_) {
        if (p.second != nullptr) publishMetaData(p.second);
    }
}


bool Rack::loadModulePreset(RackPreset &rackPreset, const EntityId &moduleId, const mec::Preferences &prefs) {
    std::string moduleType = prefs.getString("moduleType");

    mec::Preferences params(prefs.getSubTree("params"));
    std::vector<ModulePresetValue> presetValues;
    if (params.valid()) { // just ignore if not present
        for (EntityId paramId : params.getKeys()) {
            mec::Preferences::Type t = params.getType(paramId);
            switch (t) {
                case mec::Preferences::P_BOOL:
                    presetValues.push_back(
                            ModulePresetValue(paramId, ParamValue(params.getBool(paramId) ? 1.0f : 0.0f)));
                    break;
                case mec::Preferences::P_NUMBER:
                    presetValues.push_back(ModulePresetValue(paramId, ParamValue((float) params.getDouble(paramId))));
                    break;
                case mec::Preferences::P_STRING:
                    presetValues.push_back(ModulePresetValue(paramId, ParamValue(params.getString(paramId))));
                    break;
                    //ignore
                case mec::Preferences::P_NULL:
                case mec::Preferences::P_ARRAY:
                case mec::Preferences::P_OBJECT:
                default:
                    break;
            }
        } //for param
    }

    mec::Preferences midimapping(prefs.getSubTree("midi-mapping"));
    MidiMap midimap;
    if (midimapping.valid()) { // just ignore if not present
        mec::Preferences cc(midimapping.getSubTree("cc"));
        // only currently handling CC midi learn
        if (cc.valid()) {
            for (std::string ccstr : cc.getKeys()) {
                unsigned ccnum = std::stoi(ccstr);
                mec::Preferences::Array array = cc.getArray(ccstr);
                if (array.valid()) {
                    for (int i = 0; i < array.getSize(); i++) {
                        EntityId paramId = array.getString(i);
                        midimap[ccnum].push_back(paramId);
                    }
                }
            }
        }
    }

    mec::Preferences modmapping(prefs.getSubTree("mod-mapping"));
    ModulationMap modmap;
    if (modmapping.valid()) { // just ignore if not present
        mec::Preferences bus(modmapping.getSubTree("bus"));
        if (bus.valid()) {
            for (std::string busstr : bus.getKeys()) {
                unsigned busnum = std::stoi(busstr);
                mec::Preferences::Array array = bus.getArray(busstr);
                if (array.valid()) {
                    for (int i = 0; i < array.getSize(); i++) {
                        EntityId paramId = array.getString(i);
                        modmap[busnum].push_back(paramId);
                    }
                }
            }
        }
    }


    rackPreset[moduleId] = ModulePreset(moduleType, presetValues, midimap,modmap);

    return true;
}

bool Rack::saveModulePreset(ModulePreset &modulepreset, cJSON *root) {

    cJSON_AddStringToObject(root, "moduleType", modulepreset.moduleType().c_str());

    cJSON *presetValues = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "params", presetValues);

    for (auto v : modulepreset.values()) {
        switch (v.value().type()) {
            case ParamValue::T_String: {
                cJSON_AddStringToObject(presetValues, v.paramId().c_str(), v.value().stringValue().c_str());
                break;
            }
            case ParamValue::T_Float: {
                cJSON_AddNumberToObject(presetValues, v.paramId().c_str(), v.value().floatValue());
                break;
            }
        }//switch
    }

    // midi mapping
    cJSON *midi = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "midi-mapping", midi);
    cJSON *ccs = cJSON_CreateObject();
    cJSON_AddItemToObject(midi, "cc", ccs);
    for (auto mm : modulepreset.midiMap()) {
        if (mm.second.size() > 0) {
            std::string ccnum = std::to_string(mm.first);
            cJSON *array = cJSON_CreateArray();
            cJSON_AddItemToObject(ccs, ccnum.c_str(), array);
            for (auto paramId : mm.second) {
                cJSON *itm = cJSON_CreateString(paramId.c_str());
                cJSON_AddItemToArray(array, itm);
            }
        }
    }


    // modulation mapping
    cJSON *modulation = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "mod-mapping", modulation);
    cJSON *buss = cJSON_CreateObject();
    cJSON_AddItemToObject(modulation, "bus", buss);
    for (auto mm : modulepreset.modulationMap()) {
        if (mm.second.size() > 0) {
            std::string busnum = std::to_string(mm.first);
            cJSON *array = cJSON_CreateArray();
            cJSON_AddItemToObject(buss, busnum.c_str(), array);
            for (auto paramId : mm.second) {
                cJSON *itm = cJSON_CreateString(paramId.c_str());
                cJSON_AddItemToArray(array, itm);
            }
        }
    }

    return true;
}

bool Rack::updateModulePreset(std::shared_ptr<Module> module, ModulePreset &modulePreset) {
    bool ret = true;
    module->dumpCurrentValues();
    std::vector<std::shared_ptr<Parameter>> params = module->getParams();
    std::vector<ModulePresetValue> presetValues;
    for (auto p : params) {
        presetValues.push_back(ModulePresetValue(p->id(), p->current()));
    }

    modulePreset = ModulePreset(module->type(), presetValues, module->getMidiMapping(), module->getModulationMapping());
    return ret;
}

bool Rack::applyModulePreset(std::shared_ptr<Module> module, const ModulePreset &modulePreset) {
    bool ret = false;


    // restore parameter values
    for (auto p : modulePreset.values()) {
        if (p.value().type() == ParamValue::T_Float) {
            model()->changeParam(CS_PRESET, model()->localRack()->id(), module->id(), p.paramId(), p.value());
            ret |= true;
        } //iffloat
        //TODO: preset, support non numeric types
    }

    module->setMidiMapping(modulePreset.midiMap());

    return ret;
}


void Rack::dumpSettings() const {
    LOG_1("Rack Settings :" << id());
    LOG_1("------------------------");
    for (auto preset : presets_) {
        LOG_1("Preset : " << preset.first);
        for (auto rpreset: preset.second) {
            LOG_1("   Module : " << rpreset.first);
            for (auto param : rpreset.second.values()) {
                LOG_1("       " << param.paramId() << " : " << param.value().floatValue());
            }
        }
    }
}

void Rack::dumpParameters() {
    LOG_1("Rack Parameters :" << id());
    LOG_1("------------------------");
    for (auto m : modules_) {
        if (m.second != nullptr) m.second->dumpParameters();
    }
}

void Rack::dumpCurrentValues() {
    LOG_1("Rack Values : " << id());
    LOG_1("-----------------------");
    for (auto m : modules_) {
        if (m.second != nullptr) m.second->dumpCurrentValues();
    }
}


} //namespace
