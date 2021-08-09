#pragma once

#include "../mec_device.h"
#include "mec_mididevice.h"

#include <KontrolModel.h>

#include <iostream>

#include <ElectraMidi.h>
#include <MidiDevice.h>

#include "electraone/SysExStream.h"


namespace mec {


class ElectraOne : public Device, public Kontrol::KontrolCallback {
public:
    ElectraOne(ICallback &cb);
    virtual ~ElectraOne();

    //mec::Device
    bool init(void *) override;
    bool process() override;
    void deinit() override;
    bool isActive() override;

    //Kontrol::KontrolCallback

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override;
    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Page &) override;
    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override;
    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override;
    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &, const std::string &) override;
    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::EntityId &,
                    const std::string &) override;
    void savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;

    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;

    void publishStart(Kontrol::ChangeSource, unsigned numRacks) override;
    void publishRackFinished(Kontrol::ChangeSource, const Kontrol::Rack &) override;

    void saveSettings(Kontrol::ChangeSource, const Kontrol::Rack &) override;

//    void ping(Kontrol::ChangeSource src, const std::string &host, unsigned port, unsigned keepAlive) override {  }
//    void assignMidiCC(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &, unsigned midiCC)  override {  }
//    void unassignMidiCC(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &, unsigned midiCC)  override {  }
//    void assignModulation(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &, unsigned bus)  override  {  }
//    void unassignModulation(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &, unsigned bus) override  {  }


    std::shared_ptr<Kontrol::KontrolModel> model() { return Kontrol::KontrolModel::model(); }

    bool handleE1SysEx(Kontrol::ChangeSource src, SysExInputStream &sysex,
                       std::shared_ptr<Kontrol::KontrolModel> model);

private:
    bool send(SysExOutputStream &sysex);


    void stop() override;

    bool broadcastChange(Kontrol::ChangeSource src);
    unsigned stringToken(const char *);
    const std::string &tokenString(unsigned);

    void resetTokenCache();
    unsigned createStringToken(const char *tkn);

    std::shared_ptr<ElectraLite::MidiDevice> device_;
    std::shared_ptr<ElectraLite::MidiCallback> midiCallback_;
    bool active_;


    static constexpr int OUTPUT_MAX_SZ = 128;
    SysExOutputStream sysExOutStream_;
    SysExOutputStream stringOutStream_;

    unsigned pollCount_;
    unsigned pollFreq_;
    unsigned pollSleep_;

    unsigned lastToken_ = 0;
    std::map<std::string, unsigned> strToToken_;
    std::map<unsigned, std::string> tokenToString_;

    ICallback &callback_;
};;


}
