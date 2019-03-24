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
#ifndef _WIN32

#   include <dirent.h>

#else
#   include "dirent_win.h"
#endif

#include <sys/stat.h>

#include <clocale>
#include <cJSON.h>
#include <mec_log.h>
#include <mec_prefs.h>

namespace Kontrol {


void  Rack::init() {
    mec::Preferences prefs("orac.json");
    if(prefs.valid()) {
        dataDir_ = prefs.getString("dataDir", dataDir_);
        mediaDir_ = prefs.getString("mediaDir", mediaDir_);
        moduleDir_ = prefs.getString("moduleDir", moduleDir_);
        userModuleDir_ = prefs.getString("userModuleDir", userModuleDir_);
    }
}



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
    for (const auto &p : modules_) {
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




bool Rack::saveSettings() {
    // save to original module settings file
    // note: we do not save back to an preferences file, as this would not be complete
    if (!settingsFile_.empty()) {
        saveSettings(settingsFile_);
    }
    return false;
}

bool Rack::loadSettings(const std::string &filename) {
    bool ret = false;
    settingsFile_ = filename;

    // load rack preferences
    std::string rackPrefFile = dataDir_+ "/" + filename;
    mec::Preferences rackPrefs(rackPrefFile);
    if(rackPrefs.valid()) {
        currentPreset_ = rackPrefs.getString("currentPreset", currentPreset_);
    }

    // load presets
    presets_.clear();
    std::string presetsdir = dataDir_ + "/presets";
    std::setlocale(LC_ALL, "en_US.UTF-8");

    struct stat st;
    struct dirent **namelist;

    int n = scandir(presetsdir.c_str(), &namelist, NULL, alphasort);
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            if (namelist[i]->d_type == DT_DIR &&
                strcmp(namelist[i]->d_name, "..") != 0
                && strcmp(namelist[i]->d_name, ".") != 0) {
                std::string presetId = namelist[i]->d_name;
                std::string paramfile = presetsdir + "/" + presetId+ "/params.json";
                int fs = stat(paramfile.c_str(), &st);
                if(fs == 0) {
                    presets_.push_back(presetId);
                    addResource("preset",presetId);
                }
            }
        }
    }

    if(currentPreset().length()>0) loadFilePreset(currentPreset_);

    model()->publishMetaData();
    return ret;
}


bool Rack::saveSettings(const std::string &filename) {
    {
        std::string rackPrefFile = dataDir_+ "/" + filename + ".json";
        std::ofstream outfile(rackPrefFile.c_str());
        cJSON *root = cJSON_CreateObject();

        cJSON_AddStringToObject(root, "currentPreset", currentPreset_.c_str());
        const char *text = cJSON_Print(root);
        outfile << text << std::endl;
        outfile.close();
        cJSON_Delete(root);
    }
    return true;
}

std::vector<std::string> Rack::getPresetList() {
    std::vector<std::string> presets;
    for (const auto &p : presets_) {
        presets.push_back(p);
    }
    return presets;
}


bool Rack::savePreset(std::string presetId) {
    bool ret = false;

    // store preset in rackPreset_
    for (const auto &m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            auto moduleId = module->id();
            ModulePreset modulePreset = rackPreset_[moduleId];
            ret |= updateModulePreset(module, modulePreset);
            rackPreset_[moduleId] = modulePreset;
        }
    }

    addResource("preset",presetId);
    model()->publishResource(CS_LOCAL,*this,"preset",presetId);

    //save rackPreset_ to file
    saveFilePreset(presetId);

    currentPreset_ = presetId;
    model()->savePreset(CS_LOCAL, id(), currentPreset());

    //    dumpSettings();
    return ret;
}

bool Rack::loadPreset(std::string presetId) {
    bool ret = loadFilePreset(presetId);
    return ret;
}

