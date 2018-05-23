#include "mec_kontroldevice.h"

#include "mec_log.h"

namespace mec {


static const unsigned OSC_POLL_MS = 10;

////////////////////////////////////////////////
KontrolDevice::KontrolDevice(ICallback &cb) :
        active_(false), callback_(cb),
        listenPort_(0) {
    model_ = Kontrol::KontrolModel::model();
}

KontrolDevice::~KontrolDevice() {
    deinit();
}


class KontrolDeviceClientHandler : public Kontrol::KontrolCallback {
public:
    KontrolDeviceClientHandler(KontrolDevice &kd) : this_(kd) { ; }

    //Kontrol::KontrolCallback
    void ping(Kontrol::ChangeSource src, const std::string &host, unsigned port, unsigned keepAlive) override {
        this_.newClient(src, host, port, keepAlive);
    }

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override { ; }

    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
              const Kontrol::Page &) override { ; }

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; }

    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override { ; }

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &,
                  const std::string &, const std::string &) override { ; };

    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    KontrolDevice &this_;
};

void *kontroldevice_processor_func(void *pKontrolDevice) {
    KontrolDevice *pThis = static_cast<KontrolDevice *>(pKontrolDevice);
    pThis->processorRun();
    return nullptr;
}


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
    processor_ = std::thread(kontroldevice_processor_func, this);

    LOG_0("KontrolDevice::init - complete");
    return active_;
}

void KontrolDevice::newClient(
        Kontrol::ChangeSource src,
        const std::string &host,
        unsigned port,
        unsigned keepalive) {

    for (auto client : clients_) {
        if (client->isThisHost(host, port)) {
            return;
        }
    }

    std::string id = "client.osc:" + host + ":" + std::to_string(port);

    auto client = std::make_shared<Kontrol::OSCBroadcaster>(src, keepalive, true);
    if (client->connect(host, port)) {
        LOG_0("KontrolDevice::new client " << client->host() << " : " << client->port() << " KA = " << keepalive);
//        client->sendPing(listenPort_);
        client->ping(src, host, port, keepalive);
        clients_.push_back((client));
        model_->addCallback(id, client);
    }
}


void KontrolDevice::processorRun() {
    while (active_) {
        if (osc_receiver_) {
            osc_receiver_->poll();

            static const std::chrono::seconds pingFrequency(5);
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastPing_);
            if (dur >= pingFrequency) {
                lastPing_ = now;
                bool inactive = false;
                for (auto client : clients_) {
                    if (!client->isActive()) {
                        // not received a ping from this client
                        inactive = true;
                    } else {
                        client->sendPing(osc_receiver_->port());
                    }
                }

                // search for inactive clients and remove
                while (inactive) {
                    inactive = false;
                    for (auto it = clients_.begin(); !inactive && it != clients_.end();) {
                        auto client = *it;
                        if (!client->isActive()) {
                            LOG_0("KontrolDevice::Process... remove inactive client " << client->host() << " : "
                                                                                      << client->port());
                            Kontrol::EntityId rackId = Kontrol::Rack::createId(client->host(), client->port());
                            client->stop();
                            clients_.erase(it);
                            model_->deleteRack(Kontrol::CS_LOCAL, rackId);
                            inactive = true;
                        } else {
                            it++;
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(OSC_POLL_MS));
    }
}


bool KontrolDevice::process() {
    // return queue_.process(callback_);
    return true;
}

void KontrolDevice::deinit() {
    LOG_0("KontrolDevice::deinit");
    for (auto client : clients_) {
        client->stop();
    }
    clients_.clear();
    if (osc_receiver_) osc_receiver_->stop();

    active_ = false;
    if (processor_.joinable()) {
        processor_.join();
    }
}

bool KontrolDevice::isActive() {
    return active_;
}

}



