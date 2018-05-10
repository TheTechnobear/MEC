#include "KontrolDevice.h"

#include "../../m_pd.h"

//#include <iostream>

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
    midiLearn(false);
    modulationLearn(false);
    lastParamId_ = "";
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
    if (currentModuleId_.empty()) {
        currentRackId_ = rack.id();
        currentModule(module.id());
    }
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
    // set last param for midi learn
    if ((midiLearnActive_ || modulationLearnActive_)
        && rack.id() == currentRackId_
        && module.id() == currentModuleId_) {
        lastParamId_ = param.id();
    }

    auto m = modes_[currentMode_];
    if (m != nullptr) m->changed(src, rack, module, param);
}

void KontrolDevice::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const std::string &resType,
                             const std::string &resValue) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->resource(src, rack, resType, resValue);
}

void KontrolDevice::loadModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                               const Kontrol::EntityId &moduleId, const std::string &modType) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->loadModule(src, rack, moduleId, modType);
}

void KontrolDevice::midiCC(unsigned num, unsigned value) {
    if (midiLearnActive_) {
        if (!lastParamId_.empty()) {
            auto rack = model()->getRack(currentRackId_);
            if (rack != nullptr) {
                if (value > 0) {
                    rack->addMidiCCMapping(num, currentModuleId_, lastParamId_);
                    lastParamId_ = "";
                } else {
                    //std::cerr << "midiCC unlearn" << num << " " << lastParamId_ << std::endl;
                    rack->removeMidiCCMapping(num, currentModuleId_, lastParamId_);
                    lastParamId_ = "";
                }
            }
        }
    }

    auto rack = model()->getLocalRack();
    if (rack != nullptr) {
        rack->changeMidiCC(num, value);
    }
}

void KontrolDevice::digital(unsigned bus, bool value) {
//    auto rack = model()->getLocalRack();
//    if (rack != nullptr) {
//        rack->changeDigital(bus, value);
//    }
}

void KontrolDevice::analog(unsigned bus, float value) {
    if (modulationLearnActive_) {
        if (!lastParamId_.empty()) {
            auto rack = model()->getRack(currentRackId_);
            if (rack != nullptr) {
                if (value > 0.1) {
                    //std::cerr << "modulation learn" << bus << " " << lastParamId_ << std::endl;
                    rack->addModulationMapping(bus, currentModuleId_, lastParamId_);
                    lastParamId_ = "";
                } else {
                    //std::cerr << "modulation unlearn" << bus << " " << lastParamId_ << std::endl;
                    rack->removeModulationMapping(bus, currentModuleId_, lastParamId_);
                    lastParamId_ = "";
                }
            }
        }
    }

    auto rack = model()->getLocalRack();
    // update param model
    if (rack != nullptr) {
        rack->changeModulation(bus, value);
    }
}


void KontrolDevice::midiLearn(bool b) {
    lastParamId_ = "";
    modulationLearnActive_ = false;
    midiLearnActive_ = b;
}

void KontrolDevice::modulationLearn(bool b) {
    lastParamId_ = "";
    midiLearnActive_ = false;
    modulationLearnActive_ = b;
}



void KontrolDevice::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    auto m = modes_[currentMode_];
    if (m != nullptr) m->activeModule(src, rack, module);
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

void KontrolDevice::currentModule(const Kontrol::EntityId &moduleId) {
    currentModuleId_ = moduleId;
    model()->activeModule(Kontrol::CS_LOCAL, currentRackId_, currentModuleId_);
}
