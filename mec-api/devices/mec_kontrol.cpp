#include "mec_kontrol.h"

#include "../mec_log.h"
#include "../mec_prefs.h"

#include "Parameter.h"

namespace mec {

////////////////////////////////////////////////
Kontrol::Kontrol(ICallback& cb) :
    state_(S_UNCONNECTED),
    active_(false), callback_(cb),
    listenPort_(0), connectPort_(0)  {
    param_model_ = oKontrol::ParameterModel::model();
}

Kontrol::~Kontrol() {
    deinit();
}



bool Kontrol::init(void* arg) {
    Preferences prefs(arg);

    if (active_) {
        deinit();
    }
    active_ = false;
    state_ = S_UNCONNECTED;

    connectPort_ = prefs.getInt("connectPort", 9000);

    listenPort_ = prefs.getInt("listenPort", 9001);
    if (listenPort_ > 0) {
        auto p = std::make_shared<oKontrol::OSCReceiver>(param_model_);
        if (p->listen(listenPort_)) {
            osc_receiver_ = p;
        }
    }

    if (connectPort_ > 0) {
        std::string host = "127.0.0.1";
        std::string id = "mec.osc:" + host + ":" + std::to_string(connectPort_);
        param_model_->removeCallback(id);
        auto p = std::make_shared<oKontrol::OSCBroadcaster>();
        if (p->connect(host, (unsigned) connectPort_)) {
            osc_broadcaster_ = p;
            param_model_->addCallback(id, osc_broadcaster_);
        }
    }

    if (osc_broadcaster_ && osc_receiver_) {
        state_ = S_CONNECT_REQUEST;
    }

    active_ = true;

    LOG_0("Kontrol::init - complete");
    return active_;
}

bool Kontrol::process() {

    // wait till after initialisatioin of all devices before requesting meta data
    if (osc_broadcaster_) {
        switch (state_) {
        case S_UNCONNECTED :
        case S_CONNECTED:
            break;
        case S_CONNECT_REQUEST :
            LOG_0("Kontrol::process request callback");
            osc_broadcaster_->requestConnect(listenPort_);
            state_ = S_METADATA_REQUEST;
            break;
        case S_METADATA_REQUEST :
            LOG_0("Kontrol::process request meta data");
            osc_broadcaster_->requestMetaData();
            state_ = S_CONNECTED;
            break;
        }
    }
    // return queue_.process(callback_);
    if (osc_receiver_) osc_receiver_->poll();
    return true;
}

void Kontrol::deinit() {
    LOG_0("Kontrol::deinit");
    if (osc_broadcaster_) osc_broadcaster_->stop();
    if (osc_receiver_) osc_receiver_->stop();

    active_ = false;
}

bool Kontrol::isActive() {
    return active_;
}

}



