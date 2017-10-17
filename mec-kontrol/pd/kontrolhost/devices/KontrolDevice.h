#pragma once

#include "ParameterModel.h"
#include "Parameter.h"

#include <map>
#include <memory>


class KontrolDevice;

class DeviceMode :  public Kontrol::ParameterCallback {
public:
    virtual ~DeviceMode() {;}
    virtual bool init() = 0;
    virtual void poll() = 0;
    virtual void activate() = 0;
    virtual void changePot(unsigned pot, float value) = 0;
    virtual void changeEncoder(unsigned encoder, float value) = 0;
    virtual void encoderButton(unsigned encoder, bool value) = 0;

protected:
    std::shared_ptr<KontrolDevice> parent_;
};
 

class KontrolDevice : public Kontrol::ParameterCallback  {
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
    //Kontrol::ParameterCallback 
    virtual void addClient(const std::string&, unsigned );
    virtual void page(Kontrol::ParameterSource , const Kontrol::Page& );
    virtual void param(Kontrol::ParameterSource, const Kontrol::Parameter&);
    virtual void changed(Kontrol::ParameterSource src, const Kontrol::Parameter& p);

    void sendPdMessage(const char* obj, float f); 
    std::shared_ptr<Kontrol::ParameterModel> model() { return param_model_;}
protected:
      std::shared_ptr<Kontrol::ParameterModel> param_model_;
      std::map<unsigned, std::shared_ptr<DeviceMode>> modes_;
      unsigned currentMode_;
};