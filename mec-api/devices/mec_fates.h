#pragma once



#include "../mec_device.h"

#include <fateslite/FatesDevice.h>


#include <KontrolModel.h>

#include <ip/UdpSocket.h>
#include <string>
#include <readerwriterqueue.h>
#include <thread>


namespace mec {

class FatesParamMode;
class FatesListener;


enum FatesModes {
    FM_PARAMETER,
    FM_MAINMENU,
    FM_PRESETMENU,
    FM_MODULEMENU,
    FM_MODULESELECTMENU
};

class FatesMode : public Kontrol::KontrolCallback {
public:
    FatesMode() { ; }
    virtual ~FatesMode() { ; }

    virtual void poll() = 0;
    virtual void activate() = 0;
    virtual void navPrev()=0;
    virtual void navNext()=0;
    virtual void navActivate()=0;
};


class Fates : public Device, public Kontrol::KontrolCallback {
public:
    Fates();
    virtual ~Fates();

    //mec::Device
    bool init(void*) override;
    bool process() override;
    void deinit() override ;
    bool isActive() override;

    //Kontrol::KontrolCallback

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override;
    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Page &) override;
    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;
    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;
    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &, const std::string &) override;
    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::EntityId &, const std::string &) override;
    void savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void addMode(FatesModes mode, std::shared_ptr<FatesMode>);
    void changeMode(FatesModes);
    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;

    void changePot(unsigned pot, float value);

    void displayPopup(const std::string &text, bool);
    void displayParamNum(unsigned num, const Kontrol::Parameter &p, bool local);
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
    void currentPage(const Kontrol::EntityId &id) { currentPageId_ = id;}


    void midiLearn(bool b);
    bool midiLearn() { return midiLearnActive_;}
    void modulationLearn(bool b);
    bool modulationLearn() { return modulationLearnActive_;}

    unsigned menuTimeout() {return menuTimeout_;}
private:

    void navPrev();
    void navNext();
    void navActivate();

    void nextPage();
    void prevPage();
    void nextModule();
    void prevModule();

    friend class FatesListener;

    std::vector<std::shared_ptr<Kontrol::Module>> getModules(const std::shared_ptr<Kontrol::Rack>& rack);

    bool connect(const std::string& host, unsigned port);


    void stop() override;


    static const unsigned int OUTPUT_BUFFER_SIZE = 1024;


    bool active_;

    Kontrol::EntityId currentRackId_;
    Kontrol::EntityId currentModuleId_;
    Kontrol::EntityId currentPageId_;

    bool midiLearnActive_;
    bool modulationLearnActive_;

    FatesModes currentMode_;
    std::map<FatesModes, std::shared_ptr<FatesMode>> modes_;
    std::vector<std::string> moduleOrder_;
    unsigned menuTimeout_;

};

}
