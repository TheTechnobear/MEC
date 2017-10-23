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
    KontrolCallback() {
        ;
    }
    virtual ~KontrolCallback() {
        ;
    }
    virtual void stop() { ; }

    virtual void rack(ParameterSource, const Rack&) = 0;
    virtual void module(ParameterSource, const Rack&, const Module&) = 0;
    virtual void page(ParameterSource src, const Rack&, const Module&, const Page&) = 0 ;
    virtual void param(ParameterSource src, const Rack&, const Module&, const Parameter&) = 0;
    virtual void changed(ParameterSource src, const Rack&, const Module&, const Parameter&) = 0;
};



class KontrolModel {
public:
    static std::shared_ptr<KontrolModel> model();
    // static void free();

    void publishMetaData() const;

    // observer functionality
    void clearCallbacks();
    void removeCallback(const std::string& id);
    void removeCallback(std::shared_ptr<KontrolCallback>);
    void addCallback(const std::string& id, std::shared_ptr<KontrolCallback> listener);


    // access
    std::shared_ptr<Rack>          getLocalRack() const;
    std::shared_ptr<Rack>          getRack(const EntityId& rackId) const;
    std::shared_ptr<Module>        getModule(const std::shared_ptr<Rack>&, const EntityId& moduleId) const;
    std::shared_ptr<Page>          getPage(const std::shared_ptr<Module>&, const EntityId& pageId) const;
    std::shared_ptr<Parameter>     getParam(const std::shared_ptr<Module>&, const EntityId& paramId) const;

    std::vector<std::shared_ptr<Rack>>      getRacks() const;
    std::vector<std::shared_ptr<Module>>    getModules(const std::shared_ptr<Rack>&) const;
    std::vector<std::shared_ptr<Page>>      getPages(const std::shared_ptr<Module>&) const;
    std::vector<std::shared_ptr<Parameter>> getParams(const std::shared_ptr<Module>&) const;
    std::vector<std::shared_ptr<Parameter>> getParams(const std::shared_ptr<Module>&, const std::shared_ptr<Page>&) const;


    void createRack(
        ParameterSource src,
        const EntityId& rackId,
        const std::string& host,
        unsigned port) ;

    void createModule(
        ParameterSource src,
        const EntityId& rackId,
        const EntityId& moduleId,
        const std::string& displayName,
        const std::string& type
    ) const;

    void createParam(
        ParameterSource src,
        const EntityId& rackId,
        const EntityId& moduleId,
        const std::vector<ParamValue>& args
    ) const;

    void createPage(
        ParameterSource src,
        const EntityId& rackId,
        const EntityId& moduleId,
        const EntityId& pageId,
        const std::string& displayName,
        const std::vector<EntityId> paramIds
    ) const;

    void changeParam(
        ParameterSource src,
        const EntityId& rackId,
        const EntityId& moduleId,
        const EntityId& paramId,
        ParamValue v) const;


    void publishRack(ParameterSource, const Rack&) const;
    void publishModule(ParameterSource, const Rack&, const Module&) const;
    void publishPage(ParameterSource src, const Rack&, const Module&, const Page&) const;
    void publishParam(ParameterSource src, const Rack&, const Module&, const Parameter&) const;
    void publishChanged(ParameterSource src, const Rack&, const Module&, const Parameter&) const;


    bool loadModuleDefinitions(const EntityId& rackId, const EntityId& moduleId, const std::string& filename);
    bool loadModuleDefinitions(const EntityId& rackId, const EntityId& moduleId, const mec::Preferences& prefs);

private:
    void publishMetaData(const std::shared_ptr<Rack>& rack) const;
    void publishMetaData(const std::shared_ptr<Rack>& rack, const std::shared_ptr<Module>& module);

    KontrolModel();
    std::shared_ptr<Rack>  localRack_;
    std::unordered_map<EntityId, std::shared_ptr<Rack>> racks_;
    std::unordered_map<std::string, std::shared_ptr<KontrolCallback> > listeners_; // key = source : host:ip
};

} //namespace
