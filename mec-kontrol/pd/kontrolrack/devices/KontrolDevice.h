#pragma once

#include <KontrolModel.h>
#include <Parameter.h>

#include <map>
#include <memory>


class KontrolDevice;

class DeviceMode : public Kontrol::KontrolCallback {
public:
    virtual ~DeviceMode() override { ; }

    virtual bool init() = 0;
    virtual void poll() = 0;
    virtual void activate() = 0;
    virtual void changePot(unsigned pot, float value) = 0;
    virtual void changeEncoder(unsigned encoder, float value) = 0;
    virtual void encoderButton(unsigned encoder, bool value) = 0;
    virtual void keyPress(unsigned key, unsigned value) = 0;

protected:
};


class KontrolDevice : public Kontrol::KontrolCallback {
public:
    KontrolDevice();
    virtual ~KontrolDevice();

    void changeMode(unsigned);
    void addMode(unsigned mode, std::shared_ptr<DeviceMode>);

    virtual bool init();
    virtual void poll();
    virtual void changePot(unsigned pot, float value);
    virtual void changeEncoder(unsigned encoder, float value);
    virtual void encoderButton(unsigned encoder, bool value);
    virtual void keyPress(unsigned key, unsigned value);

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


    virtual void midiCC(unsigned num, unsigned value);
    virtual void digital(unsigned bus, bool value);
    virtual void analog(unsigned bus, float value);


    void sendPdMessage(const char *obj, float f);

    std::shared_ptr<Kontrol::KontrolModel> model() { return model_; }


    void midiLearn(bool b);

    bool midiLearn() { return midiLearnActive_; }

    void modulationLearn(bool b);

    bool modulationLearn() { return modulationLearnActive_; }

    Kontrol::EntityId currentRack() { return currentRackId_; }

    void currentRack(const Kontrol::EntityId &p) { currentRackId_ = p; }

    Kontrol::EntityId currentModule() { return currentModuleId_; }

    void currentModule(const Kontrol::EntityId &modId);


private:
    std::shared_ptr<Kontrol::KontrolModel> model_;
    std::map<unsigned, std::shared_ptr<DeviceMode>> modes_;
    unsigned currentMode_;
    Kontrol::EntityId currentRackId_;
    Kontrol::EntityId currentModuleId_;
    Kontrol::EntityId lastParamId_; // for midi learn
    bool midiLearnActive_;
    bool modulationLearnActive_;

};
