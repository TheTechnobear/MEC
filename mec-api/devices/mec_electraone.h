#pragma once

#include "../mec_device.h"

#include <KontrolModel.h>

// #include <ip/UdpSocket.h>
// #include <string>
// #include <readerwriterqueue.h>
// #include <thread>

#include <ElectraDevice.h>
#include <ElectraSchema.h>

namespace mec {


class ElectraOneParamMode;

class ElectraOneListener;


enum ElectraOneModes {
    NM_PARAMETER,
    NM_MAINMENU,
    NM_PRESETMENU,
    NM_MODULEMENU,
    NM_MODULESELECTMENU
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


class ElectraOne : public Device, public Kontrol::KontrolCallback {
public:
    ElectraOne();
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

    void send(ElectraOnePreset::Preset& preset);

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

    unsigned menuTimeout() { return menuTimeout_; }

    void nextModule();
    void prevModule();
private:

    void navPrev();
    void navNext();
    void navActivate();

    void nextPage();
    void prevPage();

    friend class ElectraOneListener;

    std::vector<std::shared_ptr<Kontrol::Module>> getModules(const std::shared_ptr<Kontrol::Rack> &rack);

//    bool connect(const std::string& host, unsigned port);


    void stop() override;



    std::shared_ptr<ElectraLite::ElectraDevice> device_;
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
    unsigned menuTimeout_;

    unsigned pollCount_;
    unsigned pollFreq_;
    unsigned pollSleep_;

};

class ElectraOneDeviceCallback : public ElectraLite::ElectraCallback {
public:
    ElectraOneDeviceCallback(ElectraOne &p) : parent_(p) { ; }

    virtual ~ElectraOneDeviceCallback() = default;
    
     void onInit() override {;}
     void onDeinit() override {;}
     void onError(unsigned err, const char *errStr) override {;}
     void onInfo(const std::string& json) override {;}
     void onPreset(const std::string& json) override {;}
     void onConfig(const std::string& json) override {;}

    void noteOn(unsigned int ch, unsigned int n, unsigned int v) override {
        int b = n - 60;
        if(b>=0 && b<=3) parent_.onButton(b,v);
    }

    void noteOff(unsigned int ch, unsigned int n, unsigned int v) override {
        int b = n - 60;
        if(b>=0 && b<=3) parent_.onButton(b,0);
    }

    void cc(unsigned int ch, unsigned int cc, unsigned int v) override {
         if(cc<12) parent_.onEncoder(cc,v);
    }
private:
    ElectraOne &parent_;
};


}