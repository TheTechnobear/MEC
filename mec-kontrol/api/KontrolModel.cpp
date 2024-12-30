#include "KontrolModel.h"
#include <mec_prefs.h>
#include <stdexcept>

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
    for (const auto &i : listeners_) {
        (i.second)->publishStart(CS_LOCAL, 1);
    }
    publishMetaData(localRack_);
}

void KontrolModel::publishMetaData(const std::shared_ptr<Rack> &rack) const {
    std::vector<std::shared_ptr<Module>> modules = getModules(rack);
    publishRack(CS_LOCAL, *rack);
    for (const auto &resType:rack->getResourceTypes()) {
        for (const auto &res : rack->getResources(resType)) {
            publishResource(CS_LOCAL, *rack, resType, res);
        }
    }

    publishPreset(rack);

    for (const auto &p : modules) {
        if (p != nullptr) publishMetaData(rack, p);
    }
    for (const auto &i : listeners_) {
        (i.second)->publishRackFinished(CS_LOCAL, *rack);
    }
}

void KontrolModel::publishMetaData(const std::shared_ptr<Rack> &rack, const std::shared_ptr<Module> &module) const {
    publishModule(CS_LOCAL, *rack, *module);

    for (const auto &param: module->getParams()) {
        publishParam(CS_LOCAL, *rack, *module, *param);
    }
    for (const auto &page: module->getPages()) {
        publishPage(CS_LOCAL, *rack, *module, *page);
    }
    for (const auto &param: module->getParams()) {
        publishChanged(CS_LOCAL, *rack, *module, *param);
    }

    publishMidiMapping(CS_LOCAL, *rack, *module, module->getMidiMapping());
}

