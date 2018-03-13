#include "KontrolDevice.h"

#include "../../m_pd.h"

KontrolDevice::KontrolDevice() {
    model_ = Kontrol::KontrolModel::model();
}

KontrolDevice::~KontrolDevice() {
    ;
}

void KontrolDevice::changeMode(unsigned mode) {
    currentMode_ = mode;
    auto m = modes_[mode];
    if (m != nullptr) m->activate();
}

void KontrolDevice::addMode(unsigned mode, std::shared_ptr<DeviceMode> handler) {
    modes_[mode] = handler;
}

bool KontrolDevice::init() {
    //FIXME: cannot create shared_ptr like this
    //model_->addCallback("pd.kdevice", std::shared_ptr<KontrolDevice>(this));
    for (auto m : modes_) {
        if (m.second != nullptr) m.second->init();
    }
    return true;
}

void KontrolDevice::poll() {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->poll();
}

void KontrolDevice::changePot(unsigned pot, float value) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->changePot(pot, value);
}

void KontrolDevice::changeEncoder(unsigned encoder, float value) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->changeEncoder(encoder, value);
}

void KontrolDevice::encoderButton(unsigned encoder, bool value) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->encoderButton(encoder, value);
}

void KontrolDevice::keyPress(unsigned key, unsigned value) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->keyPress(key, value);
}

void KontrolDevice::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->rack(src, rack);
}

void KontrolDevice::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->module(src, rack, module);
}

void KontrolDevice::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                         const Kontrol::Page &page) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->page(src, rack, module, page);
}

void KontrolDevice::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                          const Kontrol::Parameter &param) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->param(src, rack, module, param);
}

void KontrolDevice::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module,
                            const Kontrol::Parameter &param) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->changed(src, rack, module, param);
}

void KontrolDevice::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const std::string &resType,
                             const std::string &resValue) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->resource(src, rack, resType, resValue);
}

void KontrolDevice::loadModule(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                            const Kontrol::EntityId &moduleId, const std::string &modType) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->loadModule(source, rack, moduleId, modType);
}

void KontrolDevice::midiCC(unsigned num, unsigned value) {
    auto rack = model()->getLocalRack();
    if (rack != nullptr) {
        rack->changeMidiCC(num, value);
    }
}


static t_pd *get_object(const char *s) {
    t_pd *x = gensym(s)->s_thing;
    return x;
}

void KontrolDevice::sendPdMessage(const char *obj, float f) {
    t_pd *sendobj = get_object(obj);
    if (!sendobj) {
        post("KontrolDevice::sendPdMessage to %s failed", obj);
        return;
    }

    t_atom a;
    SETFLOAT(&a, f);
    pd_forwardmess(sendobj, 1, &a);
}
