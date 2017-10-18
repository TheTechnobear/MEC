#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

#include "Parameter.h"

namespace mec{
class Preferences;
}

namespace Kontrol {

// todo: parameters probably need to be
// slot:paramid
// do we need pageid? probably not

// possibly
// map<slotId, vector<page.vector<paramid>>
// map<slotId, map<paramid,parameter>
// slots dont need order (here!?), pages do need an order, parameters need order in page
// when broadcast, we will need slot/paramid
// change will also need slotid/paramid


enum ParameterSource {
	PS_LOCAL,
	PS_OSC
};



class Device : public Entity {
public:
	Device(const std::string& id, const std::string& displayName) 
		: Entity(id,displayName) {
			;
	}
private:
};

class Patch : public Entity {
public:
	Patch(const std::string& id, const std::string& displayName) 
		: Entity(id,displayName) {
			;
	}
private:
};


class Page : public Entity {
public:
	Page(
	    const std::string& id,
	    const std::string& displayName,
	    const std::vector<std::string> paramIds
	);
	const std::vector<std::string>& paramIds() const { return paramIds_;}

private:
	std::vector<std::string> paramIds_;
};

class ParameterCallback {
public:
	ParameterCallback() {
		;
	}
	virtual ~ParameterCallback() {
		;
	}
	virtual void stop() { ; }
	virtual void addClient(const std::string& host, unsigned port) = 0;
	virtual void page(ParameterSource src, const Page&) = 0 ;
	virtual void param(ParameterSource src, const Parameter&) = 0;
	virtual void changed(ParameterSource src, const Parameter&) = 0;
};


class Preset {
public:
    Preset(const std::string& id, const ParamValue& v) : paramId_(id), value_(v) {;}
    std::string paramId() { return paramId_;}
    ParamValue value() { return value_;}
private:
    std::string paramId_;
    ParamValue value_;
};

class ParameterModel;

class ParameterModel {
public:
	static std::shared_ptr<ParameterModel> model();
	// static void free();

	void addClient(const std::string& host, unsigned port);
	bool addParam(ParameterSource src, const std::vector<ParamValue>& args);
	bool addPage(
	    ParameterSource src,
	    const std::string& id,
	    const std::string& displayName,
	    const std::vector<std::string> paramIds
	);

	bool  changeParam(ParameterSource src, const std::string& id, const ParamValue& value);

	void clearCallbacks() {
		for(auto p : listeners_) {
			(p.second)->stop();
		}

		listeners_.clear();
	}
	void removeCallback(const std::string& id) {
		auto p = listeners_.find(id);
		if(p!=listeners_.end()) {
			(p->second)->stop();
			listeners_.erase(id);
		}
	}

	void removeCallback(std::shared_ptr<ParameterCallback>) {
		// for(std::vector<std::shared_ptr<ParameterCallback> >::iterator i(listeners_) : listeners_) {
		// 	if(*i == listener) {
		// 		i.remove();
		// 		return;
		// 	}
		// }
	}
	void addCallback(const std::string& id, std::shared_ptr<ParameterCallback> listener) {
		auto p = listeners_[id];
		if(p!=nullptr) p->stop();
		listeners_[id] = listener; 
	}

    void publishMetaData() const;

	unsigned 	getPageCount() { return pageIds_.size();}
	std::string getPageId(unsigned pageNum) { return pageNum < pageIds_.size() ? pageIds_[pageNum] : "";}
	std::shared_ptr<Page> getPage(const std::string& pageId) { return pages_[pageId]; }

	std::string getParamId(const std::string& pageId, unsigned paramNum);
	std::shared_ptr<Parameter> getParam(const std::string paramId) { return parameters_[paramId]; }

	bool loadParameterDefinitions(const std::string& filename);
	bool loadParameterDefinitions(const mec::Preferences& prefs);
	bool loadPatchSettings(const std::string& filename);
	bool loadPatchSettings(const mec::Preferences& prefs);

	void dumpParameters();
	void dumpCurrentValues();
	void dumpPatchSettings();

	bool applyPreset(std::string presetId);
	bool changeMidiCC(unsigned midiCC, unsigned midiValue);
	void addMidiCCMapping(unsigned midiCC, std::string paramId);

private:
	ParameterModel();
	std::string patchName_; // temp

	std::shared_ptr<mec::Preferences> paramDefinitions_;
	std::shared_ptr<mec::Preferences> patchSettings_;

	std::unordered_map<unsigned, std::string> midi_mapping_; // key CC id, value = paramId
	std::unordered_map<std::string, std::vector<Preset>> presets_; // key = presetid

	std::vector<std::string> pageIds_; // ordered list of page id, for presentation
	
	std::unordered_map<std::string, std::shared_ptr<Parameter> >parameters_; // key = paramId
	std::unordered_map<std::string, std::shared_ptr<Page> >pages_; // key = pageId

	std::unordered_map<std::string,std::shared_ptr<ParameterCallback> > listeners_; // key = source : host:ip
};

} //namespace