#pragma once

#include "Entity.h"
#include "ParamValue.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <set>


namespace Kontrol {

class Module;
class KontrolModel;

class Rack : public Entity {
public:
    Rack( const std::string& host,
          unsigned port,
          const std::string& displayName)
        : Entity(createId(host, port), displayName), host_(host), port_(port) {
        ;
    }

    static EntityId createId(const std::string& host, unsigned port) {
        return (host + ":" + std::to_string(port));
    }

    std::vector<std::shared_ptr<Module>>  getModules();
    std::shared_ptr<Module> getModule(const EntityId& moduleId);
    void addModule(const std::shared_ptr<Module>& module);


    bool loadModuleDefinitions(const EntityId& moduleId, const mec::Preferences& prefs);

    bool loadSettings(const std::string& filename);
    bool loadSettings(const mec::Preferences& prefs);

    bool saveSettings();
    bool saveSettings(const std::string& filename);

    bool applyPreset(std::string presetId);
    bool updatePreset(std::string presetId);

    const std::string& currentPreset() const { return currentPreset_;}
    std::vector<std::string> getPresetList();

    bool changeMidiCC(unsigned midiCC, unsigned midiValue);
    void addMidiCCMapping(unsigned ccnum, const EntityId& moduleId, const EntityId& paramId);
    void removeMidiCCMapping(unsigned ccnum, const EntityId& moduleId, const EntityId& paramId);

    static std::shared_ptr<KontrolModel> model();

    void publishMetaData(const std::shared_ptr<Module>& module) const;
    void publishMetaData() const;

    void publishCurrentValues(const std::shared_ptr<Module>& module) const;
    void publishCurrentValues() const;

    const std::set<std::string>& getResources(const std::string& type);
    void addResource(const std::string& type, const std::string& resource);


    void dumpSettings() const;
    void dumpParameters();
    void dumpCurrentValues();

    const std::string& host() const { return host_;}
    unsigned port() const { return port_;}

private:
    std::string host_;
    unsigned port_;
    std::unordered_map<EntityId, std::shared_ptr<Module>> modules_;
    std::unordered_map<std::string, std::set<std::string>> resources_;

    std::string settingsFile_;
    std::shared_ptr<mec::Preferences> settings_;
    std::string currentPreset_;

    std::shared_ptr<mec::Preferences> moduleDefinitions_;

};

}
