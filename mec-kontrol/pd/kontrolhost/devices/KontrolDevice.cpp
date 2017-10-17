#include "KontrolDevice.h"

#include "m_pd.h"

KontrolDevice::KontrolDevice() {
    param_model_ = Kontrol::ParameterModel::model();
}

KontrolDevice::~KontrolDevice() {
    ;
}

void KontrolDevice::changeMode(unsigned mode) {
    currentMode_ = mode;
    auto m = modes_[mode];
    if(m!=nullptr) m->activate();
}

void KontrolDevice::addMode(unsigned mode, std::shared_ptr<DeviceMode> handler) {
    modes_[mode] = handler;
}

bool KontrolDevice::init() {
    param_model_->addCallback("pd.device", std::shared_ptr<KontrolDevice>(this));
    for(auto m : modes_) {
        if(m.second !=nullptr) m.second->init();
    }
    return true;
}

void KontrolDevice::poll() {
    auto m = modes_[currentMode_];
    if(m!=nullptr) m->poll();
}

void KontrolDevice::changePot(unsigned pot, float value) {
    auto m = modes_[currentMode_];
    if(m!=nullptr) m->changePot(pot,value);

}

void KontrolDevice::changeEncoder(unsigned encoder, float value) {
    auto m = modes_[currentMode_];
    if(m!=nullptr) m->changeEncoder(encoder,value);

}

void KontrolDevice::encoderButton(unsigned encoder, bool value) {
    auto m = modes_[currentMode_];
    if(m!=nullptr) m->encoderButton(encoder,value);

}

void KontrolDevice::addClient(const std::string& host, unsigned port) {
    auto m = modes_[currentMode_];
    if(m!=nullptr) m->addClient(host,port);
}

void KontrolDevice::page(Kontrol::ParameterSource src, const Kontrol::Page& p) {
    auto m = modes_[currentMode_];
    if(m!=nullptr) m->page(src,p);

}

void KontrolDevice::param(Kontrol::ParameterSource src , const Kontrol::Parameter& p)  {
    auto m = modes_[currentMode_];
    if(m!=nullptr) m->param(src,p);

}

void KontrolDevice::changed(Kontrol::ParameterSource src, const Kontrol::Parameter& p) {
    auto m = modes_[currentMode_];
    if(m!=nullptr) m->changed(src,p);
}

static t_pd *get_object(const char *s) {
  t_pd *x = gensym(s)->s_thing;
  return x;
}

void KontrolDevice::sendPdMessage(const char* obj, float f) {
  t_pd* sendobj = get_object(obj);
  if (!sendobj) { post("KontrolDevice::sendPdMessage to %s failed",obj); return; }

  t_atom a;
  SETFLOAT(&a, f);
  pd_forwardmess(sendobj, 1, &a);
}