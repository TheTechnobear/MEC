#include "mec_api.h"
/////////////////////////////////////////////////////////

#include "mec_prefs.h"
#include "mec_device.h"
#include "mec_log.h"

#include <vector>
#include <memory>


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
    virtual void mec_control(int cmd, void* other);

private:
    void initDevices();

    std::vector<std::shared_ptr<MecDevice>> devices_;
    std::unique_ptr<MecPreferences> prefs_;
    std::vector<IMecCallback*> callbacks_;
};


//////////////////////////////////////////////////////////
//MecApi
MecApi::MecApi(const std::string& configFile) {
    LOG_1("MecApi::MecApi");
    impl_ = new MecApi_Impl(configFile);
} 
MecApi::~MecApi() {
    LOG_1("MecApi::~MecApi");
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
    LOG_1("MecApi_Impl::~MecApi_Impl");
    for (std::vector<std::shared_ptr<MecDevice>>::iterator it = devices_.begin() ; it != devices_.end(); ++it) {
        LOG_1("device deinit ");
        (*it)->deinit();
    }
    devices_.clear();
    LOG_1("devices cleared");
    prefs_.reset();
}

void MecApi_Impl::init() {
    LOG_1("MecApi_Impl::init");
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

void MecApi_Impl::mec_control(int cmd, void* other) {
    for (std::vector<IMecCallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->mec_control(cmd,other);
    }
}


/////////////////////////////////////////////////////////

#include "devices/mec_eigenharp.h"
#include "devices/mec_soundplane.h"
#include "devices/mec_midi.h"
#include "devices/mec_push2.h"
#include "devices/mec_osct3d.h"

void MecApi_Impl::initDevices() {

    // if (prefs_->exists("osc")) {
    //     LOG_1(std::cout   << "osc initialise " << std::endl;)
    // }

    if (prefs_->exists("eigenharp")) {
        LOG_1("eigenharp initialise ");
        std::shared_ptr<MecDevice> device;
        device.reset(new MecEigenharp(*this));
        if(device->init(prefs_->getSubTree("eigenharp"))) {
            if(device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1("eigenharp init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("eigenharp init failed ");
            device->deinit();
        }
    }

    if (prefs_->exists("soundplane")) {
        LOG_1("soundplane initialise");
        std::shared_ptr<MecDevice> device;
        device.reset(new MecSoundplane(*this));
        if(device->init(prefs_->getSubTree("soundplane"))) {
            if(device->isActive()) {
                devices_.push_back(device);
                LOG_1("soundplane init active ");
            } else {
                LOG_1("soundplane init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("soundplane init failed ");
            device->deinit();
        }
    }

    if (prefs_->exists("midi")) {
        LOG_1("midi initialise ");
        std::shared_ptr<MecDevice> device;
        device.reset(new MecMidi(*this));
        if(device->init(prefs_->getSubTree("midi"))) {
            if(device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1("midi init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("midi init failed ");
            device->deinit();
        }
    }

    if (prefs_->exists("push2")) {
        LOG_1("push2 initialise ");
        std::shared_ptr<MecDevice> device;
        device.reset(new MecPush2(*this));
        if(device->init(prefs_->getSubTree("push2"))) {
            if(device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1("push2 init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("push2 init failed ");
            device->deinit();
        }
    }

    if (prefs_->exists("osct3d")) {
        LOG_1("osct3d initialise ");
        std::shared_ptr<MecDevice> device;
        device.reset(new MecOscT3D(*this));
        if(device->init(prefs_->getSubTree("osct3d"))) {
            if(device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1("osct3d init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("osct3d init failed ");
            device->deinit();
        }
    }


}

