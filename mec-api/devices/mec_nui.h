#pragma once

#include "../mec_device.h"

#include <KontrolModel.h>

#include <ip/UdpSocket.h>
#include <string>
#include <readerwriterqueue.h>
#include <thread>

#include <NuiDevice.h>


namespace mec {


class NuiParamMode;

class NuiListener;


enum NuiModes {
    NM_PARAMETER,
    NM_MAINMENU,
    NM_PRESETMENU,
    NM_MODULEMENU,
    NM_MODULESELECTMENU
};

class NuiMode : public Kontrol::KontrolCallback {
public:
    NuiMode() { ; }

    virtual ~NuiMode() { ; }

    virtual bool init() = 0;
    virtual void poll() = 0;
    virtual void activate() = 0;

    // fates device
    virtual void onButton(unsigned id, unsigned value) = 0;
    virtual void onEncoder(unsigned id, int value) = 0;
};


class Nui : public Device, public Kontrol::KontrolCallback {
public:
    Nui();
    virtual ~Nui();

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
    void addMode(NuiModes mode, std::shared_ptr<NuiMode>);
    void changeMode(NuiModes);
    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;

    void changePot(unsigned pot, float value);

    void displayPopup(const std::string &text, bool);
    void displayParamNum(unsigned num, const Kontrol::Parameter &p, bool local, bool selected);
    void clearParamNum(unsigned num);
    void displayLine(unsigned line, const char *);
    void invertLine(unsigned line);
    void clearDisplay();
    void displayTitle(const std::string &module, const std::string &page);

    std::shared_ptr<Kontrol::KontrolModel> model() { return Kontrol::KontrolModel::model(); }

    Kontrol::EntityId currentRack() { return currentRackId_; }

    void currentRack(const Kontrol::EntityId &p) { currentRackId_ = p; }

    Kontrol::EntityId currentModule() { return currentModuleId_; }

    void currentModule(const Kontrol::EntityId &modId);


    Kontrol::EntityId currentPage() { return currentPageId_; }

    void currentPage(const Kontrol::EntityId &id) { currentPageId_ = id; }

    // from fates device
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

    friend class NuiListener;

    std::vector<std::shared_ptr<Kontrol::Module>> getModules(const std::shared_ptr<Kontrol::Rack> &rack);

//    bool connect(const std::string& host, unsigned port);


    void stop() override;


    static const unsigned int OUTPUT_BUFFER_SIZE = 1024;


    std::shared_ptr<NuiLite::NuiDevice> device_;
    bool active_;

    Kontrol::EntityId currentRackId_;
    Kontrol::EntityId currentModuleId_;
    Kontrol::EntityId currentPageId_;

    bool midiLearnActive_;
    bool modulationLearnActive_;

    NuiModes currentMode_;
    std::map<NuiModes, std::shared_ptr<NuiMode>> modes_;
    std::vector<std::string> moduleOrder_;
    unsigned menuTimeout_;

};

class NuiDeviceCallback : public NuiLite::NuiCallback {
public:
    NuiDeviceCallback(Nui &p) : parent_(p) { ; }

    void onButton(unsigned id, unsigned value) override {
        parent_.onButton(id, value);
    }

    void onEncoder(unsigned id, int value) override {
        parent_.onEncoder(id, value);
    }

private:
    Nui &parent_;
};


}