bool Rack::loadFilePreset(const std::string& presetId) {
    bool ret = false;

    std::string dir = dataDir_ + "/presets/"+presetId;
    std::string filename = dir+"/params.json";

    rackPreset_.clear();
    mec::Preferences preset(filename);
    if(!preset.valid()) return false;

    // load preset from file
    for (const std::string &moduleId :preset.getKeys()) {
        mec::Preferences modulepresetspref(preset.getSubTree(moduleId));
        if (modulepresetspref.valid()) {
            ret |= loadModulePreset(rackPreset_, moduleId, modulepresetspref);
        }
    }

    // publish it
    for (const auto &m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            auto moduleId = module->id();
            if (rackPreset_.count(moduleId) > 0) {
                ModulePreset modulePreset = rackPreset_[moduleId];
                if (module->type() != modulePreset.moduleType()) {
                    model()->loadModule(CS_PRESET, id(), module->id(), modulePreset.moduleType());
                    module = getModule(moduleId);
                }

                ret |= applyModulePreset(module, modulePreset);
            }
        }
    }
    currentPreset_ = presetId;
    model()->loadPreset(CS_LOCAL, id(), currentPreset());
    return ret;
}

bool Rack::saveFilePreset(const std::string& presetId) {
    bool ret = true;

    std::string dir = dataDir_ + "/presets/"+presetId;
    std::string filename = dir+"/params.json";
    mkdir(dir.c_str(),S_IRWXU|S_IRWXG|S_IRWXO);

    std::ofstream outfile(filename.c_str());
    cJSON *root = cJSON_CreateObject();

    for (const auto &mp : rackPreset_) {

        auto moduleId = mp.first;
        auto modulePreset = mp.second;
        cJSON *mjson = cJSON_CreateObject();
        cJSON_AddItemToObject(root, moduleId.c_str(), mjson);
        saveModulePreset(modulePreset, mjson);
    }

    // const char* text = cJSON_PrintUnformatted(root);
    const char *text = cJSON_Print(root);
    outfile << text << std::endl;
    outfile.close();
    cJSON_Delete(root);
    return ret;
}


