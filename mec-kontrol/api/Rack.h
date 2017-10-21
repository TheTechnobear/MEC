#pragma once

#include "Entity.h"
#include "ParamValue.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>


namespace Kontrol {

class Module;
class KontrolModel;


class Preset {
public:
    Preset(const EntityId& moduleId, const EntityId& paramId, const ParamValue& v) : 
        moduleId_(moduleId) , paramId_(paramId), value_(v) {;}
    const EntityId& moduleId() { return moduleId_;}
    const EntityId& paramId() { return paramId_;}
    ParamValue value() { return value_;}
private:
    EntityId moduleId_;
    EntityId paramId_;
    ParamValue value_;
};

class MidiMapping {
public: 
    MidiMapping(unsigned cc,const EntityId& moduleId, const EntityId& paramId) :   
        cc_(cc), moduleId_(moduleId) , paramId_(paramId) {;}
    const EntityId& moduleId() { return moduleId_;}
    const EntityId& paramId() { return paramId_;}
private:
    unsigned  cc_;
    EntityId moduleId_;
    EntityId paramId_;
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

    bool loadModuleDefinitions(const EntityId& moduleId, const mec::Preferences& prefs);

    bool loadRackSettings(const std::string& filename);
    bool loadRackSettings(const mec::Preferences& prefs);

    bool saveRackSettings();
    bool saveRackSettings(const std::string& filename);

    bool applyPreset(std::string presetId);
    bool savePreset(std::string presetId);

    std::string currentPreset() { return currentPreset_;}
    std::vector<std::string> getPresetList();

    bool changeMidiCC(unsigned midiCC, unsigned midiValue);
    void addMidiCCMapping(unsigned ccnum, const EntityId& moduleId, const EntityId& paramId);

    static std::shared_ptr<KontrolModel> model();

    void publishMetaData(const std::shared_ptr<Module>& module) const;
    void publishMetaData() const;

    void dumpRackSettings() const;

    std::string host() const { return host_;}
    unsigned port() const { return port_;}
private:
    std::string host_;
    unsigned port_;
    std::unordered_map<EntityId, std::shared_ptr<Module>> modules_;

    std::string rackSettingsFile_;
    std::shared_ptr<mec::Preferences> rackSettings_;
    std::string currentPreset_;

    std::unordered_map<unsigned, std::shared_ptr<MidiMapping>> midi_mapping_; // key CC id, value = paramId
    std::unordered_map<std::string, std::vector<Preset>> presets_; // key = presetid

    std::shared_ptr<mec::Preferences> moduleDefinitions_;

};

}
