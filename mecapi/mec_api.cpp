#include "mec_api.h"
/////////////////////////////////////////////////////////

#include "mec_prefs.h"
#include "mec_device.h"
#include "mec_log.h"

#include <vector>
#include <memory>
#include <iostream>


/////////////////////////////////////////////////////////
class MecApi_Impl : public MecCallback {
public:
    MecApi_Impl(const std::string& configFile);
    ~MecApi_Impl();

    void init();
    void process();  // periodically call to process messages

    void subscribe(MecCallback*);
    void unsubscribe(MecCallback*);

    //MecCallback
    virtual void touchOn(int touchId, int note, float x, float y, float z);
    virtual void touchContinue(int touchId, int note, float x, float y, float z);
    virtual void touchOff(int touchId, int note, float x, float y, float z);
    virtual void control(int ctrlId, float v);

private:
    void initDevices();

    std::vector<std::shared_ptr<MecDevice>> devices_;
    std::unique_ptr<MecPreferences> prefs_;
    MecCallback* callback_;
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

void MecApi::subscribe(MecCallback* p) {
    impl_->subscribe(p);

}
void MecApi::unsubscribe(MecCallback* p) {
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

void MecApi_Impl::subscribe(MecCallback* p) {
    callback_ = p;
}

void MecApi_Impl::unsubscribe(MecCallback* p) {
    if(callback_ == p ) {
        callback_ = NULL;
    }
}

void MecApi_Impl::touchOn(int touchId, int note, float x, float y, float z) {
    callback_->touchOn(touchId,note,x,y,z);
}

void MecApi_Impl::touchContinue(int touchId, int note, float x, float y, float z) {
    callback_->touchContinue(touchId,note,x,y,z);

}

void MecApi_Impl::touchOff(int touchId, int note, float x, float y, float z) {
    callback_->touchOff(touchId,note,x,y,z);
}

void MecApi_Impl::control(int ctrlId, float v) {
    callback_->control(ctrlId,v);
}


/////////////////////////////////////////////////////////

#include "mec_eigenharp.h"

void MecApi_Impl::initDevices() {

    // if (prefs_->exists("osc")) {
    //     LOG_1(std::cout   << "osc initialise " << std::endl;)
    // }

    if (prefs_->exists("eigenharp")) {
        LOG_1(std::cout   << "eigenharp initialise " << std::endl;)
        std::shared_ptr<MecDevice> device;
        device.reset(new MecEigenharp(*this));
        if(device->init(prefs_->getSubTree("eigenharp")) && device->isActive()) {
            devices_.push_back(device);
        } else {
            device->deinit();
        }
    }

    // if (prefs_->exists("soundplane")) {
    //     LOG_1(std::cout   << "soundplane initialise " << std::endl;)
    // }

    // if (prefs_->exists("push2")) {
    //     LOG_1(std::cout   << "push2 initialise " << std::endl;)
    // }

    // if (prefs_->exists("midi")) {
    //     LOG_1(std::cout   << "midi initialise " << std::endl;)
    // }
}