void KontrolModel::publishPreset(const std::shared_ptr<Rack> &rack) const {
    for (const auto &i : listeners_) {
        (i.second)->loadPreset(CS_LOCAL, *rack, rack->currentPreset());
    }
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
    for (const auto &p : racks_) {
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
    for (const auto &p : listeners_) {
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
        unsigned port) {
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


void KontrolModel::deleteRack(ChangeSource src, const EntityId &rackId)
{
    if (localRack_ && localRack_->id() == rackId)
        localRack_ = nullptr;
    auto rack = getRack(rackId);
    if (rack)
    {
        for (const auto &i : listeners_) {
            (i.second)->deleteRack(src, *rack);
        }
    }
    racks_.erase(rackId);
}


void KontrolModel::activeModule(ChangeSource src, const EntityId &rackId ,const EntityId &moduleId) {
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    if (module != nullptr) {
        for (const auto &i : listeners_) {
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
    for (const auto &i : listeners_) {
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
    if (src.type()==ChangeSource::REMOTE  && localRack() && rackId == localRack()->id()) {
        localRack()->addMidiCCMapping(midiCC, moduleId, paramId);
    }

    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    auto param = getParam(module, paramId);
    if (param == nullptr) return;

    for (const auto &i : listeners_) {
        (i.second)->assignMidiCC(src, *rack, *module, *param, midiCC);
    }
}

void KontrolModel::publishStart(ChangeSource src, unsigned numRacks) {
    for (const auto &i : listeners_) {
        (i.second)->publishStart(src, numRacks);
    }
}

void KontrolModel::publishRackFinished(ChangeSource src, const EntityId &rackId) {
    for (const auto &i : listeners_) {
        auto rack = getRack(rackId);
        if (rack == nullptr)
            return;
        (i.second)->publishRackFinished(src, *rack);
    }
}

void KontrolModel::unassignMidiCC(ChangeSource src, const EntityId &rackId, const EntityId &moduleId,
                                  const EntityId &paramId, unsigned midiCC) {
    if (src.type()==ChangeSource::REMOTE  && localRack() && rackId == localRack()->id()) {
        localRack()->removeMidiCCMapping(midiCC, moduleId, paramId);
    }
    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    auto param = getParam(module, paramId);
    if (param == nullptr) return;

    for (const auto &i : listeners_) {
        (i.second)->unassignMidiCC(src, *rack, *module, *param, midiCC);
    }
}


void KontrolModel::assignModulation(ChangeSource src,
                      const EntityId &rackId,
                      const EntityId &moduleId,
                      const EntityId &paramId,
                      unsigned bus) {
    if (src.type()==ChangeSource::REMOTE  && localRack() && rackId == localRack()->id()) {
        //TODO: when adding src dependent modulation, check to see what we should use for remote mod
        localRack()->addModulationMapping("mod", bus, moduleId, paramId);
    }

    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    auto param = getParam(module, paramId);
    if (param == nullptr) return;

    for (const auto &i : listeners_) {
        (i.second)->assignModulation(src, *rack, *module, *param, bus);
    }
}

void KontrolModel::unassignModulation(ChangeSource src,
                        const EntityId &rackId,
                        const EntityId &moduleId,
                        const EntityId &paramId,
                        unsigned bus) {
    if (src.type()==ChangeSource::REMOTE && localRack() && rackId == localRack()->id()) {
        //TODO: when adding src dependent modulation, check to see what we should use for remote mod
        localRack()->removeModulationMapping("mod", bus, moduleId, paramId);
    }

    auto rack = getRack(rackId);
    auto module = getModule(rack, moduleId);
    auto param = getParam(module, paramId);
    if (param == nullptr) return;

    for (const auto &i : listeners_) {
        (i.second)->unassignModulation(src, *rack, *module, *param, bus);
    }
}



void KontrolModel::savePreset(ChangeSource src, const EntityId &rackId, std::string preset) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return;

    if (src.type()==ChangeSource::REMOTE && localRack() && rackId == localRack()->id()) {
        localRack()->savePreset(preset);
    } else {
        rack->currentPreset(preset);
    }

    for (const auto &i : listeners_) {
        (i.second)->savePreset(src, *rack, preset);
    }
}

void KontrolModel::loadPreset(ChangeSource src, const EntityId &rackId, std::string preset) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return;
    if (src.type()==ChangeSource::REMOTE  && localRack() && rackId == localRack()->id()) {
        localRack()->loadPreset(preset);
    } else {
        rack->currentPreset(preset);
    }

    for (const auto &i : listeners_) {
        (i.second)->loadPreset(src, *rack, preset);
    }
}

void KontrolModel::saveSettings(ChangeSource src, const EntityId &rackId) {
    if (src.type()==ChangeSource::REMOTE  && localRack() && rackId == localRack()->id()) {
        localRack()->saveSettings();
    }

    auto rack = getRack(rackId);
    if (rack == nullptr) return;
    for (const auto &i : listeners_) {
        (i.second)->saveSettings(src, *rack);
    }
}

void KontrolModel::ping(
        ChangeSource src,
        const std::string &host,
        unsigned port,
        unsigned keepAlive) const {
    for (const auto &i : listeners_) {
        (i.second)->ping(src, host, port, keepAlive);
    }
}


void KontrolModel::loadModule(ChangeSource src,
                              const EntityId &rackId,
                              const EntityId &moduleId,
                              const std::string &moduleType) {
    auto rack = getRack(rackId);
    if (rack == nullptr) return;

    for (const auto &i : listeners_) {
        (i.second)->loadModule(src, *rack, moduleId, moduleType);
    }
}
void KontrolModel::midiLearn(ChangeSource src, bool b) {
    for (const auto &i : listeners_) {
        (i.second)->midiLearn(src, b);
    }
}

void KontrolModel::modulationLearn(ChangeSource src, bool b) {
    for (const auto &i : listeners_) {
        (i.second)->modulationLearn(src, b);
    }
}


void KontrolModel::publishRack(ChangeSource src, const Rack &rack) const {
    for (const auto &i : listeners_) {
        (i.second)->rack(src, rack);
    }
}

void KontrolModel::publishModule(ChangeSource src, const Rack &rack, const Module &module) const {
    for (const auto &i : listeners_) {
        (i.second)->module(src, rack, module);
    }
}

void KontrolModel::publishPage(ChangeSource src, const Rack &rack, const Module &module, const Page &page) const {
    for (const auto &i : listeners_) {
        (i.second)->page(src, rack, module, page);
    }

}

void KontrolModel::publishParam(ChangeSource src, const Rack &rack, const Module &module,
                                const Parameter &param) const {
    for (const auto &i : listeners_) {
        (i.second)->param(src, rack, module, param);
    }

}

void KontrolModel::publishChanged(ChangeSource src, const Rack &rack, const Module &module,
                                  const Parameter &param) const {
    for (const auto &i : listeners_) {
        (i.second)->changed(src, rack, module, param);
    }
}


void KontrolModel::publishResource(ChangeSource src, const Rack &rack,
                                   const std::string &type, const std::string &res) const {
    for (const auto &i : listeners_) {
        (i.second)->resource(src, rack, type, res);
    }
}

void KontrolModel::publishMidiMapping(ChangeSource src, const Rack &rack, const Module &module,
                                      const MidiMap &midiMap) const {
    for (const auto &k : midiMap) {
        for (const auto &j : k.second) {
            auto parameter = module.getParam(j);
            if (parameter) {
                for (const auto &i : listeners_) {
                    (i.second)->assignMidiCC(src, rack, module, *parameter, k.first);
                }
            }
        }
    }
}

bool KontrolModel::loadModuleDefinitions(const EntityId &rackId, const EntityId &moduleId,
                                         const std::string &filename) {
    std::string file;
    if(filename.at(0)=='/') file = filename;
    else file=localRack()->mainDir() + "/" + filename;
    mec::Preferences prefs(file);
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
