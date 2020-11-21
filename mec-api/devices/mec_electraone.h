#pragma once

#include "../mec_device.h"
#include "mec_mididevice.h"

#include <KontrolModel.h>

// #include <ip/UdpSocket.h>
// #include <string>
// #include <readerwriterqueue.h>
// #include <thread>

#include <iostream>

#include <ElectraDevice.h>
#include <MidiDevice.h>
#include <ElectraSchema.h>

namespace mec {


class ElectraOneParamMode;

class ElectraOneListener;


enum ElectraOneModes {
    E1_MAIN,
    E1_MAINMENU,
    E1_PRESETMENU,
    E1_MODULEMENU,
    E1_MODULESELECTMENU
};

class ElectraOneMode : public Kontrol::KontrolCallback {
public:
    ElectraOneMode() { ; }

    virtual ~ElectraOneMode() { ; }

    virtual bool init() = 0;
    virtual void poll() = 0;
    virtual void activate() = 0;

    // fates device
    virtual void onButton(unsigned id, unsigned value) = 0;
    virtual void onEncoder(unsigned id, int value) = 0;
};

class ElectraOneMidiCallback;

class ElectraOne : public Device, public Kontrol::KontrolCallback {
public:
    ElectraOne(ICallback& cb);
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
    void addMode(ElectraOneModes mode, std::shared_ptr<ElectraOneMode>);
    void changeMode(ElectraOneModes);
    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;

    void changePot(unsigned pot, float value);

    void send(ElectraOnePreset::Preset &preset);

    std::shared_ptr<Kontrol::KontrolModel> model() { return Kontrol::KontrolModel::model(); }

    Kontrol::EntityId currentRack() { return currentRackId_; }

    void currentRack(const Kontrol::EntityId &p) { currentRackId_ = p; }

    Kontrol::EntityId currentModule() { return currentModuleId_; }

    void currentModule(const Kontrol::EntityId &modId);


    Kontrol::EntityId currentPage() { return currentPageId_; }

    void currentPage(const Kontrol::EntityId &id) { currentPageId_ = id; }

    void onButton(unsigned id, unsigned value);
    void onEncoder(unsigned id, int value);

    void midiLearn(bool b);

    bool midiLearn() { return midiLearnActive_; }

    void modulationLearn(bool b);

    bool modulationLearn() { return modulationLearnActive_; }

    void nextModule();
    void prevModule();

    void outputCC(unsigned cc,unsigned v);
    void outputNoteOn(unsigned n,unsigned v);
    void outputNoteOff(unsigned n,unsigned v);

private:
    void navPrev();
    void navNext();
    void navActivate();

    void nextPage();
    void prevPage();

    friend class ElectraOneListener;

    std::vector<std::shared_ptr<Kontrol::Module>> getModules(const std::shared_ptr<Kontrol::Rack> &rack);

    void stop() override;

    std::shared_ptr<ElectraLite::ElectraDevice> device_;
    std::shared_ptr<ElectraLite::MidiDevice> mididevice_;// temp?
    std::shared_ptr<ElectraLite::MidiCallback> midiCB_;
    bool active_;

    std::string lastMessageSent_;

    Kontrol::EntityId currentRackId_;
    Kontrol::EntityId currentModuleId_;
    Kontrol::EntityId currentPageId_;

    bool midiLearnActive_;
    bool modulationLearnActive_;

    ElectraOneModes currentMode_;
    std::map<ElectraOneModes, std::shared_ptr<ElectraOneMode>> modes_;
    std::vector<std::string> moduleOrder_;

    unsigned pollCount_;
    unsigned pollFreq_;
    unsigned pollSleep_;

    ICallback& callback_;
};

class ElectraOneMidiCallback : public ElectraLite::MidiCallback {
public:
    ElectraOneMidiCallback(ElectraOne &p) : parent_(p) { ; }
    virtual ~ElectraOneMidiCallback() = default;

    void noteOn(unsigned int ch, unsigned int n, unsigned int v) override {
//        std::cerr << "noteOn" << ch << " " << n << " " << v << std::endl;
        parent_.onButton(n, v);
    }

    void noteOff(unsigned int ch, unsigned int n, unsigned int v) override {
//        std::cerr << "noteOff" << ch << " " << n << " " << v << std::endl;
        parent_.onButton(n, 0);
    }

    void cc(unsigned int ch, unsigned int cc, unsigned int v) override {
        std::cerr << "cc" << ch << " " << cc << " " << v << std::endl;
        parent_.onEncoder(cc, v);
    }

//    void pitchbend(unsigned ch, int v) override { ; } // +/- 8192
//    void ch_pressure(unsigned ch, unsigned v) override { ; }
private:
    ElectraOne &parent_;
};


class ElectraOneDeviceCallback : public ElectraLite::ElectraCallback {
public:
    ElectraOneDeviceCallback(ElectraOne &p) : parent_(p) { ; }

    virtual ~ElectraOneDeviceCallback() = default;

    void onInit() override { ; }

    void onDeinit() override { ; }

    void onError(unsigned err, const char *errStr) override { ; }

    void onInfo(const std::string &json) override { ; }

    void onPreset(const std::string &json) override { ; }

    void onConfig(const std::string &json) override { ; }

    void noteOn(unsigned int ch, unsigned int n, unsigned int v) override {
//        std::cerr << "noteOn" << ch << " " << n << " " << v << std::endl;
        parent_.onButton(n, v);
    }

    void noteOff(unsigned int ch, unsigned int n, unsigned int v) override {
//        std::cerr << "noteOff" << ch << " " << n << " " << v << std::endl;
        parent_.onButton(n, 0);
    }

    void cc(unsigned int ch, unsigned int cc, unsigned int v) override {
        std::cerr << "cc" << ch << " " << cc << " " << v << std::endl;
        parent_.onEncoder(cc, v);
    }


private:
    ElectraOne &parent_;
};


}
