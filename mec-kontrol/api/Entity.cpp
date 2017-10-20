#include "Entity.h"

#include "Patch.h"

namespace Kontrol {


 void Device::addPatch(const std::shared_ptr<Patch>& patch) { 
    if(patch!=nullptr) {
        patches_[patch->id()] = patch;
    }
}


std::vector<std::shared_ptr<Patch>>  Device::getPatches() {
    std::vector<std::shared_ptr<Patch>> ret;
    for (auto p : patches_) {
        if (p.second != nullptr) ret.push_back(p.second);
    }
    return ret;
}

std::shared_ptr<Patch> Device::getPatch(const EntityId& patchId) {
    return patches_[patchId];
}

} //namespace