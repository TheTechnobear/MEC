#pragma once

#include "Parameter.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace oKontrol {

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

class Page {
public:
	Page(
	    const std::string& id,
	    const std::string& displayName,
	    const std::vector<std::string> paramIds
	);
	const std::string& id() const { return id_;};
	const std::string& displayName() const { return displayName_;};
	const std::vector<std::string>& paramIds() const { return paramIds_;}
private:

	std::string id_;
	std::string displayName_;
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

	virtual void addClient(const std::string& host, unsigned port) = 0;
	virtual void page(ParameterSource src, const Page&) = 0 ;
	virtual void param(ParameterSource src, const Parameter&) = 0;
	virtual void changed(ParameterSource src, const Parameter&) = 0;
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
		listeners_.clear();
	}
	void removeCallback(std::shared_ptr<ParameterCallback>) {
		// for(std::vector<std::shared_ptr<ParameterCallback> >::iterator i(listeners_) : listeners_) {
		// 	if(*i == listener) {
		// 		i.remove();
		// 		return;
		// 	}
		// }
	}
	void addCallback(std::shared_ptr<ParameterCallback> listener) {
		listeners_.push_back(listener);
	}

    void publishMetaData() const;

	unsigned 	getPageCount() { return pageIds_.size();}
	std::string getPageId(unsigned pageNum) { return pageNum < pageIds_.size() ? pageIds_[pageNum] : nullptr;}
	std::shared_ptr<Page> getPage(const std::string& pageId) { return pages_[pageId]; }

	std::string getParamId(const std::string& pageId, unsigned paramNum);
	std::shared_ptr<Parameter> getParam(const std::string paramId) { return parameters_[paramId]; }
private:
	ParameterModel();

	std::unordered_map<std::string, std::shared_ptr<Parameter> >parameters_;
	std::unordered_map<std::string, std::shared_ptr<Page> >pages_;
	std::vector<std::string> pageIds_;

	std::vector<std::shared_ptr<ParameterCallback> > listeners_;
};

} //namespace