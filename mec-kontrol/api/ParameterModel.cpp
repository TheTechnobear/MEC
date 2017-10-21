#include "ParameterModel.h"

#include "Device.h"

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
    publishMetaData(localDevice_);
}

void ParameterModel::publishMetaData(const std::shared_ptr<Device>& device) const {
    std::vector<std::shared_ptr<Patch>> patches = getPatches(device);
    for (auto p : patches) {
        if (p != nullptr) publishMetaData(device);
    }
}


//access
std::shared_ptr<Device> ParameterModel::getLocalDevice() const {
    return localDevice_;
}

std::shared_ptr<Device>  ParameterModel::getDevice(const EntityId& deviceId) const {
    try {
        return devices_.at(deviceId);
    } catch (const std::out_of_range&) {
        return nullptr;
    }
}

std::shared_ptr<Patch> ParameterModel::getPatch(const std::shared_ptr<Device>& device, const EntityId& patchId) const {
    if (device != nullptr) return device->getPatch(patchId);
    return nullptr;
}

std::shared_ptr<Page>  ParameterModel::getPage(const std::shared_ptr<Patch>& patch, const EntityId& pageId) const {
    if (patch != nullptr) return patch->getPage(pageId);
    return nullptr;
}

std::shared_ptr<Parameter>  ParameterModel::getParam(const std::shared_ptr<Patch>& patch, const EntityId& paramId) const {
    if (patch != nullptr) return patch->getParam(paramId);
    return nullptr;
}


std::vector<std::shared_ptr<Device>>    ParameterModel::getDevices() const {
    std::vector<std::shared_ptr<Device>> ret;
    for (auto p : devices_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::vector<std::shared_ptr<Patch>>     ParameterModel::getPatches(const std::shared_ptr<Device>& device) const {
    std::vector<std::shared_ptr<Patch>> ret;
    if (device != nullptr) ret = device->getPatches();
    return ret;
}

std::vector<std::shared_ptr<Page>>      ParameterModel::getPages(const std::shared_ptr<Patch>& patch) const {
    std::vector<std::shared_ptr<Page>> ret;
    if (patch != nullptr) ret = patch->getPages();
    return ret;
}

std::vector<std::shared_ptr<Parameter>> ParameterModel::getParams(const std::shared_ptr<Patch>& patch) const {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (patch != nullptr) ret = patch->getParams();
    return ret;
}

std::vector<std::shared_ptr<Parameter>> ParameterModel::getParams(const std::shared_ptr<Patch>& patch, const std::shared_ptr<Page>& page) const {
    std::vector<std::shared_ptr<Parameter>> ret;
    if (patch != nullptr && page != nullptr) ret = patch->getParams(page);
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


void ParameterModel::createDevice(
    ParameterSource src,
    const EntityId& deviceId,
    const std::string& host,
    unsigned port
) {
    std::string desc = host;
    auto device = std::make_shared<Device>(host, port, desc);
    devices_[device->id()] = device;

    for ( auto i : listeners_) {
        (i.second)->device(src, *device);
    }
}

void ParameterModel::createPatch(
    ParameterSource src,
    const EntityId& deviceId,
    const EntityId& patchId
) const {

    auto device = getDevice(deviceId);

    if (device == nullptr) return;

    auto patch = std::make_shared<Patch>(patchId, patchId);
    device->addPatch(patch);

    for ( auto i : listeners_) {
        (i.second)->patch(src, *device, *patch);
    }
}

void ParameterModel::createPage(
    ParameterSource src,
    const EntityId& deviceId,
    const EntityId& patchId,
    const EntityId& pageId,
    const std::string& displayName,
    const std::vector<EntityId> paramIds
) const {
    auto device = getDevice(deviceId);
    auto patch = getPatch(device, patchId);
    if (patch == nullptr) return;

    auto page = patch->createPage(pageId, displayName, paramIds);
    if (page != nullptr) {
        for ( auto i : listeners_) {
            (i.second)->page(src, *device, *patch, *page);
        }
    }
}

void ParameterModel::createParam(
    ParameterSource src,
    const EntityId& deviceId,
    const EntityId& patchId,
    const std::vector<ParamValue>& args
) const {
    auto device = getDevice(deviceId);
    auto patch = getPatch(device, patchId);
    if (patch == nullptr) return;

    auto param = patch->createParam(args);
    if (param != nullptr) {
        for ( auto i : listeners_) {
            (i.second)->param(src, *device, *patch, *param);
        }
    }
}

void ParameterModel::changeParam(
    ParameterSource src,
    const EntityId& deviceId,
    const EntityId& patchId,
    const EntityId& paramId,
    ParamValue v) const {
    auto device = getDevice(deviceId);
    auto patch = getPatch(device, patchId);
    auto param = getParam(patch, paramId);
    if (param == nullptr) return;

    if (patch->changeParam(paramId, v)) {
        for ( auto i : listeners_) {
            (i.second)->changed(src, *device, *patch, *param);
        }
    }
}



bool ParameterModel::loadParameterDefinitions(const EntityId& deviceId, const EntityId& patchId, const std::string& filename) {
    auto device = getDevice(deviceId);
    if (device == nullptr) return false;
    return device->loadParameterDefinitions(patchId, filename);
}

bool ParameterModel::loadParameterDefinitions(const EntityId& deviceId, const EntityId& patchId, const mec::Preferences& prefs) {
    auto device = getDevice(deviceId);
    if (device == nullptr) return false;
    return device->loadParameterDefinitions(patchId, prefs);
}



} //namespace
