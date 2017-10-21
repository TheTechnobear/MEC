#include "ParameterModel.h"

namespace Kontrol {


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

void ParameterModel::publishMetaData() const {
    publishMetaData(localRack_);
}

void ParameterModel::publishMetaData(const std::shared_ptr<Rack>& rack) const {
    std::vector<std::shared_ptr<Module>> modules = getModules(rack);
    for (auto p : modules) {
        if (p != nullptr) publishMetaData(rack);
    }
}


//access
std::shared_ptr<Rack> ParameterModel::getLocalRack() const {
    return localRack_;
}

std::shared_ptr<Rack>  ParameterModel::getRack(const EntityId& rackId) const {
    try {
        return racks_.at(rackId);
    } catch (const std::out_of_range&) {
        return nullptr;
    }
}

std::shared_ptr<Module> ParameterModel::getModule(const std::shared_ptr<Rack>& rack, const EntityId& moduleId) const {
    if (rack != nullptr) return rack->getModule(moduleId);
    return nullptr;
}

std::shared_ptr<Page>  ParameterModel::getPage(const std::shared_ptr<Module>& module, const EntityId& pageId) const {
    if (module != nullptr) return module->getPage(pageId);
    return nullptr;
}

std::shared_ptr<Parameter>  ParameterModel::getParam(const std::shared_ptr<Module>& module, const EntityId& paramId) const {
    if (module != nullptr) return module->getParam(paramId);
    return nullptr;
}


std::vector<std::shared_ptr<Rack>>    ParameterModel::getRacks() const {
    std::vector<std::shared_ptr<Rack>> ret;
    for (auto p : racks_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::vector<std::shared_ptr<Module>>     ParameterModel::getModules(const std::shared_ptr<Rack>& rack) const {
    std::vector<std::shared_ptr<Module>> ret;
    if (rack != nullptr) ret = rack->getModules();
    return ret;
}

std::vector<std::shared_ptr<Page>>      ParameterModel::getPages(const std::shared_ptr<Module>& module) const {
    std::vector<std::shared_ptr<Page>> ret;
    if (module != nullptr) ret = module->getPages();
    return ret;
}

std::vector<std::shared_ptr<Parameter>> ParameterModel::getParams(const std::shared_ptr<Module>& module) const {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (module != nullptr) ret = module->getParams();
    return ret;
}

std::vector<std::shared_ptr<Parameter>> ParameterModel::getParams(const std::shared_ptr<Module>& module, const std::shared_ptr<Page>& page) const {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (module != nullptr && page != nullptr) ret = module->getParams(page);
    return ret;
}


// listener model
void ParameterModel::clearCallbacks() {
    for (auto p : listeners_) {
        (p.second)->stop();
    }

    listeners_.clear();
}
void ParameterModel::removeCallback(const std::string& id) {
    auto p = listeners_.find(id);
    if (p != listeners_.end()) {
        (p->second)->stop();
        listeners_.erase(id);
    }
}

void ParameterModel::removeCallback(std::shared_ptr<ParameterCallback>) {
    // for(std::vector<std::shared_ptr<ParameterCallback> >::iterator i(listeners_) : listeners_) {
    //  if(*i == listener) {
    //      i.remove();
    //      return;
    //  }
    // }
}
void ParameterModel::addCallback(const std::string& id, std::shared_ptr<ParameterCallback> listener) {
    auto p = listeners_[id];
    if (p != nullptr) p->stop();
    listeners_[id] = listener;
}


void ParameterModel::createRack(
    ParameterSource src,
    const EntityId& rackId,
    const std::string& host,
    unsigned port
) {
    std::string desc = host;
    auto rack = std::make_shared<Rack>(host, port, desc);
    racks_[rack->id()] = rack;

    for ( auto i : listeners_) {
        (i.second)->rack(src, *rack);
    }
}

void ParameterModel::createModule(
    ParameterSource src,
    const EntityId& rackId,
    const EntityId& moduleId
) const {

    auto rack = getRack(rackId);

    if (rack == nullptr) return;

    auto module = std::make_shared<Module>(moduleId, moduleId);
    rack->addModule(module);

    for ( auto i : listeners_) {
        (i.second)->module(src, *rack, *module);
    }
}

void ParameterModel::createPage(
    ParameterSource src,
    const EntityId& rackId,
    const EntityId& moduleId,
    const EntityId& pageId,
    const std::string& displayName,
    const std::vector<EntityId> paramIds
) const {
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    if (module == nullptr) return;

    auto page = module->createPage(pageId, displayName, paramIds);
    if (page != nullptr) {
        for ( auto i : listeners_) {
            (i.second)->page(src, *rack, *module, *page);
        }
    }
}

void ParameterModel::createParam(
    ParameterSource src,
    const EntityId& rackId,
    const EntityId& moduleId,
    const std::vector<ParamValue>& args
) const {
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    if (module == nullptr) return;

    auto param = module->createParam(args);
    if (param != nullptr) {
        for ( auto i : listeners_) {
            (i.second)->param(src, *rack, *module, *param);
        }
    }
}

void ParameterModel::changeParam(
    ParameterSource src,
    const EntityId& rackId,
    const EntityId& moduleId,
    const EntityId& paramId,
    ParamValue v) const {
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    auto param = getParam(module, paramId);
    if (param == nullptr) return;

    if (module->changeParam(paramId, v)) {
        for ( auto i : listeners_) {
            (i.second)->changed(src, *rack, *module, *param);
        }
    }
}



bool ParameterModel::loadParameterDefinitions(const EntityId& rackId, const EntityId& moduleId, const std::string& filename) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return false;
    return rack->loadParameterDefinitions(moduleId, filename);
}

bool ParameterModel::loadParameterDefinitions(const EntityId& rackId, const EntityId& moduleId, const mec::Preferences& prefs) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return false;
    return rack->loadParameterDefinitions(moduleId, prefs);
}



} //namespace
