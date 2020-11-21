#include "mec_api.h"
/////////////////////////////////////////////////////////

#include "mec_prefs.h"
#include "mec_device.h"
#include "mec_log.h"

#if !DISABLE_EIGENHARP
#   include "devices/mec_eigenharp.h"
#endif
#if !DISABLE_SOUNDPLANELITE
#   include "devices/mec_soundplane.h"
#endif
#if !DISABLE_PUSH2
#   include "devices/mec_push2.h"
#endif
#if !DISABLE_OSCDISPLAY
#   include "devices/mec_oscdisplay.h"
#endif
#if !DISABLE_NUI
#   include "devices/mec_nui.h"
#endif

#if !DISABLE_ELECTRAONE
#   include "devices/mec_electraone.h"
#endif

#include "devices/mec_mididevice.h"
#include "devices/mec_osct3d.h"
#include "devices/mec_kontroldevice.h"

namespace mec {

/////////////////////////////////////////////////////////
class MecApi_Impl : public ICallback, public ISurfaceCallback, public IMusicalCallback {
public:
    MecApi_Impl(void *prefs);
    MecApi_Impl(const std::string &configFile);
    ~MecApi_Impl();

    void init();
    void process();  // periodically call to process messages

    void subscribe(ICallback *);
    void unsubscribe(ICallback *);


    void subscribe(ISurfaceCallback *);
    void unsubscribe(ISurfaceCallback *);

    void subscribe(IMusicalCallback *);
    void unsubscribe(IMusicalCallback *);


    //callbacks...
    virtual void touchOn(int touchId, float note, float x, float y, float z);
    virtual void touchContinue(int touchId, float note, float x, float y, float z);
    virtual void touchOff(int touchId, float note, float x, float y, float z);
    virtual void control(int ctrlId, float v);
    virtual void mec_control(int cmd, void *other);

    virtual void touchOn(const Touch &);
    virtual void touchContinue(const Touch &);
    virtual void touchOff(const Touch &);

    virtual void touchOn(const MusicalTouch &);
    virtual void touchContinue(const MusicalTouch &);
    virtual void touchOff(const MusicalTouch &);

private:
    void initDevices();

