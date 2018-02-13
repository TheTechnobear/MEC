#pragma once

#include <unordered_map>
#include <string>
#include <memory>

#include "Entity.h"
#include "Rack.h"
#include "Module.h"
#include "Parameter.h"

namespace Kontrol {

class KontrolModel;

class KontrolCallback {
public:
    KontrolCallback() { ; }

    virtual ~KontrolCallback() { ; }

    virtual void rack(ChangeSource, const Rack &) = 0;
    virtual void module(ChangeSource, const Rack &, const Module &) = 0;
    virtual void page(ChangeSource, const Rack &, const Module &, const Page &) = 0;
    virtual void param(ChangeSource, const Rack &, const Module &, const Parameter &) = 0;
    virtual void changed(ChangeSource, const Rack &, const Module &, const Parameter &) = 0;

    virtual void ping(ChangeSource src, unsigned port, const std::string &host, unsigned keepAlive) { ; }

    virtual void assignMidiCC(ChangeSource, const Rack &, const Module &, const Parameter &, unsigned midiCC) { ; }

    virtual void updatePreset(ChangeSource, const Rack &, std::string preset) { ; }

    virtual void applyPreset(ChangeSource, const Rack &, std::string preset) { ; }

    virtual void saveSettings(ChangeSource, const Rack &) { ; }

    virtual void stop() { ; }
};


class KontrolModel {
public:
    static std::shared_ptr<KontrolModel> model();
    // static void free();

    void publishMetaData() const;

    // observer functionality
    void clearCallbacks();
    void removeCallback(const std::string &id);
    void removeCallback(std::shared_ptr<KontrolCallback>);
    void addCallback(const std::string &id, const std::shared_ptr<KontrolCallback> &listener);

    // access
    std::shared_ptr<Rack> getLocalRack() const;
    std::shared_ptr<Rack> getRack(const EntityId &rackId) const;
    std::shared_ptr<Module> getModule(const std::shared_ptr<Rack> &, const EntityId &moduleId) const;
    std::shared_ptr<Page> getPage(const std::shared_ptr<Module> &, const EntityId &pageId) const;
    std::shared_ptr<Parameter> getParam(const std::shared_ptr<Module> &, const EntityId &paramId) const;

    std::vector<std::shared_ptr<Rack>> getRacks() const;
    std::vector<std::shared_ptr<Module>> getModules(const std::shared_ptr<Rack> &) const;
    std::vector<std::shared_ptr<Page>> getPages(const std::shared_ptr<Module> &) const;
    std::vector<std::shared_ptr<Parameter>> getParams(const std::shared_ptr<Module> &) const;
    std::vector<std::shared_ptr<Parameter>> getParams(const std::shared_ptr<Module> &,
                                                      const std::shared_ptr<Page> &) const;

    std::shared_ptr<Rack> createRack(
            ChangeSource src,
            const EntityId &rackId,
            const std::string &host,
            unsigned port);

    std::shared_ptr<Module> createModule(
            ChangeSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const std::string &displayName,
            const std::string &type
    ) const;

    std::shared_ptr<Parameter> createParam(
            ChangeSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const std::vector<ParamValue> &args
    ) const;

    std::shared_ptr<Page> createPage(
            ChangeSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const EntityId &pageId,
            const std::string &displayName,
            const std::vector<EntityId> paramIds
    ) const;

    std::shared_ptr<Parameter> changeParam(
            ChangeSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const EntityId &paramId,
            ParamValue v) const;

    void ping(ChangeSource src, const std::string &host, unsigned port, unsigned keepAlive) const;

    void assignMidiCC(ChangeSource src,
                      const EntityId &rackId,
                      const EntityId &moduleId,
                      const EntityId &paramId,
                      unsigned midiCC);
    void updatePreset(ChangeSource src,
                      const EntityId &rackId,
                      std::string preset);
    void applyPreset(ChangeSource src,
                     const EntityId &rackId,
                     std::string preset);
    void saveSettings(ChangeSource src,
                      const EntityId &rackId);


    std::shared_ptr<Rack> createLocalRack(unsigned port);

    EntityId localRackId() { if (localRack_) return localRack_->id(); else return ""; }

    std::shared_ptr<Rack> localRack() { return localRack_; }

    void publishRack(ChangeSource, const Rack &) const;
    void publishModule(ChangeSource, const Rack &, const Module &) const;
    void publishPage(ChangeSource src, const Rack &, const Module &, const Page &) const;
    void publishParam(ChangeSource src, const Rack &, const Module &, const Parameter &) const;
    void publishChanged(ChangeSource src, const Rack &, const Module &, const Parameter &) const;

    bool loadSettings(const EntityId &rackId, const std::string &filename);
    bool loadModuleDefinitions(const EntityId &rackId, const EntityId &moduleId, const std::string &filename);
    bool loadModuleDefinitions(const EntityId &rackId, const EntityId &moduleId, const mec::Preferences &prefs);

private:
    void publishMetaData(const std::shared_ptr<Rack> &rack) const;
    void publishMetaData(const std::shared_ptr<Rack> &rack, const std::shared_ptr<Module> &module) const;

    KontrolModel();
    std::shared_ptr<Rack> localRack_;
    std::unordered_map<EntityId, std::shared_ptr<Rack>> racks_;
    std::unordered_map<std::string, std::shared_ptr<KontrolCallback> > listeners_; // key = source : host:ip
};

} //namespace
