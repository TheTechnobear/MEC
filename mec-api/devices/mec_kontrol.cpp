#include "mec_kontrol.h"

#include "../mec_log.h"
#include "../mec_prefs.h"

#include "Parameter.h"

namespace mec {

////////////////////////////////////////////////
Kontrol::Kontrol(ICallback& cb) :
    active_(false), callback_(cb), 
    listenPort_(0), connectPort_(0), 
    requestMetaData_(true) {
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

    connectPort_ = prefs.getInt("connectPort", 9000);

    if (connectPort_ > 0) {
        auto p = std::make_shared<oKontrol::OSCBroadcaster>();
        if (p->connect("localhost", (unsigned) connectPort_)) {
            osc_broadcaster_ = p;
            param_model_->addCallback(osc_broadcaster_);
        }
    }


    listenPort_ = prefs.getInt("listenPort", 9001);
    if (listenPort_ > 0) {
        auto p = std::make_shared<oKontrol::OSCReceiver>(param_model_);
        if (p->listen(listenPort_)) {
            osc_receiver_ = p;
        }
    }

    active_ = true;
    requestMetaData_ = true;

    LOG_0("Kontrol::init - complete");
    return active_;
}

bool Kontrol::process() {

    // wait till after initialisatioin of all devices before requesting meta data
    if(osc_broadcaster_ && requestMetaData_) {
        osc_broadcaster_->requestMetaData();
        requestMetaData_ = false;
    }
    // return queue_.process(callback_);
    if(osc_receiver_) osc_receiver_->poll();
    return true;
}

void Kontrol::deinit() {
    LOG_0("Kontrol::deinit");
    if(osc_broadcaster_) osc_broadcaster_->stop();
    if(osc_receiver_) osc_receiver_->stop();

    active_ = false;
}

bool Kontrol::isActive() {
    return active_;
}

}



