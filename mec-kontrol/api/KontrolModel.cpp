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

void KontrolModel::publishMetaData(const std::shared_ptr<Rack> &rack) const {
    std::vector<std::shared_ptr<Module>> modules = getModules(rack);
    publishRack(CS_LOCAL, *rack);
    for (auto resType:rack->getResourceTypes()) {
        for (auto res : rack->getResources(resType)) {
            publishResource(CS_LOCAL, *rack, resType, res);
        }
    }
    for (const auto &p : modules) {
        if (p != nullptr) publishMetaData(rack, p);
    }
}

void KontrolModel::publishMetaData(const std::shared_ptr<Rack> &rack, const std::shared_ptr<Module> &module) const {
    publishModule(CS_LOCAL, *rack, *module);
    for (const auto param: module->getParams()) {
        publishParam(CS_LOCAL, *rack, *module, *param);
    }
    for (const auto page: module->getPages()) {
        publishPage(CS_LOCAL, *rack, *module, *page);
    }
    for (const auto param: module->getParams()) {
        publishChanged(CS_LOCAL, *rack, *module, *param);
    }
    publishMidiMapping(CS_LOCAL, *rack, *module, module->getMidiMapping());
}


std::shared_ptr<Rack> KontrolModel::createLocalRack(unsigned port) {
    std::string host = "127.0.0.1";
    auto rackId = Rack::createId(host, port);
    localRack_ = createRack(CS_LOCAL, rackId, host, port);
    return localRack_;
}

//access
std::shared_ptr<Rack> KontrolModel::getLocalRack() const {
    return localRack_;
}

std::shared_ptr<Rack> KontrolModel::getRack(const EntityId &rackId) const {
    try {
        return racks_.at(rackId);
    } catch (const std::out_of_range &) {
        return nullptr;
    }
}

std::shared_ptr<Module> KontrolModel::getModule(const std::shared_ptr<Rack> &rack, const EntityId &moduleId) const {
    if (rack != nullptr) return rack->getModule(moduleId);
    return nullptr;
}

std::shared_ptr<Page> KontrolModel::getPage(const std::shared_ptr<Module> &module, const EntityId &pageId) const {
    if (module != nullptr) return module->getPage(pageId);
    return nullptr;
}

std::shared_ptr<Parameter> KontrolModel::getParam(const std::shared_ptr<Module> &module,
                                                  const EntityId &paramId) const {
    if (module != nullptr) return module->getParam(paramId);
    return nullptr;
}

