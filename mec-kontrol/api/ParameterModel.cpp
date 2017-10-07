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
) :
    id_(id),
    displayName_(displayName),
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
    if (p->type() != PT_Invalid) {
        parameters_[p->id()] = p;
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

bool ParameterModel::loadParameterDefintions(const std::string& filename) {
    paramDefinitions_ = std::make_shared<mec::Preferences>(filename);
    return loadParameterDefintions(*paramDefinitions_);
}

bool ParameterModel::loadParameterDefintions(const mec::Preferences& prefs) {
    if (!prefs.valid()) return false;

    mec::Preferences patch(prefs.getSubTree("patch"));
    if (!patch.valid()) return false;

    patchName_ = patch.getString("name");
    if (patchName_.empty()) {
        LOG_0("invalid parameter file : patch name missing" );
        return false;
    }

    // load parameters
    mec::Preferences::Array params(patch.getArray("parameters"));
    if (!params.valid()) return false;
    for (int i = 0; i < params.getSize(); i++) {

        mec::Preferences::Array pargs(params.getArray(i));
        if (!pargs.valid()) return false;

        std::vector<ParamValue> args;

        for (int j = 0; j < pargs.getSize(); j++) {
            mec::Preferences::Type t = pargs.getType(i);
            switch (t ) {
            case mec::Preferences::P_BOOL: args.push_back(ParamValue(pargs.getBool(j) ? 1.0f : 0.0f )); break;
            case mec::Preferences::P_NUMBER: args.push_back(ParamValue((float) pargs.getDouble(j))); break;
            case mec::Preferences::P_STRING: args.push_back(ParamValue(pargs.getString(j))); break;
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

    // load pages
    mec::Preferences::Array pages(patch.getArray("pages"));

    if (!pages.valid()) return false;
    for (int i = 0; i < pages.getSize(); i++) {
        mec::Preferences::Array page(params.getArray(i));
        if (!page.valid()) return false;

        if (page.getSize() < 2) return false; // need id, displayname
        int j = 0;
        std::string id = page.getString(j++);
        std::string displayname = page.getString(j++);
        std::vector<std::string> paramIds;
        for ( ; j < page.getSize(); j++) {
            paramIds.push_back(pages.getString(i));
        }
        addPage(PS_LOCAL, id, displayname, paramIds);
    }


    return true;

}
bool ParameterModel::loadPatchSettings(const std::string& filename) {
    patchSettings_ = std::make_shared<mec::Preferences>(filename);
    return loadPatchSettings(*patchSettings_);
}

//TODO
class Preset {
public:
    Preset(const std::string& id, const ParamValue& v) : paramId_(id), value_(v) {;}
    std::string paramId_;
    ParamValue value_;
};

bool ParameterModel::loadPatchSettings(const mec::Preferences& prefs) {
    mec::Preferences presets(prefs.getSubTree("presets"));
    if (presets.valid()) { // just ignore if not present
        std::map<std::string, std::vector<Preset>> presets_;//TODO

        for(std::string presetId: presets.getKeys()) {

            mec::Preferences params(presets);
            std::vector<Preset> preset;

            for(std::string paramID: params.getKeys()) {
                mec::Preferences::Type t = params.getType(paramID);
                switch (t) {
                case mec::Preferences::P_BOOL:   preset.push_back(Preset(paramID,ParamValue(params.getBool(paramID)? 1.0f : 0.0f ))); break;
                case mec::Preferences::P_NUMBER: preset.push_back(Preset(paramID,ParamValue((float) params.getDouble(paramID)))); break;
                case mec::Preferences::P_STRING: preset.push_back(Preset(paramID,ParamValue(params.getString(paramID)))); break;
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
        if(cc.valid()) {
            for(std::string ccstr: cc.getKeys()) {
                int ccnum = std::stoi(ccstr);
                std::string paramId = cc.getString(ccstr);
                //TODO
                // auto p=getParam(paramId);
                // p->setMidiCC(ccnum);
            }
        }
    }

    return true;
}

} //namespace
