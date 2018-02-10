#include "Module.h"


#include <algorithm>
#include <limits>
#include <string.h>
#include <iostream>
#include <map>
#include <fstream>


#include <mec_prefs.h>
#include <mec_log.h>

// for saving presets only , later moved to Preferences
#include <cJSON.h>

namespace Kontrol {


#if 0

std::string Module::getParamId(const EntityId& pageId, unsigned paramNum) {
    auto page = pages_[pageId];
    if (page != nullptr && paramNum < page->paramIds().size()) {
        return page->paramIds()[paramNum];
    }
    return std::string("");
}
#endif

// Module
std::shared_ptr<Parameter> Module::createParam(const std::vector<ParamValue> &args) {
    auto p = Parameter::create(args);
    if (p->valid()) {
        parameters_[p->id()] = p;
        return p;
    }
    return nullptr;
}

bool Module::changeParam(const EntityId &paramId, const ParamValue &value, bool force) {
    auto p = parameters_[paramId];
    if (p != nullptr) {
        if (p->change(value, force)) {
            return true;
        }
    }
    return false;
}


std::shared_ptr<Page> Module::createPage(
        const EntityId &pageId,
        const std::string &displayName,
        const std::vector<EntityId> paramIds
) {
    // std::cout << "Module::addPage " << id << std::endl;
    auto p = std::make_shared<Page>(pageId, displayName, paramIds);
    pages_[pageId] = p;
    pageIds_.push_back(pageId);
    return p;
}

// access functions
std::shared_ptr<Page> Module::getPage(const EntityId &pageId) {
    return pages_[pageId];
}

std::shared_ptr<Parameter> Module::getParam(const EntityId &paramId) {
    return parameters_[paramId];
}

std::vector<std::shared_ptr<Page>> Module::getPages() {
    std::vector<std::shared_ptr<Page>> ret;
    for (auto p : pageIds_) {
        auto page = pages_[p];
        if (page != nullptr) ret.push_back(page);
    }
    return ret;
}

std::vector<std::shared_ptr<Parameter>> Module::getParams() {
    std::vector<std::shared_ptr<Parameter>> ret;
    for (auto p : parameters_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::vector<std::shared_ptr<Parameter>> Module::getParams(const std::shared_ptr<Page> &page) {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (page != nullptr) {
        for (auto pid : page->paramIds()) {
            auto param = parameters_[pid];
            if (param != nullptr) ret.push_back(param);
        }
    }
    return ret;
}


bool Module::loadModuleDefinitions(const mec::Preferences &module) {
    if (!module.valid()) return false;

    type_ = module.getString("name");
    displayName_ = module.getString("display");

    if (module.exists("parameters")) {
        // load parameters
        mec::Preferences::Array params(module.getArray("parameters"));
        if (!params.valid()) return false;
        for (int i = 0; i < params.getSize(); i++) {
            mec::Preferences::Array pargs(params.getArray(i));
            if (!pargs.valid()) return false;

            std::vector<ParamValue> args;

            for (int j = 0; j < pargs.getSize(); j++) {
                mec::Preferences::Type t = pargs.getType(j);
                switch (t) {
                    case mec::Preferences::P_BOOL:
                        args.push_back(ParamValue(pargs.getBool(j) ? 1.0f : 0.0f));
                        break;
                    case mec::Preferences::P_NUMBER:
                        args.push_back(ParamValue((float) pargs.getDouble(j)));
                        break;
                    case mec::Preferences::P_STRING:
                        args.push_back(ParamValue(pargs.getString(j)));
                        break;
                        //ignore
                    case mec::Preferences::P_NULL:
                    case mec::Preferences::P_ARRAY:
                    case mec::Preferences::P_OBJECT:
                    default:
                        break;
                }
            }
            createParam(args);
        }
    }

    if (module.exists("pages")) {
        // load pages
        mec::Preferences::Array pages(module.getArray("pages"));

        if (!pages.valid()) return false;
        for (int i = 0; i < pages.getSize(); i++) {
            mec::Preferences::Array page(pages.getArray(i));
            if (!page.valid()) return false;

            if (page.getSize() < 2) return false; // need id, displayname
            auto id = page.getString(0);
            std::string displayname = page.getString(1);
            std::vector<std::string> paramIds;
            mec::Preferences::Array paramArray(page.getArray(2));
            for (int j = 0; j < paramArray.getSize(); j++) {
                paramIds.push_back(paramArray.getString(j));
            }
            createPage(id, displayname, paramIds);
        }
    }

    return true;

}

void Module::dumpSettings() const {
    LOG_1("Module Settings Dump");
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
        for (auto id : cc.second) {
            LOG_1("CC : " << cc.first << " -> " << id);
        }
    }
}


void Module::dumpParameters() {
    const char *IND = "    ";
    // print by page , this will miss anything not on a page, but gives a clear way of setting things
    LOG_1("Parameter Dump : " << displayName_ << " : " << type_);
    LOG_1("----------------------");
    for (std::string pageId : pageIds_) {
        auto page = pages_[pageId];
        if (page == nullptr) {
            LOG_1("Page not found: " << pageId);
            continue;
        }
        LOG_1(page->id());
        LOG_1(page->displayName());
        for (std::string paramId : page->paramIds()) {
            auto param = parameters_[paramId];
            if (param == nullptr) {
                LOG_1("Parameter not found:" << paramId);
                continue;
            }
            std::vector<ParamValue> args;
            param->createArgs(args);
            std::string d = IND;
            for (auto pv : args) {
                switch (pv.type()) {
                    case ParamValue::T_Float :
                        d += "  " + std::to_string(pv.floatValue()) + " [F],";
                        break;
                    case ParamValue::T_String :
                    default:
                        d += pv.stringValue() + " [S],";
                        break;
                }
            }
            LOG_1(d);
        }
        LOG_1("--------------");
    }
}

void Module::dumpCurrentValues() {
    const char *IND = "    ";
    // print by page , this will miss anything not on a page, but gives a clear way of setting things
    LOG_1("Current Values Dump");
    LOG_1("-------------------");
    for (std::string pageId : pageIds_) {
        auto page = pages_[pageId];
        if (page == nullptr) {
            LOG_1("Page not found: " << pageId);
            continue;
        }
        LOG_1(page->id());
        LOG_1(page->displayName());
        for (auto paramId : page->paramIds()) {
            auto param = parameters_[paramId];
            if (param == nullptr) {
                LOG_1("Parameter not found:" << paramId);
                continue;
            }
            std::string d = IND + paramId + " : ";
            ParamValue cv = param->current();
            switch (cv.type()) {
                case ParamValue::T_Float :
                    d += "  " + std::to_string(cv.floatValue()) + " [F],";
                    break;
                case ParamValue::T_String :
                default:
                    d += cv.stringValue() + " [S],";
                    break;
            }
            LOG_1(d);
        }
        LOG_1("--------------");
    }
}

bool Module::loadSettings(const mec::Preferences &prefs) {
    type_ = prefs.getString("module");
    mec::Preferences presets(prefs.getSubTree("presets"));
    if (presets.valid()) { // just ignore if not present

        for (std::string presetId : presets.getKeys()) {
            mec::Preferences params(presets.getSubTree(presetId));

            std::vector<Preset> preset;

            for (EntityId paramId : params.getKeys()) {
                mec::Preferences::Type t = params.getType(paramId);
                switch (t) {
                    case mec::Preferences::P_BOOL:
                        preset.push_back(Preset(paramId, ParamValue(params.getBool(paramId) ? 1.0f : 0.0f)));
                        break;
                    case mec::Preferences::P_NUMBER:
                        preset.push_back(Preset(paramId, ParamValue((float) params.getDouble(paramId))));
                        break;
                    case mec::Preferences::P_STRING:
                        preset.push_back(Preset(paramId, ParamValue(params.getString(paramId))));
                        break;
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

    mec::Preferences midimapping(prefs.getSubTree("midi-mapping"));
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
                        addMidiCCMapping(ccnum, paramId);
                    }
                }
            }
        }
    }
    return true;
}

bool Module::saveSettings(cJSON *root) {
    cJSON *presets = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "module", type().c_str());