bool Rack::changeMidiCC(unsigned midiCC, unsigned midiValue) {
    bool ret = false;
    for (const auto &m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            std::vector<EntityId> mmvec = module->getParamsForCC(midiCC);
            for (const auto &paramId : mmvec) {
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
    for (const auto &m : modules_) {
        auto module = m.second;
        if (module != nullptr) {
            std::vector<EntityId> mmvec = module->getParamsForModulation(bus);
            for (const auto &paramId : mmvec) {
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

void Rack::addModulationMapping(const std::string &src, unsigned bus, const EntityId &moduleId, const EntityId &paramId) {
    auto module = getModule(moduleId);
    if (module != nullptr) module->addModulationMapping(src, bus, paramId);
}

void Rack::removeModulationMapping(const std::string &src, unsigned bus, const EntityId &moduleId, const EntityId &paramId) {
    auto module = getModule(moduleId);
    if (module != nullptr) module->removeModulationMapping(src, bus, paramId);
}



void Rack::publishCurrentValues(const std::shared_ptr<Module> &module) const {
    if (module != nullptr) {
        std::vector<std::shared_ptr<Parameter>> params = module->getParams();
        std::vector<std::shared_ptr<Page>> pages = module->getPages();
        for (const auto &p : params) {
            model()->publishChanged(CS_LOCAL, *this, *module, *p);
        }
    }
}


void Rack::publishCurrentValues() const {
    for (const auto &p : modules_) {
        if (p.second != nullptr) publishCurrentValues(p.second);
    }
}


void Rack::publishMetaData(const std::shared_ptr<Module> &module) const {
    if (module != nullptr) {
        model()->publishModule(CS_LOCAL, *this, *module);
        std::vector<std::shared_ptr<Parameter>> params = module->getParams();
        std::vector<std::shared_ptr<Page>> pages = module->getPages();
        for (const auto &p : params) {
            model()->publishParam(CS_LOCAL, *this, *module, *p);
        }
        for (const auto &p : pages) {
            if (p != nullptr) {
                model()->publishPage(CS_LOCAL, *this, *module, *p);
            }
        }
    }
    for (const auto &rt : resources_) {
        for (const auto &res : rt.second) {
            model()->publishResource(CS_LOCAL, *this, rt.first, res);
        }
    }
}

std::set<std::string> Rack::getResourceTypes() {
    std::set<std::string> resTypes;
    for (const auto &r : resources_) {
        resTypes.insert(r.first);
    }
    return resTypes;
}


void Rack::addResource(const std::string &type, const std::string &resource) {
    resources_[type].insert(resource);
//    model()->publishResource(CS_LOCAL,*this,type,resource);
}


const std::set<std::string> &Rack::getResources(const std::string &type) {
    return resources_[type];
}


void Rack::publishMetaData() const {
    for (const auto &p : modules_) {
        if (p.second != nullptr) publishMetaData(p.second);
    }
}


bool Rack::loadModulePreset(RackPreset &rackPreset, const EntityId &moduleId, const mec::Preferences &prefs) {
    std::string moduleType = prefs.getString("moduleType");

    mec::Preferences params(prefs.getSubTree("params"));
    std::vector<ModulePresetValue> presetValues;
    if (params.valid()) { // just ignore if not present
        for (const EntityId &paramId : params.getKeys()) {
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
            for (const std::string &ccstr : cc.getKeys()) {
                int ccnum = std::stoi(ccstr);
                mec::Preferences::Array array = cc.getArray(ccstr);
                if (array.valid()) {
                    for (unsigned int i = 0; i < array.getSize(); i++) {
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
            for (const std::string &busstr : bus.getKeys()) {
                int busnum = std::stoi(busstr);
                mec::Preferences::Array array = bus.getArray(busstr);
                if (array.valid()) {
                    for (unsigned int i = 0; i < array.getSize(); i++) {
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
    for (const auto &mm : modulepreset.midiMap()) {
        if (!mm.second.empty()) {
            std::string ccnum = std::to_string(mm.first);
            cJSON *array = cJSON_CreateArray();
            cJSON_AddItemToObject(ccs, ccnum.c_str(), array);
            for (const auto &paramId : mm.second) {
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
    for (const auto &mm : modulepreset.modulationMap()) {
        if (!mm.second.empty()) {
            std::string busnum = std::to_string(mm.first);
            cJSON *array = cJSON_CreateArray();
            cJSON_AddItemToObject(buss, busnum.c_str(), array);
            for (const auto &paramId : mm.second) {
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
    for (const auto &p : params) {
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
    for(const auto &param: module->getParams()) {
        if (param->current().type() ==  ParamValue::T_Float) {
            model()->changeParam(CS_PRESET, model()->localRack()->id(), module->id(), param->id(),
                                 param->current().floatValue());
        }
    }

    module->setMidiMapping(modulePreset.midiMap());
    module->setModulationMapping(modulePreset.modulationMap());

    return ret;
}


void Rack::dumpSettings() const {
    LOG_1("Rack Settings :" << id());
    LOG_1("------------------------");
    LOG_1("dataDir : "  << dataDir());
    LOG_1("mediaDir : " << mediaDir());
    LOG_1("moduleDir : "  << moduleDir());
    LOG_1("userModuleDir : "  << userModuleDir());
    LOG_1("currentPreset : "  << currentPreset());
    for (const auto &preset : presets_) {
        LOG_1("Preset : " << preset);
    }
}

void Rack::dumpParameters() {
    LOG_1("Rack Parameters :" << id());
    LOG_1("------------------------");
    for (const auto &m : modules_) {
        if (m.second != nullptr) m.second->dumpParameters();
    }
}

void Rack::dumpCurrentValues() {
    LOG_1("Rack Values : " << id());
    LOG_1("-----------------------");
    for (const auto &m : modules_) {
        if (m.second != nullptr) m.second->dumpCurrentValues();
    }
}


} //namespace
