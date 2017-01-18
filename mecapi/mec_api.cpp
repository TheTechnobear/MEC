#include "mec_api.h"
/////////////////////////////////////////////////////////

#include "mec_prefs.h"
#include "mec_device.h"
#include "mec_log.h"

#include <vector>
#include <memory>
#include <iostream>


/////////////////////////////////////////////////////////
class MecApi_Impl : public IMecCallback {
public:
    MecApi_Impl(const std::string& configFile);
    ~MecApi_Impl();

    void init();
    void process();  // periodically call to process messages

    void subscribe(IMecCallback*);
    void unsubscribe(IMecCallback*);

    //IMecCallback
    virtual void touchOn(int touchId, float note, float x, float y, float z);
    virtual void touchContinue(int touchId, float note, float x, float y, float z);
    virtual void touchOff(int touchId, float note, float x, float y, float z);
    virtual void control(int ctrlId, float v);

private:
    void initDevices();

    std::vector<std::shared_ptr<MecDevice>> devices_;
    std::unique_ptr<MecPreferences> prefs_;
    std::vector<IMecCallback*> callbacks_;
};


//////////////////////////////////////////////////////////
//MecApi
MecApi::MecApi(const std::string& configFile) {
    impl_ = new MecApi_Impl(configFile);
} 
MecApi::~MecApi() {
    delete impl_;
}

void MecApi::init() {
    impl_->init();
}

void MecApi::process() {
    impl_->process();
}

void MecApi::subscribe(IMecCallback* p) {
    impl_->subscribe(p);

}
void MecApi::unsubscribe(IMecCallback* p) {
    impl_->unsubscribe(p);
}

/////////////////////////////////////////////////////////
//MecApi_Impl
MecApi_Impl::MecApi_Impl(const std::string& configFile) {
    prefs_.reset(new MecPreferences(configFile));
} 
MecApi_Impl::~MecApi_Impl() {
    for (std::vector<std::shared_ptr<MecDevice>>::iterator it = devices_.begin() ; it != devices_.end(); ++it) {
        (*it)->deinit();
    }
    devices_.clear();
    prefs_.reset();
}

void MecApi_Impl::init() {

    initDevices();
}

void MecApi_Impl::process() {
    for (std::vector<std::shared_ptr<MecDevice>>::iterator it = devices_.begin() ; it != devices_.end(); ++it) {
        (*it)->process();
    }
}

void MecApi_Impl::subscribe(IMecCallback* p) {
    callbacks_.push_back(p);
}

void MecApi_Impl::unsubscribe(IMecCallback* p) {
    for (std::vector<IMecCallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        if(p==(*it)) {
            callbacks_.erase(it);
            return;
        }
    }
}

void MecApi_Impl::touchOn(int touchId, float note, float x, float y, float z) {
    for (std::vector<IMecCallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->touchOn(touchId,note,x,y,z);
    }
}

void MecApi_Impl::touchContinue(int touchId, float note, float x, float y, float z) {
    for (std::vector<IMecCallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->touchContinue(touchId,note,x,y,z);
    }
}

void MecApi_Impl::touchOff(int touchId, float note, float x, float y, float z) {
    for (std::vector<IMecCallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->touchOff(touchId,note,x,y,z);
    }
}

void MecApi_Impl::control(int ctrlId, float v) {
    for (std::vector<IMecCallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->control(ctrlId,v);
    }
}


/////////////////////////////////////////////////////////

#include "mec_eigenharp.h"
#include "mec_soundplane.h"

void MecApi_Impl::initDevices() {

    // if (prefs_->exists("osc")) {
    //     LOG_1(std::cout   << "osc initialise " << std::endl;)
    // }

    if (prefs_->exists("eigenharp")) {
        LOG_1(std::cout   << "eigenharp initialise " << std::endl;)
        std::shared_ptr<MecDevice> device;
        device.reset(new MecEigenharp(*this));
        if(device->init(prefs_->getSubTree("eigenharp"))) {
            if(device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1(std::cout   << "eigenharp init inactive " << std::endl;)
                device->deinit();
            }
        } else {
            LOG_1(std::cout   << "eigenharp init failed " << std::endl;)
            device->deinit();
        }
    }

    if (prefs_->exists("soundplane")) {
        LOG_1(std::cout   << "soundplane initialise " << std::endl;)
        std::shared_ptr<MecDevice> device;
        device.reset(new MecSoundplane(*this));
        if(device->init(prefs_->getSubTree("soundplane"))) {
            if(device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1(std::cout   << "soundplane init inactive " << std::endl;)
                device->deinit();
            }
        } else {
            LOG_1(std::cout   << "soundplane init failed " << std::endl;)
            device->deinit();
        }
    }

    // if (prefs_->exists("push2")) {
    //     LOG_1(std::cout   << "push2 initialise " << std::endl;)
    // }

    // if (prefs_->exists("midi")) {
    //     LOG_1(std::cout   << "midi initialise " << std::endl;)
    // }
}

