#include "ParameterModel.h"

#include <algorithm>
#include <limits>
#include <string.h>
#include <iostream>
#include <map>

#include "mec_prefs.h"
#include "mec_log.h"


namespace Kontrol {

Page::Page(
    const std::string& id,
    const std::string& displayName,
    const std::vector<std::string> paramIds
) : Entity(id,displayName),
    paramIds_(paramIds) {
    ;
}

std::shared_ptr<ParameterModel> ParameterModel::model() {
    static std::shared_ptr<ParameterModel> model_;
    if (!model_) {
        model_ = std::shared_ptr<ParameterModel>(new ParameterModel());
    }
    return model_;
}




// void ParameterModel::free() {
//     model_.reset();
// }

ParameterModel::ParameterModel() {
}


bool ParameterModel::addParam( ParameterSource src, const std::vector<ParamValue>& args) {
    auto p = Parameter::create(args);
    if (p->valid()) {
        parameters_[p->id()] = p;
        // LOG_1("addParam " << p->id());
        for ( auto i : listeners_) {
            (i.second)->param(src, *p);
        }
        return true;
    }
    return false;
}

bool ParameterModel::addPage(
    ParameterSource src,
    const std::string& id,
    const std::string& displayName,
    const std::vector<std::string> paramIds
) {
    // std::cout << "ParameterModel::addPage " << id << std::endl;
    auto p = std::make_shared<Page>(id, displayName, paramIds);
    pages_[id] = p;
    pageIds_.push_back(id);
    for ( auto i : listeners_) {
        (i.second)->page(src, *p);
    }
    return true;
}

std::string ParameterModel::getParamId(const std::string& pageId, unsigned paramNum) {
    auto page = pages_[pageId];
    if (page != nullptr && paramNum < page->paramIds().size()) {
        return page->paramIds()[paramNum];
    }
    return std::string("");
}

void ParameterModel::addClient(const std::string& host, unsigned port) {
    for ( auto i : listeners_) {
        (i.second)->addClient(host, port);
    }
}


bool ParameterModel::changeParam(ParameterSource src, const std::string& id, const ParamValue& value) {
    auto p = parameters_[id];
    if (p != nullptr) {
        if (p->change(value)) {
            for ( auto i : listeners_) {
                (i.second)->changed(src, *p);
            }
            return true;
        }
    }
    return false;
}

void ParameterModel::publishMetaData() const {
    for ( auto i : listeners_) {
        for (auto p : parameters_) {
            const Parameter& param = *(p.second);
            (i.second)->param(PS_LOCAL, param);
            (i.second)->changed(PS_LOCAL, param);
        }
        for (auto pid : pageIds_) {
            auto p = pages_.find(pid);
            if (p != pages_.end()) {
                (i.second)->page(PS_LOCAL,  *(p->second));
            }
        }
    }
}

bool ParameterModel::applyPreset(std::string presetId) {
    auto presetvalues = presets_[presetId];
    bool ret = false;
    for (auto presetvalue : presetvalues) {
        auto param = parameters_[presetvalue.paramId()];
        if (param != nullptr) {
            if (presetvalue.value().type() == ParamValue::T_Float) {
                if (presetvalue.value() != param->current()) {
                    ret |= changeParam(PS_LOCAL, presetvalue.paramId(), presetvalue.value());
                }
            }
        }
    }
    return ret;
}

bool ParameterModel::changeMidiCC(unsigned midiCC, unsigned midiValue) {
    auto paramId = midi_mapping_[midiCC];
    if (!paramId.empty()) {
        auto param = parameters_[paramId];
        if (param != nullptr) {
            ParamValue pv = param->calcMidi(midiValue);
            if (pv != param->current()) {
                return changeParam(PS_MIDI, paramId, pv);
            }
        }
    }

    return false;
}

bool ParameterModel::loadParameterDefinitions(const std::string& filename) {
    paramDefinitions_ = std::make_shared<mec::Preferences>(filename);
    return loadParameterDefinitions(*paramDefinitions_);
}

bool ParameterModel::loadParameterDefinitions(const mec::Preferences& prefs) {
    if (!prefs.valid()) return false;

    mec::Preferences patch(prefs.getSubTree("patch"));
    if (!patch.valid()) return false;

    patchName_ = patch.getString("name");
    if (patchName_.empty()) {
        LOG_0("invalid parameter file : patch name missing" );
        return false;
    }

    if (patch.exists("parameters")) {
        // load parameters
        mec::Preferences::Array params(patch.getArray("parameters"));
        if (!params.valid()) return false;
        for (int i = 0; i < params.getSize(); i++) {

            mec::Preferences::Array pargs(params.getArray(i));
            if (!pargs.valid()) return false;

            std::vector<ParamValue> args;

            for (int j = 0; j < pargs.getSize(); j++) {
                mec::Preferences::Type t = pargs.getType(j);
                switch (t ) {
                case mec::Preferences::P_BOOL:      args.push_back(ParamValue(pargs.getBool(j) ? 1.0f : 0.0f )); break;
                case mec::Preferences::P_NUMBER:    args.push_back(ParamValue((float) pargs.getDouble(j))); break;
                case mec::Preferences::P_STRING:    args.push_back(ParamValue(pargs.getString(j))); break;
                //ignore
                case mec::Preferences::P_NULL:
                case mec::Preferences::P_ARRAY:
                case mec::Preferences::P_OBJECT:
                default:
                    break;
                }
            }
            addParam(PS_LOCAL, args);
        }
    }

    if (patch.exists("pages")) {
        // load pages
        mec::Preferences::Array pages(patch.getArray("pages"));

        if (!pages.valid()) return false;
        for (int i = 0; i < pages.getSize(); i++) {
            mec::Preferences::Array page(pages.getArray(i));
            if (!page.valid()) return false;

            if (page.getSize() < 2) return false; // need id, displayname
            std::string id = page.getString(0);
            std::string displayname = page.getString(1);
            std::vector<std::string> paramIds;
            mec::Preferences::Array paramArray(page.getArray(2));
            for ( int j = 0; j < paramArray.getSize(); j++) {
                paramIds.push_back(paramArray.getString(j));
            }
            addPage(PS_LOCAL, id, displayname, paramIds);
        }
    }

    return true;

}
bool ParameterModel::loadPatchSettings(const std::string& filename) {
    patchSettings_ = std::make_shared<mec::Preferences>(filename);
    return loadPatchSettings(*patchSettings_);
}


bool ParameterModel::loadPatchSettings(const mec::Preferences& prefs) {
    mec::Preferences presets(prefs.getSubTree("presets"));
    if (presets.valid()) { // just ignore if not present

        for (std::string presetId : presets.getKeys()) {

            mec::Preferences params(presets.getSubTree(presetId));
            std::vector<Preset> preset;

            for (std::string paramID : params.getKeys()) {
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
                std::string paramId = cc.getString(ccstr);
                addMidiCCMapping(ccnum, paramId);
            }
        }
    }

    return true;
}


void ParameterModel::addMidiCCMapping(unsigned ccnum, std::string paramId) {
    midi_mapping_[ccnum] = paramId;
}


void ParameterModel::dumpParameters() {
    const char* IND = "    ";
    // print by page , this will miss anything not on a page, but gives a clear way of setting things
    LOG_1("Parameter Dump");
    LOG_1("--------------");
    for (std::string pageId : pageIds_) {
        auto page = pages_[pageId];
        if (page == nullptr) { LOG_1("Page not found: " << pageId); continue;}
        LOG_1(page->id());
        LOG_1(page->displayName());
        for (std::string paramId : page->paramIds()) {
            auto param = parameters_[paramId];
            if (param == nullptr) { LOG_1("Parameter not found:" << paramId); continue;}
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

void ParameterModel::dumpCurrentValues() {
    const char* IND = "    ";
    // print by page , this will miss anything not on a page, but gives a clear way of setting things
    LOG_1("Current Values Dump");
    LOG_1("-------------------");
    for (std::string pageId : pageIds_) {
        auto page = pages_[pageId];
        if (page == nullptr) { LOG_1("Page not found: " << pageId); continue;}
        LOG_1(page->id());
        LOG_1(page->displayName());
        for (std::string paramId : page->paramIds()) {
            auto param = parameters_[paramId];
            if (param == nullptr) { LOG_1("Parameter not found:" << paramId); continue;}
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




void ParameterModel::dumpPatchSettings() {
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
