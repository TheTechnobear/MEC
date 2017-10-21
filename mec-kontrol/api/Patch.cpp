#include "Patch.h"

#include "ParameterModel.h"

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

std::string Patch::getParamId(const EntityId& pageId, unsigned paramNum) {
    auto page = pages_[pageId];
    if (page != nullptr && paramNum < page->paramIds().size()) {
        return page->paramIds()[paramNum];
    }
    return std::string("");
}
#endif





// Patch
std::shared_ptr<Parameter> Patch::createParam(const std::vector<ParamValue>& args) {
    auto p = Parameter::create(args);
    if (p->valid()) {
        parameters_[p->id()] = p;
        return p;
    }
    return nullptr;
}

bool Patch::changeParam(const EntityId& paramId, const ParamValue& value) {
    auto p = parameters_[paramId];
    if (p != nullptr) {
        if (p->change(value)) {
            return true;
        }
    }
    return false;
}


std::shared_ptr<Page> Patch::createPage(
    const EntityId& pageId,
    const std::string& displayName,
    const std::vector<EntityId> paramIds
) {
    // std::cout << "Patch::addPage " << id << std::endl;
    auto p = std::make_shared<Page>(pageId, displayName, paramIds);
    pages_[pageId] = p;
    pageIds_.push_back(pageId);
    return p;
}

// access functions
std::shared_ptr<Page> Patch::getPage(const EntityId& pageId) {
    return pages_[pageId];
}

std::shared_ptr<Parameter>  Patch::getParam(const EntityId& paramId) {
    return parameters_[paramId];
}

std::vector<std::shared_ptr<Page>> Patch::getPages() {
    std::vector<std::shared_ptr<Page>> ret;
    for (auto p : pageIds_) {
        auto page = pages_[p];
        if (page != nullptr) ret.push_back(page);
    }
    return ret;
}

std::vector<std::shared_ptr<Parameter>> Patch::getParams() {
    std::vector<std::shared_ptr<Parameter>> ret;
    for (auto p : parameters_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::vector<std::shared_ptr<Parameter>> Patch::getParams(const std::shared_ptr<Page>& page) {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (page != nullptr) {
        for (auto pid : page->paramIds()) {
            auto param = parameters_[pid];
            if (param != nullptr) ret.push_back(param);
        }
    }
    return ret;
}



bool Patch::loadParameterDefinitions(const mec::Preferences& prefs) {
    if (!prefs.valid()) return false;

    mec::Preferences patch(prefs.getSubTree("patch"));
    if (!patch.valid()) return false;

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
            createParam(args);
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
            auto id = page.getString(0);
            std::string displayname = page.getString(1);
            std::vector<std::string> paramIds;
            mec::Preferences::Array paramArray(page.getArray(2));
            for ( int j = 0; j < paramArray.getSize(); j++) {
                paramIds.push_back(paramArray.getString(j));
            }
            createPage(id, displayname, paramIds);
        }
    }

    return true;

}


void Patch::dumpParameters() {
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

void Patch::dumpCurrentValues() {
    const char* IND = "    ";
    // print by page , this will miss anything not on a page, but gives a clear way of setting things
    LOG_1("Current Values Dump");
    LOG_1("-------------------");
    for (std::string pageId : pageIds_) {
        auto page = pages_[pageId];
        if (page == nullptr) { LOG_1("Page not found: " << pageId); continue;}
        LOG_1(page->id());
        LOG_1(page->displayName());
        for (auto paramId : page->paramIds()) {
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



} //namespace