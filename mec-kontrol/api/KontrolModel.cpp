#include "KontrolModel.h"
#include <mec_prefs.h>

namespace Kontrol {


std::shared_ptr<KontrolModel> KontrolModel::model() {
    static std::shared_ptr<KontrolModel> model_;
    if (!model_) {
        model_ = std::shared_ptr<KontrolModel>(new KontrolModel());
    }
    return model_;
}

// void KontrolModel::free() {
//     model_.reset();
// }

KontrolModel::KontrolModel() {
}

void KontrolModel::publishMetaData() const {
    publishMetaData(localRack_);
}

void KontrolModel::publishMetaData(const std::shared_ptr<Rack>& rack) const {
    std::vector<std::shared_ptr<Module>> modules = getModules(rack);
    for (auto p : modules) {
        if (p != nullptr) publishMetaData(rack);
    }
}


//access
std::shared_ptr<Rack> KontrolModel::getLocalRack() const {
    return localRack_;
}

std::shared_ptr<Rack>  KontrolModel::getRack(const EntityId& rackId) const {
    try {
        return racks_.at(rackId);
    } catch (const std::out_of_range&) {
        return nullptr;
    }
}

std::shared_ptr<Module> KontrolModel::getModule(const std::shared_ptr<Rack>& rack, const EntityId& moduleId) const {
    if (rack != nullptr) return rack->getModule(moduleId);
    return nullptr;
}

std::shared_ptr<Page>  KontrolModel::getPage(const std::shared_ptr<Module>& module, const EntityId& pageId) const {
    if (module != nullptr) return module->getPage(pageId);
    return nullptr;
}

std::shared_ptr<Parameter>  KontrolModel::getParam(const std::shared_ptr<Module>& module, const EntityId& paramId) const {
    if (module != nullptr) return module->getParam(paramId);
    return nullptr;
}


std::vector<std::shared_ptr<Rack>>    KontrolModel::getRacks() const {
    std::vector<std::shared_ptr<Rack>> ret;
    for (auto p : racks_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::vector<std::shared_ptr<Module>>     KontrolModel::getModules(const std::shared_ptr<Rack>& rack) const {
    std::vector<std::shared_ptr<Module>> ret;
    if (rack != nullptr) ret = rack->getModules();
    return ret;
}

std::vector<std::shared_ptr<Page>>      KontrolModel::getPages(const std::shared_ptr<Module>& module) const {
    std::vector<std::shared_ptr<Page>> ret;
    if (module != nullptr) ret = module->getPages();
    return ret;
}

std::vector<std::shared_ptr<Parameter>> KontrolModel::getParams(const std::shared_ptr<Module>& module) const {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (module != nullptr) ret = module->getParams();
    return ret;
}

std::vector<std::shared_ptr<Parameter>> KontrolModel::getParams(const std::shared_ptr<Module>& module, const std::shared_ptr<Page>& page) const {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (module != nullptr && page != nullptr) ret = module->getParams(page);
    return ret;
}


// listener model
void KontrolModel::clearCallbacks() {
    for (auto p : listeners_) {
        (p.second)->stop();
    }

    listeners_.clear();
}
void KontrolModel::removeCallback(const std::string& id) {
    auto p = listeners_.find(id);
    if (p != listeners_.end()) {
        (p->second)->stop();
        listeners_.erase(id);
    }
}

void KontrolModel::removeCallback(std::shared_ptr<KontrolCallback>) {
    // for(std::vector<std::shared_ptr<KontrolCallback> >::iterator i(listeners_) : listeners_) {
    //  if(*i == listener) {
    //      i.remove();
    //      return;
    //  }
    // }
}
void KontrolModel::addCallback(const std::string& id, std::shared_ptr<KontrolCallback> listener) {
    auto p = listeners_[id];
    if (p != nullptr) p->stop();
    listeners_[id] = listener;
}


void KontrolModel::createRack(
    ParameterSource src,
    const EntityId& rackId,
    const std::string& host,
    unsigned port
) {
    std::string desc = host;
    auto rack = std::make_shared<Rack>(host, port, desc);
    racks_[rack->id()] = rack;

    publishRack(src,*rack);
}

void KontrolModel::createModule(
    ParameterSource src,
    const EntityId& rackId,
    const EntityId& moduleId,
    const std::string& displayName,
    const std::string& type
) const {

    auto rack = getRack(rackId);

    if (rack == nullptr) return;

    auto module = std::make_shared<Module>(moduleId, displayName,type);
    rack->addModule(module);

    publishModule(src, *rack, *module);
}

void KontrolModel::createPage(
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
        publishPage(src,*rack, *module, *page);
    }
}

void KontrolModel::createParam(
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
        publishParam(src, *rack, *module, *param);
    }
}

void KontrolModel::changeParam(
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
        publishChanged(src, *rack, *module, *param);
    }
}

void KontrolModel::publishRack(ParameterSource src, const Rack& rack) const {
    for ( auto i : listeners_) {
        (i.second)->rack(src, rack);
    }
}

void KontrolModel::publishModule(ParameterSource src , const Rack& rack, const Module& module) const {
    for ( auto i : listeners_) {
        (i.second)->module(src, rack, module);
    }
}

void KontrolModel::publishPage(ParameterSource src, const Rack& rack, const Module& module, const Page& page) const {
    for ( auto i : listeners_) {
        (i.second)->page(src, rack, module, page);
    }

}

void KontrolModel::publishParam(ParameterSource src, const Rack& rack, const Module& module, const Parameter& param) const {
    for ( auto i : listeners_) {
     (i.second)->param(src, rack, module, param);
    }

}

void KontrolModel::publishChanged(ParameterSource src, const Rack& rack, const Module& module, const Parameter& param) const {
    for ( auto i : listeners_) {
        (i.second)->changed(src, rack, module, param);
    }
}

bool KontrolModel::loadModuleDefinitions(const EntityId& rackId, const EntityId& moduleId, const std::string& filename) {
    mec::Preferences prefs(filename);
    return loadModuleDefinitions(rackId,moduleId, prefs);
}

bool KontrolModel::loadModuleDefinitions(const EntityId& rackId, const EntityId& moduleId, const mec::Preferences& prefs) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return false;
    return rack->loadModuleDefinitions(moduleId, prefs);
}

bool KontrolModel::loadSettings(const EntityId& rackId,const std::string& filename) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return false;
    return rack->loadSettings(filename);
}


} //namespace
