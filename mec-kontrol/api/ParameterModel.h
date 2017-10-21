#pragma once

#include <unordered_map>
#include <string>
#include <memory>

#include "Entity.h"
#include "Patch.h"
#include "Parameter.h"

namespace Kontrol {

class ParameterModel;

class ParameterCallback {
public:
    ParameterCallback() {
        ;
    }
    virtual ~ParameterCallback() {
        ;
    }
    virtual void stop() { ; }

    virtual void device(ParameterSource, const Device&) = 0;
    virtual void patch(ParameterSource, const Device&, const Patch&) = 0;
    virtual void page(ParameterSource src, const Device&, const Patch&, const Page&) = 0 ;
    virtual void param(ParameterSource src, const Device&, const Patch&, const Parameter&) = 0;
    virtual void changed(ParameterSource src, const Device&, const Patch&, const Parameter&) = 0;
};



class ParameterModel {
public:
	static std::shared_ptr<ParameterModel> model();
	// static void free();

	void publishMetaData() const;

	// observer functionality
	void clearCallbacks();
	void removeCallback(const std::string& id);
	void removeCallback(std::shared_ptr<ParameterCallback>);
	void addCallback(const std::string& id, std::shared_ptr<ParameterCallback> listener);


	// access
	std::shared_ptr<Device> 		getLocalDevice() const;
	std::shared_ptr<Device> 		getDevice(const EntityId& deviceId) const;
	std::shared_ptr<Patch>  		getPatch(const std::shared_ptr<Device>&, const EntityId& patchId) const;
	std::shared_ptr<Page>     		getPage(const std::shared_ptr<Patch>&, const EntityId& pageId) const;
	std::shared_ptr<Parameter>  	getParam(const std::shared_ptr<Patch>&, const EntityId& paramId) const;

	std::vector<std::shared_ptr<Device>> 	getDevices() const;
	std::vector<std::shared_ptr<Patch>>  	getPatches(const std::shared_ptr<Device>&) const;
	std::vector<std::shared_ptr<Page>> 		getPages(const std::shared_ptr<Patch>&) const;
	std::vector<std::shared_ptr<Parameter>> getParams(const std::shared_ptr<Patch>&) const;
	std::vector<std::shared_ptr<Parameter>> getParams(const std::shared_ptr<Patch>&, const std::shared_ptr<Page>&) const;


    void createDevice(
        ParameterSource src,
    	const EntityId& deviceId, 
    	const std::string& host, 
    	unsigned port) ;

    void createPatch(
        ParameterSource src, 
        const EntityId& deviceId,
        const EntityId& patchId
    ) const;

    void createParam(
        ParameterSource src, 
        const EntityId& deviceId,
        const EntityId& patchId,
        const std::vector<ParamValue>& args
    ) const;

    void createPage(
        ParameterSource src,
        const EntityId& deviceId,
        const EntityId& patchId,
        const EntityId& pageId,
        const std::string& displayName,
        const std::vector<EntityId> paramIds
    ) const;

    void changeParam(
        ParameterSource src, 
        const EntityId& deviceId,
        const EntityId& patchId,
        const EntityId& paramId, 
        ParamValue v) const;


    bool loadParameterDefinitions(const EntityId& deviceId, const EntityId& patchId, const std::string& filename);
    bool loadParameterDefinitions(const EntityId& deviceId, const EntityId& patchId, const mec::Preferences& prefs);

private:    
    void publishMetaData(const std::shared_ptr<Device>& device) const;
    void publishMetaData(const std::shared_ptr<Device>& device, const std::shared_ptr<Patch>& patch);

	ParameterModel();
	std::shared_ptr<Device>  localDevice_;
	std::unordered_map<EntityId, std::shared_ptr<Device>> devices_;
	std::unordered_map<std::string, std::shared_ptr<ParameterCallback> > listeners_; // key = source : host:ip
};

} //namespace
