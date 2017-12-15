#include "mec_kontroldevice.h"

#include "mec_log.h"

namespace mec {

////////////////////////////////////////////////
KontrolDevice::KontrolDevice(ICallback &cb) :
        active_(false), callback_(cb),
        listenPort_(0){
    model_ = Kontrol::KontrolModel::model();
}

KontrolDevice::~KontrolDevice() {
    deinit();
}


class KontrolDeviceClientHandler : public Kontrol::KontrolCallback {
public:
    KontrolDeviceClientHandler(KontrolDevice &kd) : this_(kd) {;}
    //Kontrol::KontrolCallback
    virtual void ping(const std::string &host, unsigned port) { this_.newClient(host,port);}

    virtual void rack(Kontrol::ParameterSource, const Kontrol::Rack &) { ; }
    virtual void module(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &) { ; }
    virtual void page(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                      const Kontrol::Page &) { ; }
    virtual void param(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                       const Kontrol::Parameter &) { ; }
    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                         const Kontrol::Parameter &) { ; }

    KontrolDevice& this_;
};


bool KontrolDevice::init(void *arg) {
    Preferences prefs(arg);

    LOG_0("KontrolDevice::init");

    if (active_) {
        deinit();
    }
    active_ = false;

    model_->addCallback("clienthandler", std::make_shared<KontrolDeviceClientHandler>(*this));

    listenPort_ = static_cast<unsigned>(prefs.getInt("listen port", 4000));

    if (listenPort_ > 0) {
        auto p = std::make_shared<Kontrol::OSCReceiver>(model_);
        if (p->listen(listenPort_)) {
            osc_receiver_ = p;
            LOG_0("kontrol device : listening on " << listenPort_);
        }
    }

    active_ = true;

    LOG_0("KontrolDevice::init - complete");
    return active_;
}

void KontrolDevice::newClient(const std::string &host, unsigned port) {
    for(auto client : clients_) {
        if (client->isThisHost(host, port)) {
            return;
        }
    }

    std::string id = "client.osc:" + host + ":" + std::to_string(port);
    auto client = std::make_shared<Kontrol::OSCBroadcaster>(true);
    if(client->connect(host,port)) {
        clients_.push_back((client));
        model_->addCallback(id,client);
    }
}

bool KontrolDevice::process() {

    if (osc_receiver_) {
        osc_receiver_->poll();

        static const std::chrono::seconds pingFrequency(5);
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastPing_);
        if(dur > pingFrequency) {
            lastPing_ = now;
            for(auto client : clients_) {
                if(!client->isActive()) {
                    // not received a ping from this client
                    LOG_0("KontrolDevice::Process... inactive client");
                }
                client->sendPing(osc_receiver_->port());
            }
        }

    }

    // return queue_.process(callback_);
    return true;
}

void KontrolDevice::deinit() {
    LOG_0("KontrolDevice::deinit");
    for(auto client : clients_) {
        client->stop();
    }
    clients_.clear();
    if (osc_receiver_) osc_receiver_->stop();

    active_ = false;
}

bool KontrolDevice::isActive() {
    return active_;
}

}



