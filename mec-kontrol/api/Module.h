#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>


#include "Entity.h"
#include "Parameter.h"
#include "ChangeSource.h"
#include "Rack.h"

namespace mec {
class Preferences;
}

namespace Kontrol {

class KontrolModel;


class Module : public Entity {
public:
    Module(const std::string &id,
           const std::string &displayName,
           const std::string &type)
            : Entity(id, displayName), type_(type) {
        ;
    }

    static std::shared_ptr<KontrolModel> model();

    std::shared_ptr<Parameter> createParam(const std::vector<ParamValue> &args);
    bool changeParam(const EntityId &paramId, const ParamValue &value, bool force);

    std::shared_ptr<Page> createPage(
            const EntityId &pageId,
            const std::string &displayName,
            const std::vector<EntityId> paramIds
    );


    std::shared_ptr<Page> getPage(const EntityId &pageId);
    std::shared_ptr<Parameter> getParam(const EntityId &paramId);
    std::vector<std::shared_ptr<Page>> getPages();
    std::vector<std::shared_ptr<Parameter>> getParams();
    std::vector<std::shared_ptr<Parameter>> getParams(const std::shared_ptr<Page> &);

    // unsigned    getPageCount() { return pageIds_.size();}
    // std::string getPageId(unsigned pageNum) { return pageNum < pageIds_.size() ? pageIds_[pageNum] : "";}
    // std::shared_ptr<Page> getPage(const std::string& pageId) { return pages_[pageId]; }
    // std::string getParamId(const std::string& pageId, unsigned paramNum);
    // std::shared_ptr<Parameter> getParam(const std::string paramId) { return parameters_[paramId]; }

    std::string type() const { return type_; };

    bool loadModuleDefinitions(const mec::Preferences &prefs);
    void dumpParameters();
    void dumpCurrentValues();


    std::vector<EntityId> getParamsForCC(unsigned cc);

    void addMidiCCMapping(unsigned ccnum, const EntityId &paramId);
    void removeMidiCCMapping(unsigned ccnum, const EntityId &paramId);

    MidiMap getMidiMapping() { return midi_mapping_; }

    void setMidiMapping(const MidiMap &map) { midi_mapping_ = map; }

private:
    std::string type_;

    std::vector<std::string> pageIds_; // ordered list of page id, for presentation
    std::unordered_map<std::string, std::shared_ptr<Parameter> > parameters_; // key = paramId
    std::unordered_map<std::string, std::shared_ptr<Page> > pages_; // key = pageId
    MidiMap midi_mapping_; // key CC id, value = paramId

};


} //namespace


