#include "Module.h"

#include  "KontrolModel.h"
#include <iostream>
#include <map>
#include <fstream>


#include <mec_prefs.h>
#include <mec_log.h>


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

inline std::shared_ptr<KontrolModel> Module::model() {
    return KontrolModel::model();
}

std::shared_ptr<Page> Module::createPage(
        const EntityId &pageId,
        const std::string &displayName,
        const std::vector<EntityId> paramIds
) {
    // std::cout << "Module::addPage " << id << std::endl;
    auto page = pages_[pageId];
    if (page == nullptr) {
        pageIds_.push_back(pageId);
    }

    auto p = std::make_shared<Page>(pageId, displayName, paramIds);
    pages_[pageId] = p;
    return p;
}

// access functions
std::shared_ptr<Page> Module::getPage(const EntityId &pageId) {
    return pages_[pageId];
}

std::shared_ptr<Parameter> Module::getParam(const EntityId &paramId) {
    return parameters_[paramId];
}

std::shared_ptr<Parameter> Module::getParam(const EntityId & paramId) const
{
    auto parameter = parameters_.find(paramId);
    return parameter != parameters_.end() ? parameter->second : nullptr;
}

std::vector<std::shared_ptr<Page>> Module::getPages() {
    std::vector<std::shared_ptr<Page>> ret;
    for (const auto &p : pageIds_) {
        auto page = pages_[p];
        if (page != nullptr) ret.push_back(page);
    }
    return ret;
}

std::vector<std::shared_ptr<Parameter>> Module::getParams() {
    std::vector<std::shared_ptr<Parameter>> ret;
    for (const auto &p : parameters_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::vector<std::shared_ptr<Parameter>> Module::getParams(const std::shared_ptr<Page> &page) {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (page != nullptr) {
        for (const auto &pid : page->paramIds()) {
            auto param = parameters_[pid];
            if (param != nullptr) ret.push_back(param);
        }
    }
    return ret;
}


bool Module::loadModuleDefinitions(const mec::Preferences &module) {
    if (!module.valid()) return false;

    displayName_ = module.getString("display");
    parameters_.clear();
    pages_.clear();
    pageIds_.clear();
    midi_mapping_.clear();
    modulation_mapping_.clear();

    if (module.exists("parameters")) {
        // load parameters
        mec::Preferences::Array params(module.getArray("parameters"));
        if (!params.valid()) return false;
        for (unsigned int i = 0; i < params.getSize(); i++) {
            mec::Preferences::Array pargs(params.getArray(i));
            if (!pargs.valid()) return false;

            std::vector<ParamValue> args;

            for (unsigned int j = 0; j < pargs.getSize(); j++) {
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
        for (unsigned int i = 0; i < pages.getSize(); i++) {
            mec::Preferences::Array page(pages.getArray(i));
            if (!page.valid()) return false;

            if (page.getSize() < 2) return false; // need id, displayname
            auto id = page.getString(0);
            std::string displayname = page.getString(1);
            std::vector<std::string> paramIds;
            mec::Preferences::Array paramArray(page.getArray(2));
            for (unsigned int j = 0; j < paramArray.getSize(); j++) {
                paramIds.push_back(paramArray.getString(j));
            }
            createPage(id, displayname, paramIds);
        }
    }

    return true;

}

void Module::dumpParameters() {
    const char *IND = "    ";
    // print by page , this will miss anything not on a page, but gives a clear way of setting things
    LOG_1("Parameter Dump : " << displayName_ << " : " << type_);
    LOG_1("----------------------");
    for (const std::string &pageId : pageIds_) {
        auto page = pages_[pageId];
        if (page == nullptr) {
            LOG_1("Page not found: " << pageId);
            continue;
        }
        LOG_1(page->id());
        LOG_1(page->displayName());
        for (const std::string &paramId : page->paramIds()) {
            auto param = parameters_[paramId];
            if (param == nullptr) {
                LOG_1("Parameter not found:" << paramId);
                continue;
            }
            std::vector<ParamValue> args;
            param->createArgs(args);
            std::string d = IND;
            for (const auto &pv : args) {
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
    for (const std::string &pageId : pageIds_) {
        auto page = pages_[pageId];
        if (page == nullptr) {
            LOG_1("Page not found: " << pageId);
            continue;
        }
        LOG_1(page->id());
        LOG_1(page->displayName());
        for (const auto &paramId : page->paramIds()) {
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


std::vector<EntityId> Module::getParamsForCC(unsigned cc) {
    return midi_mapping_[cc];
}

void Module::addMidiCCMapping(unsigned ccnum, const EntityId &paramId) {
    auto v = midi_mapping_[ccnum];
    for (auto &it : v) {
        if (it == paramId) {
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

std::vector<EntityId> Module::getParamsForModulation(unsigned bus) {
    return modulation_mapping_[bus];

}

void Module::addModulationMapping(const std::string &src, unsigned bus, const EntityId &paramId) {
    //TODO: modulation mapping will be unique to src, so that they can be combined
    auto v = modulation_mapping_[bus];
    for (auto &it : v) {
        if (it == paramId) {
            return; // already preset
        }
    }
    modulation_mapping_[bus].push_back(paramId);
}


void Module::removeModulationMapping(const std::string &src, unsigned bus, const EntityId &paramId) {
    //TODO: modulation mapping will be unique to src, so that they can be combined
    auto v = modulation_mapping_[bus];
    for (auto it = v.begin(); it != v.end(); it++) {
        if (*it == paramId) {
            v.erase(it);
            modulation_mapping_[bus] = v;
            return;
        }
    }
}





} //namespace
