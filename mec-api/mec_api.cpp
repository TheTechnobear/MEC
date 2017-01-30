#include "mec_api.h"
/////////////////////////////////////////////////////////

#include "mec_prefs.h"
#include "mec_device.h"
#include "mec_log.h"


#include "devices/mec_eigenharp.h"
#include "devices/mec_soundplane.h"
#include "devices/mec_midi.h"
#include "devices/mec_push2.h"
#include "devices/mec_osct3d.h"

#include <vector>
#include <memory>

namespace mec {

/////////////////////////////////////////////////////////
class MecApi_Impl : public ICallback {
public:
    MecApi_Impl(const std::string& configFile);
    ~MecApi_Impl();

    void init();
    void process();  // periodically call to process messages

    void subscribe(ICallback*);
    void unsubscribe(ICallback*);

    //ICallback
    virtual void touchOn(int touchId, float note, float x, float y, float z);
    virtual void touchContinue(int touchId, float note, float x, float y, float z);
    virtual void touchOff(int touchId, float note, float x, float y, float z);
    virtual void control(int ctrlId, float v);
    virtual void mec_control(int cmd, void* other);

private:
    void initDevices();

    std::vector<std::shared_ptr<Device>> devices_;
    std::unique_ptr<Preferences> fileprefs_; // top level prefs on file
    std::unique_ptr<Preferences> prefs_;     // api prefs
    std::vector<ICallback*> callbacks_;
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

void MecApi::subscribe(ICallback* p) {
    impl_->subscribe(p);

}
void MecApi::unsubscribe(ICallback* p) {
    impl_->unsubscribe(p);
}

/////////////////////////////////////////////////////////
//MecApi_Impl
MecApi_Impl::MecApi_Impl(const std::string& configFile) {
    fileprefs_.reset(new Preferences(configFile));
    prefs_.reset(new Preferences(fileprefs_->getSubTree("mec")));
} 

MecApi_Impl::~MecApi_Impl() {
    LOG_1("MecApi_Impl::~MecApi_Impl");
    for (std::vector<std::shared_ptr<Device>>::iterator it = devices_.begin() ; it != devices_.end(); ++it) {
        LOG_1("device deinit ");
        (*it)->deinit();
    }
    devices_.clear();
    LOG_1("devices cleared");
    prefs_.reset();
    fileprefs_.reset();
}

void MecApi_Impl::init() {
    LOG_1("MecApi_Impl::init");
    initDevices();
}

void MecApi_Impl::process() {
    for (std::vector<std::shared_ptr<Device>>::iterator it = devices_.begin() ; it != devices_.end(); ++it) {
        (*it)->process();
    }
}

void MecApi_Impl::subscribe(ICallback* p) {
    callbacks_.push_back(p);
}

void MecApi_Impl::unsubscribe(ICallback* p) {
    for (std::vector<ICallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        if(p==(*it)) {
            callbacks_.erase(it);
            return;
        }
    }
}

void MecApi_Impl::touchOn(int touchId, float note, float x, float y, float z) {
    for (std::vector<ICallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->touchOn(touchId,note,x,y,z);
    }
}

void MecApi_Impl::touchContinue(int touchId, float note, float x, float y, float z) {
    for (std::vector<ICallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->touchContinue(touchId,note,x,y,z);
    }
}

void MecApi_Impl::touchOff(int touchId, float note, float x, float y, float z) {
    for (std::vector<ICallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->touchOff(touchId,note,x,y,z);
    }
}

void MecApi_Impl::control(int ctrlId, float v) {
    for (std::vector<ICallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->control(ctrlId,v);
    }
}

void MecApi_Impl::mec_control(int cmd, void* other) {
    for (std::vector<ICallback*>::iterator it = callbacks_.begin() ; it != callbacks_.end(); ++it) {
        (*it)->mec_control(cmd,other);
    }
}


/////////////////////////////////////////////////////////



void MecApi_Impl::initDevices() {
    if(fileprefs_==nullptr || prefs_==nullptr) {
        LOG_1("MecApi_Impl :: invalid preferences file");
        return;
    } 

    if (prefs_->exists("eigenharp")) {
        LOG_1("eigenharp initialise ");
        std::shared_ptr<Device> device;
        device.reset(new mec::Eigenharp(*this));
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
        std::shared_ptr<Device> device;
        device.reset(new Soundplane(*this));
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
        std::shared_ptr<Device> device;
        device.reset(new MidiDevice(*this));
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
        std::shared_ptr<Device> device;
        device.reset(new Push2(*this));
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
        std::shared_ptr<Device> device;
        device.reset(new OscT3D(*this));
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

}