std::vector<std::shared_ptr<Rack>> KontrolModel::getRacks() const {
    std::vector<std::shared_ptr<Rack>> ret;
    for (auto p : racks_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::vector<std::shared_ptr<Module>> KontrolModel::getModules(const std::shared_ptr<Rack> &rack) const {
    std::vector<std::shared_ptr<Module>> ret;
    if (rack != nullptr) ret = rack->getModules();
    return ret;
}

std::vector<std::shared_ptr<Page>> KontrolModel::getPages(const std::shared_ptr<Module> &module) const {
    std::vector<std::shared_ptr<Page>> ret;
    if (module != nullptr) ret = module->getPages();
    return ret;
}

std::vector<std::shared_ptr<Parameter>> KontrolModel::getParams(const std::shared_ptr<Module> &module) const {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (module != nullptr) ret = module->getParams();
    return ret;
}

std::vector<std::shared_ptr<Parameter>> KontrolModel::getParams(const std::shared_ptr<Module> &module,
                                                                const std::shared_ptr<Page> &page) const {
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

void KontrolModel::removeCallback(const std::string &id) {
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

void KontrolModel::addCallback(const std::string &id, const std::shared_ptr<KontrolCallback> &listener) {
    auto p = listeners_[id];
    if (p != nullptr) p->stop();
    listeners_[id] = listener;
}

std::shared_ptr<Rack> KontrolModel::createRack(
        ChangeSource src,
        const EntityId &rackId,
        const std::string &host,
        unsigned port
) {
    std::string desc = host;
    auto rack = std::make_shared<Rack>(host, port, desc);
    racks_[rack->id()] = rack;

    publishRack(src, *rack);
    return rack;
}

std::shared_ptr<Module> KontrolModel::createModule(
        ChangeSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const std::string &displayName,
        const std::string &type
) const {

    auto rack = getRack(rackId);

    if (rack == nullptr) return nullptr;

    auto module = std::make_shared<Module>(moduleId, displayName, type);
    rack->addModule(module);

    publishModule(src, *rack, *module);
    return module;
}

std::shared_ptr<Page> KontrolModel::createPage(
        ChangeSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const EntityId &pageId,
        const std::string &displayName,
        const std::vector<EntityId> paramIds
) const {
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    if (module == nullptr) return nullptr;

    auto page = module->createPage(pageId, displayName, paramIds);
    if (page != nullptr) {
        publishPage(src, *rack, *module, *page);
    }
    return page;
}

std::shared_ptr<Parameter> KontrolModel::createParam(
        ChangeSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const std::vector<ParamValue> &args
) const {
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    if (module == nullptr) return nullptr;

    auto param = module->createParam(args);
    if (param != nullptr) {
        publishParam(src, *rack, *module, *param);
    }
    return param;
}


void KontrolModel::activeModule(ChangeSource src, const EntityId &rackId ,const EntityId &moduleId) {
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    if (module != nullptr) {
        for (auto i : listeners_) {
            (i.second)->activeModule(src, *rack, *module);
        }
    }
}


void KontrolModel::createResource(ChangeSource src,
                                  const EntityId &rackId,
                                  const std::string &resType,
                                  const std::string &resValue) const {
    auto rack = getRack(rackId);
    if (rack == nullptr) return;
    rack->addResource(resType, resValue);
    for (auto i : listeners_) {
        (i.second)->resource(src, *rack, resType, resValue);
    }
}


std::shared_ptr<Parameter> KontrolModel::changeParam(
        ChangeSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const EntityId &paramId,
        ParamValue v) const {
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    auto param = getParam(module, paramId);
    if (param == nullptr) return nullptr;

    if (module->changeParam(paramId, v, src == CS_PRESET)) {
        publishChanged(src, *rack, *module, *param);
    }
    return param;
}

void KontrolModel::assignMidiCC(ChangeSource src, const EntityId &rackId, const EntityId &moduleId,
                                const EntityId &paramId, unsigned midiCC) {
    if (localRack() && rackId == localRack()->id()) {
        localRack()->addMidiCCMapping(midiCC, moduleId, paramId);
    } else {
        auto rack = getRack(rackId);
        auto module = getModule(rack, moduleId);
        auto param = getParam(module, paramId);
        if (param == nullptr) return;

        rack->addMidiCCMapping(midiCC, moduleId, paramId);

        for (auto i : listeners_) {
            (i.second)->assignMidiCC(src, *rack, *module, *param, midiCC);
        }
    }
}


void KontrolModel::unassignMidiCC(ChangeSource src, const EntityId &rackId, const EntityId &moduleId,
                                  const EntityId &paramId, unsigned midiCC) {
    if (localRack() && rackId == localRack()->id()) {
        localRack()->removeMidiCCMapping(midiCC, moduleId, paramId);
    } else {
        auto rack = getRack(rackId);
        auto module = getModule(rack, moduleId);
        auto param = getParam(module, paramId);
        if (param == nullptr) return;

        for (auto i : listeners_) {
            (i.second)->unassignMidiCC(src, *rack, *module, *param, midiCC);
        }
    }
}


void KontrolModel::updatePreset(ChangeSource src, const EntityId &rackId, std::string preset) {
    if (localRack() && rackId == localRack()->id()) {
        localRack()->updatePreset(preset);
    } else {
        auto rack = getRack(rackId);
        if (rack == nullptr) return;
        for (auto i : listeners_) {
            (i.second)->updatePreset(src, *rack, preset);
        }
    }
}

void KontrolModel::applyPreset(ChangeSource src, const EntityId &rackId, std::string preset) {
    if (localRack() && rackId == localRack()->id()) {
        localRack()->applyPreset(preset);
    } else {
        auto rack = getRack(rackId);
        if (rack == nullptr) return;
        for (auto i : listeners_) {
            (i.second)->applyPreset(src, *rack, preset);
        }
    }
}

void KontrolModel::saveSettings(ChangeSource src, const EntityId &rackId) {
    if (localRack() && rackId == localRack()->id()) {
        localRack()->saveSettings();
    } else {
        auto rack = getRack(rackId);
        if (rack == nullptr) return;
        for (auto i : listeners_) {
            (i.second)->saveSettings(src, *rack);
        }
    }
}

void KontrolModel::ping(
        ChangeSource src,
        const std::string &host,
        unsigned port,
        unsigned keepAlive) const {
    for (auto i : listeners_) {
        (i.second)->ping(src, host, port, keepAlive);
    }
}


void KontrolModel::loadModule(ChangeSource src,
                              const EntityId &rackId,
                              const EntityId &moduleId,
                              const std::string &moduleType) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return;
    for (auto i : listeners_) {
        (i.second)->loadModule(src, *rack, moduleId, moduleType);
    }
}


void KontrolModel::publishRack(ChangeSource src, const Rack &rack) const {
    for (auto i : listeners_) {
        (i.second)->rack(src, rack);
    }
}

void KontrolModel::publishModule(ChangeSource src, const Rack &rack, const Module &module) const {
    for (auto i : listeners_) {
        (i.second)->module(src, rack, module);
    }
}

void KontrolModel::publishPage(ChangeSource src, const Rack &rack, const Module &module, const Page &page) const {
    for (auto i : listeners_) {
        (i.second)->page(src, rack, module, page);
    }

}

void KontrolModel::publishParam(ChangeSource src, const Rack &rack, const Module &module,
                                const Parameter &param) const {
    for (auto i : listeners_) {
        (i.second)->param(src, rack, module, param);
    }

}

void KontrolModel::publishChanged(ChangeSource src, const Rack &rack, const Module &module,
                                  const Parameter &param) const {
    for (auto i : listeners_) {
        (i.second)->changed(src, rack, module, param);
    }
}


void KontrolModel::publishResource(ChangeSource src, const Rack &rack,
                                   const std::string &type, const std::string &res) const {
    for (auto i : listeners_) {
        (i.second)->resource(src, rack, type, res);
    }
}

void KontrolModel::publishMidiMapping(ChangeSource src, const Rack &rack, const Module &module,
                                      const MidiMap &midiMap) const {
    for (auto k : midiMap) {
        for (auto j : k.second) {
            auto parameter = module.getParam(j);
            if (parameter) {
                for (auto i : listeners_) {
                    (i.second)->assignMidiCC(src, rack, module, *parameter, k.first);
                }
            }
        }
    }
}

bool KontrolModel::loadModuleDefinitions(const EntityId &rackId, const EntityId &moduleId,
                                         const std::string &filename) {
    mec::Preferences prefs(filename);
    return loadModuleDefinitions(rackId, moduleId, prefs);
}

bool KontrolModel::loadModuleDefinitions(const EntityId &rackId, const EntityId &moduleId,
                                         const mec::Preferences &prefs) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return false;
    return rack->loadModuleDefinitions(moduleId, prefs);
}

bool KontrolModel::loadSettings(const EntityId &rackId, const std::string &filename) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return false;
    return rack->loadSettings(filename);
}

} //namespace
