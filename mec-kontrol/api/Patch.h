#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>


#include "Entity.h"
#include "Parameter.h"

namespace mec {
class Preferences;
}

namespace Kontrol {

enum ParameterSource {
    PS_LOCAL,
    PS_MIDI,
    PS_PRESET,
    PS_OSC
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


class Patch : public Entity {
public:
    Patch(const std::string& id, const std::string& displayName)
        : Entity(id, displayName) {
        ;
    }
    static std::shared_ptr<ParameterModel> model();

    std::shared_ptr<Parameter>  createParam(const std::vector<ParamValue>& args);
    bool changeParam(const EntityId& paramId, const ParamValue& value);

    std::shared_ptr<Page>  createPage(
        const EntityId& pageId,
        const std::string& displayName,
        const std::vector<EntityId> paramIds
    );


    void publishMetaData() const;


    std::shared_ptr<Page>                   getPage(const EntityId& pageId);
    std::shared_ptr<Parameter>              getParam(const EntityId& paramId);
    std::vector<std::shared_ptr<Page>>      getPages();
    std::vector<std::shared_ptr<Parameter>> getParams();
    std::vector<std::shared_ptr<Parameter>> getParams(const std::shared_ptr<Page>&);

    // unsigned    getPageCount() { return pageIds_.size();}
    // std::string getPageId(unsigned pageNum) { return pageNum < pageIds_.size() ? pageIds_[pageNum] : "";}
    // std::shared_ptr<Page> getPage(const std::string& pageId) { return pages_[pageId]; }
    // std::string getParamId(const std::string& pageId, unsigned paramNum);
    // std::shared_ptr<Parameter> getParam(const std::string paramId) { return parameters_[paramId]; }

    bool loadParameterDefinitions(const std::string& filename);
    bool loadParameterDefinitions(const mec::Preferences& prefs);

    bool loadPatchSettings(const std::string& filename);
    bool loadPatchSettings(const mec::Preferences& prefs);

    bool savePatchSettings();
    bool savePatchSettings(const std::string& filename);

    bool applyPreset(std::string presetId);
    bool savePreset(std::string presetId);
    std::string currentPreset() { return currentPreset_;}
    std::vector<std::string> getPresetList();

    bool changeMidiCC(unsigned midiCC, unsigned midiValue);
    void addMidiCCMapping(unsigned midiCC, std::string paramId);

    void dumpParameters();
    void dumpCurrentValues();
    void dumpPatchSettings();


private:
    std::vector<std::string> pageIds_; // ordered list of page id, for presentation
    std::unordered_map<std::string, std::shared_ptr<Parameter> >parameters_; // key = paramId
    std::unordered_map<std::string, std::shared_ptr<Page> >pages_; // key = pageId

    std::shared_ptr<mec::Preferences> paramDefinitions_;

    std::string patchSettingsFile_;
    std::string currentPreset_;
    std::shared_ptr<mec::Preferences> patchSettings_;

    std::unordered_map<unsigned, std::string> midi_mapping_; // key CC id, value = paramId
    std::unordered_map<std::string, std::vector<Preset>> presets_; // key = presetid
};





} //namespace


