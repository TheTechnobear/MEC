#pragma once

#include "Entity.h"
#include "ParamValue.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>


namespace Kontrol {

class Module;
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

class Rack : public Entity {
public:
    Rack( const std::string& host,
            unsigned port,
            const std::string& displayName)
        : Entity(createId(host, port), displayName) {
        ;
    }

    static EntityId createId(const std::string& host, unsigned port) {
        return (host + ":" + std::to_string(port));
    }

    std::vector<std::shared_ptr<Module>>  getModules();
    std::shared_ptr<Module> getModule(const EntityId& moduleId);
    void addModule(const std::shared_ptr<Module>& module);


    bool loadParameterDefinitions(const EntityId& moduleId, const std::string& filename);
    bool loadParameterDefinitions(const EntityId& moduleId, const mec::Preferences& prefs);

    bool loadModuleSettings(const std::string& filename);
    bool loadModuleSettings(const mec::Preferences& prefs);

    bool saveModuleSettings();
    bool saveModuleSettings(const std::string& filename);

    bool applyPreset(std::string presetId);
    bool savePreset(std::string presetId);

    std::string currentPreset() { return currentPreset_;}
    std::vector<std::string> getPresetList();

    bool changeMidiCC(unsigned midiCC, unsigned midiValue);
    void addMidiCCMapping(unsigned ccnum, const EntityId& moduleId, const EntityId& paramId);

    static std::shared_ptr<ParameterModel> model();

    void publishMetaData(const std::shared_ptr<Module>& module) const;
    void publishMetaData() const;

    void dumpModuleSettings() const;

    std::string host() const { return host_;}
    unsigned port() const { return port_;}
private:
    std::string host_;
    unsigned port_;
    std::unordered_map<EntityId, std::shared_ptr<Module>> modules_;

    std::string moduleSettingsFile_;
    std::string currentPreset_;
    std::shared_ptr<mec::Preferences> moduleSettings_;

    std::unordered_map<unsigned, std::string> midi_mapping_; // key CC id, value = paramId
    std::unordered_map<std::string, std::vector<Preset>> presets_; // key = presetid

    std::shared_ptr<mec::Preferences> paramDefinitions_;

};

}
