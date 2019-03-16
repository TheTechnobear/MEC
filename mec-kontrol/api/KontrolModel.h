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
    virtual void resource(ChangeSource, const Rack &, const std::string &, const std::string &) = 0;

    virtual void deleteRack(ChangeSource, const Rack &) = 0;

    virtual void activeModule(ChangeSource, const Rack &, const Module &) { ; }
    virtual void loadModule(ChangeSource, const Rack &, const EntityId &, const std::string &) { ; }

    virtual void ping(ChangeSource src, const std::string &host, unsigned port, unsigned keepAlive) { ; }

    virtual void assignMidiCC(ChangeSource, const Rack &, const Module &, const Parameter &, unsigned midiCC) { ; }
    virtual void unassignMidiCC(ChangeSource, const Rack &, const Module &, const Parameter &, unsigned midiCC) { ; }

    virtual void assignModulation(ChangeSource, const Rack &, const Module &, const Parameter &, unsigned bus) { ; }
    virtual void unassignModulation(ChangeSource, const Rack &, const Module &, const Parameter &, unsigned bus) { ; }

    // Sent just before publishing all the information to other clients.
    virtual void publishStart(ChangeSource, unsigned numRacks) { ; }

    // Sent after completing each rack. The client should expect to receive the number of racks provided via the publishStart message.
    virtual void publishRackFinished(ChangeSource, const Rack &) { ; }

    virtual void savePreset(ChangeSource, const Rack &, std::string preset) { ; }

    virtual void loadPreset(ChangeSource, const Rack &, std::string preset) { ; }

    virtual void saveSettings(ChangeSource, const Rack &) { ; }


    virtual void midiLearn(ChangeSource src, bool b) { ; }
    virtual void modulationLearn(ChangeSource src, bool b) { ; }

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
            unsigned port
            );

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

    void createResource(ChangeSource src,
                        const EntityId &rackId,
                        const std::string &resType,
                        const std::string &resValue) const;

    void deleteRack(ChangeSource src,
                    const EntityId &rackId);

    void activeModule(ChangeSource src, const EntityId &rackId ,const EntityId &moduleId);

    void ping(ChangeSource src, const std::string &host, unsigned port, unsigned keepAlive) const;

    void assignMidiCC(ChangeSource src,
                      const EntityId &rackId,
                      const EntityId &moduleId,
                      const EntityId &paramId,
                      unsigned midiCC);
    void unassignMidiCC(ChangeSource src,
                        const EntityId &rackId,
                        const EntityId &moduleId,
                        const EntityId &paramId,
                        unsigned midiCC);

    void assignModulation(ChangeSource src,
                      const EntityId &rackId,
                      const EntityId &moduleId,
                      const EntityId &paramId,
                      unsigned bus);
    void unassignModulation(ChangeSource src,
                        const EntityId &rackId,
                        const EntityId &moduleId,
                        const EntityId &paramId,
                        unsigned bus);

    void publishStart(ChangeSource src, unsigned numRacks);
    void publishRackFinished(ChangeSource src, const EntityId &rackId);

    void savePreset(ChangeSource src,
                    const EntityId &rackId,
                    std::string preset);
    void loadPreset(ChangeSource src,
                    const EntityId &rackId,
                    std::string preset);
    void saveSettings(ChangeSource src,
                      const EntityId &rackId);

    void loadModule(ChangeSource src,
                    const EntityId &rackId,
                    const EntityId &moduleId,
                    const std::string &moduleType);

    void midiLearn(ChangeSource src, bool b);
    void modulationLearn(ChangeSource src, bool b);

    std::shared_ptr<Rack> createLocalRack(unsigned port);

    EntityId localRackId() { if (localRack_) return localRack_->id(); else return ""; }

    std::shared_ptr<Rack> localRack() { return localRack_; }

    void publishRack(ChangeSource, const Rack &) const;
    void publishModule(ChangeSource, const Rack &, const Module &) const;
    void publishPage(ChangeSource src, const Rack &, const Module &, const Page &) const;
    void publishParam(ChangeSource src, const Rack &, const Module &, const Parameter &) const;
    void publishChanged(ChangeSource src, const Rack &, const Module &, const Parameter &) const;
    void publishResource(ChangeSource src, const Rack &, const std::string &, const std::string &) const;
    void publishMidiMapping(ChangeSource src, const Rack &, const Module &, const MidiMap &midiMap) const;

    bool loadSettings(const EntityId &rackId, const std::string &filename);
    bool loadModuleDefinitions(const EntityId &rackId, const EntityId &moduleId, const std::string &filename);
    bool loadModuleDefinitions(const EntityId &rackId, const EntityId &moduleId, const mec::Preferences &prefs);

private:
    void publishMetaData(const std::shared_ptr<Rack> &rack) const;
    void publishMetaData(const std::shared_ptr<Rack> &rack, const std::shared_ptr<Module> &module) const;
    void publishPreset(const std::shared_ptr<Rack> &rack) const;

    KontrolModel();
    std::shared_ptr<Rack> localRack_;
    std::unordered_map<EntityId, std::shared_ptr<Rack>> racks_;
    std::unordered_map<std::string, std::shared_ptr<KontrolCallback> > listeners_; // key = source : host:ip
};

} //namespace