    std::vector<std::shared_ptr<Device>> devices_;
    std::unique_ptr<Preferences> fileprefs_; // top level prefs on file
    std::unique_ptr<Preferences> prefs_;     // api prefs
    std::vector<ICallback *> callbacks_;
    std::vector<ISurfaceCallback *> surfaces_;
    std::vector<IMusicalCallback *> musicalsurfaces_;
};


//////////////////////////////////////////////////////////
//MecApi
MecApi::MecApi(void *prefs) {
    LOG_1("MecApi::MecApi");
    impl_ = new MecApi_Impl(prefs);
}

MecApi::MecApi(const std::string &configFile) {
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

void MecApi::subscribe(ICallback *p) {
    impl_->subscribe(p);

}

void MecApi::unsubscribe(ICallback *p) {
    impl_->unsubscribe(p);
}

void MecApi::subscribe(ISurfaceCallback *p) {
    impl_->subscribe(p);

}

void MecApi::unsubscribe(ISurfaceCallback *p) {
    impl_->unsubscribe(p);
}

void MecApi::subscribe(IMusicalCallback *p) {
    impl_->subscribe(p);

}

void MecApi::unsubscribe(IMusicalCallback *p) {
    impl_->unsubscribe(p);
}


/////////////////////////////////////////////////////////
//MecApi_Impl
MecApi_Impl::MecApi_Impl(void *prefs) {
    fileprefs_.reset(new Preferences(prefs));
    prefs_.reset(new Preferences(fileprefs_->getSubTree("mec")));
}

MecApi_Impl::MecApi_Impl(const std::string &configFile) {
    fileprefs_.reset(new Preferences(configFile));
    prefs_.reset(new Preferences(fileprefs_->getSubTree("mec")));
}

MecApi_Impl::~MecApi_Impl() {
    LOG_1("MecApi_Impl::~MecApi_Impl");
    for (std::vector<std::shared_ptr<Device>>::iterator it = devices_.begin(); it != devices_.end(); ++it) {
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
    for (std::vector<std::shared_ptr<Device>>::iterator it = devices_.begin(); it != devices_.end(); ++it) {
        (*it)->process();
    }
}

void MecApi_Impl::subscribe(ICallback *p) {
    callbacks_.push_back(p);
}

void MecApi_Impl::unsubscribe(ICallback *p) {
    for (std::vector<ICallback *>::iterator it = callbacks_.begin(); it != callbacks_.end(); ++it) {
        if (p == (*it)) {
            callbacks_.erase(it);
            return;
        }
    }
}

void MecApi_Impl::subscribe(ISurfaceCallback *p) {
    surfaces_.push_back(p);
}

void MecApi_Impl::unsubscribe(ISurfaceCallback *p) {
    for (std::vector<ISurfaceCallback *>::iterator it = surfaces_.begin(); it != surfaces_.end(); ++it) {
        if (p == (*it)) {
            surfaces_.erase(it);
            return;
        }
    }
}

void MecApi_Impl::subscribe(IMusicalCallback *p) {
    musicalsurfaces_.push_back(p);
}

void MecApi_Impl::unsubscribe(IMusicalCallback *p) {
    for (std::vector<IMusicalCallback *>::iterator it = musicalsurfaces_.begin(); it != musicalsurfaces_.end(); ++it) {
        if (p == (*it)) {
            musicalsurfaces_.erase(it);
            return;
        }
    }
}


void MecApi_Impl::touchOn(int touchId, float note, float x, float y, float z) {
    for (std::vector<ICallback *>::iterator it = callbacks_.begin(); it != callbacks_.end(); ++it) {
        (*it)->touchOn(touchId, note, x, y, z);
    }
}

void MecApi_Impl::touchContinue(int touchId, float note, float x, float y, float z) {
    for (std::vector<ICallback *>::iterator it = callbacks_.begin(); it != callbacks_.end(); ++it) {
        (*it)->touchContinue(touchId, note, x, y, z);
    }
}

void MecApi_Impl::touchOff(int touchId, float note, float x, float y, float z) {
    for (std::vector<ICallback *>::iterator it = callbacks_.begin(); it != callbacks_.end(); ++it) {
        (*it)->touchOff(touchId, note, x, y, z);
    }
}

void MecApi_Impl::control(int ctrlId, float v) {
    for (std::vector<ICallback *>::iterator it = callbacks_.begin(); it != callbacks_.end(); ++it) {
        (*it)->control(ctrlId, v);
    }
}

void MecApi_Impl::mec_control(int cmd, void *other) {
    for (std::vector<ICallback *>::iterator it = callbacks_.begin(); it != callbacks_.end(); ++it) {
        (*it)->mec_control(cmd, other);
    }
}


void MecApi_Impl::touchOn(const Touch &t) {
    for (std::vector<ISurfaceCallback *>::iterator it = surfaces_.begin(); it != surfaces_.end(); ++it) {
        (*it)->touchOn(t);
    }
}

void MecApi_Impl::touchContinue(const Touch &t) {
    for (std::vector<ISurfaceCallback *>::iterator it = surfaces_.begin(); it != surfaces_.end(); ++it) {
        (*it)->touchContinue(t);
    }
}

void MecApi_Impl::touchOff(const Touch &t) {
    for (std::vector<ISurfaceCallback *>::iterator it = surfaces_.begin(); it != surfaces_.end(); ++it) {
        (*it)->touchOff(t);
    }
}

void MecApi_Impl::touchOn(const MusicalTouch &t) {
    for (std::vector<IMusicalCallback *>::iterator it = musicalsurfaces_.begin(); it != musicalsurfaces_.end(); ++it) {
        (*it)->touchOn(t);
    }
}

void MecApi_Impl::touchContinue(const MusicalTouch &t) {
    for (std::vector<IMusicalCallback *>::iterator it = musicalsurfaces_.begin(); it != musicalsurfaces_.end(); ++it) {
        (*it)->touchContinue(t);
    }
}

void MecApi_Impl::touchOff(const MusicalTouch &t) {
    for (std::vector<IMusicalCallback *>::iterator it = musicalsurfaces_.begin(); it != musicalsurfaces_.end(); ++it) {
        (*it)->touchOff(t);
    }
}




/////////////////////////////////////////////////////////



void MecApi_Impl::initDevices() {
    if (fileprefs_ == nullptr || prefs_ == nullptr) {
        LOG_1("MecApi_Impl :: invalid preferences file");
        return;
    }

#if !DISABLE_EIGENHARP
    if (prefs_->exists("eigenharp")) {
        LOG_1("eigenharp initialise ");
        std::shared_ptr<Device> device = std::make_shared<Eigenharp>(*this);
        if (device->init(prefs_->getSubTree("eigenharp"))) {
            if (device->isActive()) {
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
#endif

#if !DISABLE_SOUNDPLANELITE
    if (prefs_->exists("soundplane")) {
        LOG_1("soundplane initialise");
        std::shared_ptr<Device> device = std::make_shared<Soundplane>(*this);
        if (device->init(prefs_->getSubTree("soundplane"))) {
            if (device->isActive()) {
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
#endif

#if !DISABLE_PUSH2
    if (prefs_->exists("push2")) {
        LOG_1("push2 initialise ");
        std::shared_ptr<Push2> device = std::make_shared<Push2>(*this);
        Kontrol::KontrolModel::model()->addCallback("push2", device);
        if (device->init(prefs_->getSubTree("push2"))) {
            if (device->isActive()) {
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
#endif


#if !DISABLE_OSCDISPLAY
    if (prefs_->exists("oscdisplay")) {
        LOG_1("oscdisplay initialise ");
        std::shared_ptr<OscDisplay> device = std::make_shared<OscDisplay>();
//        std::shared_ptr<OscDisplay> device = std::make_shared<OscDisplay>(*this);
        Kontrol::KontrolModel::model()->addCallback("oscdisplay", device);
        if (device->init(prefs_->getSubTree("oscdisplay"))) {
            if (device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1("oscdisplay init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("oscdisplay init failed ");
            device->deinit();
        }
    }
#endif


#if !DISABLE_NUI
    if (prefs_->exists("nui")) {
        LOG_1("nui initialise ");
        std::shared_ptr<Nui> device = std::make_shared<Nui>();
        Kontrol::KontrolModel::model()->addCallback("nui", device);
        if (device->init(prefs_->getSubTree("nui"))) {
            if (device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1("nui init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("nui init failed ");
            device->deinit();
        }
    }
#endif


#if !DISABLE_ELECTRAONE
    if (prefs_->exists("electraone")) {
        LOG_1("electraone initialise ");
        std::shared_ptr<ElectraOne> device = std::make_shared<ElectraOne>(*this);
        Kontrol::KontrolModel::model()->addCallback("electraone", device);
        if (device->init(prefs_->getSubTree("electraone"))) {
            if (device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1("electraone init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("electraone init failed ");
            device->deinit();
        }
    }
#endif

    if (prefs_->exists("midi")) {
        LOG_1("midi initialise ");
        std::shared_ptr<Device> device = std::make_shared<MidiDevice>(*this);
        if (device->init(prefs_->getSubTree("midi"))) {
            if (device->isActive()) {
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


    if (prefs_->exists("osct3d")) {
        LOG_1("osct3d initialise ");
        std::shared_ptr<Device> device = std::make_shared<OscT3D>(*this);
        if (device->init(prefs_->getSubTree("osct3d"))) {
            if (device->isActive()) {
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

    if (prefs_->exists("kontrol")) {
        LOG_1("KontrolDevice initialise ");
        std::shared_ptr<Device> device = std::make_shared<KontrolDevice>(*this);
        if (device->init(prefs_->getSubTree("Kontrol"))) {
            if (device->isActive()) {
                devices_.push_back(device);
            } else {
                LOG_1("KontrolDevice init inactive ");
                device->deinit();
            }
        } else {
            LOG_1("KontrolDevice init failed ");
            device->deinit();
        }
    }

}

}