    // presets
    cJSON_AddItemToObject(root, "presets", presets);
    for (auto p : presets_) {
        cJSON *preset = cJSON_CreateObject();
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

    // midi mapping
    cJSON *midi = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "midi-mapping", midi);
    cJSON *ccs = cJSON_CreateObject();
    cJSON_AddItemToObject(midi, "cc", ccs);
    for (auto mm : midi_mapping_) {
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
    return true;
}

std::vector<std::string> Module::getPresetList() {
    std::vector<std::string> presets;
    for (auto p : presets_) {
        presets.push_back(p.first);
    }
    return presets;
}

bool Module::updatePreset(std::string presetId) {
    std::vector<std::shared_ptr<Parameter>> params = getParams();
    currentPreset_ = presetId;
    std::vector<Preset> presets;
    for (auto p : params) {
        presets.push_back(Preset(p->id(), p->current()));
    }
    presets_[presetId] = presets;
    return true;
}

std::vector<Preset> Module::getPreset(std::string presetId) {
    return presets_[presetId];
}

std::vector<EntityId> Module::getParamsForCC(unsigned cc) {
    return midi_mapping_[cc];
}

void Module::addMidiCCMapping(unsigned ccnum, const EntityId &paramId) {
    auto v = midi_mapping_[ccnum];
    for (auto it = v.begin(); it != v.end(); it++) {
        if (*it == paramId) {
            return; // already preset
        }
    }
    midi_mapping_[ccnum].push_back(paramId);
}

void Module::removeMidiCCMapping(unsigned ccnum, const EntityId &paramId) {
    auto v = midi_mapping_[ccnum];
    for (auto it = v.begin(); it != v.end(); it++) {
        if (*it == paramId) {
            v.erase(it);
            midi_mapping_[ccnum] = v;
            return;
        }
    }
}


} //namespace
