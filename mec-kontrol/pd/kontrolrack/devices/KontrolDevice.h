#pragma once

#include <KontrolModel.h>
#include <Parameter.h>

#include <map>
#include <memory>


class KontrolDevice;

class DeviceMode :  public Kontrol::KontrolCallback {
public:
    virtual ~DeviceMode() {;}
    virtual bool init() = 0;
    virtual void poll() = 0;
    virtual void activate() = 0;
    virtual void changePot(unsigned pot, float value) = 0;
    virtual void changeEncoder(unsigned encoder, float value) = 0;
    virtual void encoderButton(unsigned encoder, bool value) = 0;

protected:
};


class KontrolDevice : public Kontrol::KontrolCallback  {
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

    //Kontrol::KontrolCallback
    virtual void rack(Kontrol::ParameterSource, const Kontrol::Rack&);
    virtual void module(Kontrol::ParameterSource, const Kontrol::Rack&, const Kontrol::Module&);
    virtual void page(Kontrol::ParameterSource, const Kontrol::Rack&, const Kontrol::Module&, const Kontrol::Page&);
    virtual void param(Kontrol::ParameterSource, const Kontrol::Rack&, const Kontrol::Module&, const Kontrol::Parameter&);
    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack&, const Kontrol::Module&, const Kontrol::Parameter&);

    virtual void midiCC(unsigned num, unsigned value);

    void sendPdMessage(const char* obj, float f);
    std::shared_ptr<Kontrol::KontrolModel> model() { return model_;}
protected:
    std::shared_ptr<Kontrol::KontrolModel> model_;
    std::map<unsigned, std::shared_ptr<DeviceMode>> modes_;
    unsigned currentMode_;
};
