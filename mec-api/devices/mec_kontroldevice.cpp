#include "mec_kontroldevice.h"

#include "mec_log.h"
#include "mec_prefs.h"

#include "Parameter.h"

namespace mec {

////////////////////////////////////////////////
KontrolDevice::KontrolDevice(ICallback& cb) :
    state_(S_UNCONNECTED),
    active_(false), callback_(cb),
    listenPort_(0), connectPort_(0)  {
    param_model_ = Kontrol::ParameterModel::model();
}

KontrolDevice::~KontrolDevice() {
    deinit();
}



bool KontrolDevice::init(void* arg) {
    Preferences prefs(arg);

    LOG_0("KontrolDevice::init");

    if (active_) {
        deinit();
    }
    active_ = false;
    state_ = S_UNCONNECTED;

    if(prefs.exists("parameter definitions")) {
        std::string file = prefs.getString("parameter definitions","./kontrol-param.json");
        if(!file.empty()) {
            param_model_->loadParameterDefintions(file);
        }
        param_model_->dumpParameters();
    }

    if(prefs.exists("patch settings")) {
        std::string file = prefs.getString("patch settings","./kontrol-patch.json");
        if(!file.empty()) {
            param_model_->loadPatchSettings(file);
        }
        param_model_->dumpPatchSettings();
    }

    connectPort_ = prefs.getInt("connect port", 9000);

    listenPort_ = prefs.getInt("listen port", 9001);
    if (listenPort_ > 0) {
        auto p = std::make_shared<Kontrol::OSCReceiver>(param_model_);
        if (p->listen(listenPort_)) {
            osc_receiver_ = p;
            LOG_0("kontrol device : listening on " << listenPort_);
        }
    }

    if (connectPort_ > 0) {
        std::string host = "127.0.0.1";
        std::string id = "mec.osc:" + host + ":" + std::to_string(connectPort_);
        param_model_->removeCallback(id);
        auto p = std::make_shared<Kontrol::OSCBroadcaster>();
        if (p->connect(host, (unsigned) connectPort_)) {
            osc_broadcaster_ = p;
            param_model_->addCallback(id, osc_broadcaster_);
            LOG_0("kontrol device : connected to " << connectPort_);
        }
    }

    if (osc_broadcaster_ && osc_receiver_) {
        state_ = S_CONNECT_REQUEST;
    }

    active_ = true;

    LOG_0("KontrolDevice::init - complete");
    return active_;
}

bool KontrolDevice::process() {

    // wait till after initialisatioin of all devices before requesting meta data
    if (osc_broadcaster_) {
        switch (state_) {
        case S_UNCONNECTED :
        case S_CONNECTED:
            break;
        case S_CONNECT_REQUEST :
            LOG_0("KontrolDevice::process request callback");
            osc_broadcaster_->requestConnect(listenPort_);
            state_ = S_METADATA_REQUEST;
            break;
        case S_METADATA_REQUEST :
            LOG_0("KontrolDevice::process request meta data");
            osc_broadcaster_->requestMetaData();
            state_ = S_CONNECTED;
            break;
        }
    }
    // return queue_.process(callback_);
    if (osc_receiver_) osc_receiver_->poll();
    return true;
}

void KontrolDevice::deinit() {
    LOG_0("KontrolDevice::deinit");
    if (osc_broadcaster_) osc_broadcaster_->stop();
    if (osc_receiver_) osc_receiver_->stop();

    active_ = false;
}

bool KontrolDevice::isActive() {
    return active_;
}

}



