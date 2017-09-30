#include "ParameterModel.h"

#include <algorithm>
#include <limits>
#include <string.h>
#include <iostream>

namespace oKontrol {



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
            i->param(src, *p);
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
        i->page(src, *p);
    }
    return true;
}

std::string ParameterModel::getParamId(const std::string& pageId, unsigned paramNum) {
    auto page = pages_[pageId];
    if (page == nullptr) return "";
    if (page->paramIds().size() < paramNum) return "";
    return page->paramIds()[paramNum];
}

void ParameterModel::addClient(const std::string& host, unsigned port) {
    for ( auto i : listeners_) {
        i->addClient(host, port);
    }
}


bool ParameterModel::changeParam(ParameterSource src, const std::string& id, const ParamValue& value) {
    auto p = parameters_[id];
    if (p != nullptr) {
        if (p->change(value)) {
            for ( auto i : listeners_) {
                i->changed(src, *p);
            }
            return true;
        }
    }
    return false;
}

void ParameterModel::publishMetaData() const {
    for ( auto i : listeners_) {
        for (auto p : parameters_) {
            i->param(PS_LOCAL,  *(p.second));
        }
        for (auto p : pages_) {
            i->page(PS_LOCAL,  *(p.second));
        }
    }
}


} //namespace
