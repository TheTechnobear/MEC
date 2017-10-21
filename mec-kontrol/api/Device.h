#pragma once

#include "Entity.h"
#include "ParamValue.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>


namespace Kontrol {

class Patch;
class ParameterModel;


class Preset {
public:
    Preset(const std::string& id, const ParamValue& v) : paramId_(id), value_(v) {;}
    std::string paramId() { return paramId_;}
    ParamValue value() { return value_;}
private:
    std::string paramId_;
    ParamValue value_;
};

class Device : public Entity {
public:
    Device( const std::string& host,
            unsigned port,
            const std::string& displayName)
        : Entity(createId(host, port), displayName) {
        ;
    }

    static EntityId createId(const std::string& host, unsigned port) {
        return (host + ":" + std::to_string(port));
    }

    std::vector<std::shared_ptr<Patch>>  getPatches();
    std::shared_ptr<Patch> getPatch(const EntityId& patchId);
    void addPatch(const std::shared_ptr<Patch>& patch);


    bool loadParameterDefinitions(const EntityId& patchId, const std::string& filename);
    bool loadParameterDefinitions(const EntityId& patchId, const mec::Preferences& prefs);

    bool loadPatchSettings(const std::string& filename);
    bool loadPatchSettings(const mec::Preferences& prefs);

    bool savePatchSettings();
    bool savePatchSettings(const std::string& filename);

    bool applyPreset(std::string presetId);
    bool savePreset(std::string presetId);

    std::string currentPreset() { return currentPreset_;}
    std::vector<std::string> getPresetList();

    bool changeMidiCC(unsigned midiCC, unsigned midiValue);
    void addMidiCCMapping(unsigned ccnum, const EntityId& patchId, const EntityId& paramId);

    static std::shared_ptr<ParameterModel> model();

    void publishMetaData(const std::shared_ptr<Patch>& patch) const;
    void publishMetaData() const;

    void dumpPatchSettings() const;

    std::string host() const { return host_;}
    unsigned port() const { return port_;}
private:
    std::string host_;
    unsigned port_;
    std::unordered_map<EntityId, std::shared_ptr<Patch>> patches_;

    std::string patchSettingsFile_;
    std::string currentPreset_;
    std::shared_ptr<mec::Preferences> patchSettings_;

    std::unordered_map<unsigned, std::string> midi_mapping_; // key CC id, value = paramId
    std::unordered_map<std::string, std::vector<Preset>> presets_; // key = presetid

    std::shared_ptr<mec::Preferences> paramDefinitions_;

};

}
