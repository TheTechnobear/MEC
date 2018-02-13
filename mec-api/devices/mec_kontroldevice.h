#pragma once

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

#include "KontrolModel.h"
#include "OSCBroadcaster.h"
#include "OSCReceiver.h"

#include <memory>
#include <vector>
#include <chrono>
#include <thread>

namespace mec {

class KontrolDevice : public Device {
public:
    KontrolDevice(ICallback &);
    virtual ~KontrolDevice();
    virtual bool init(void *);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    void newClient(Kontrol::ChangeSource src, const std::string &host, unsigned port, unsigned keepalive);
    void processorRun();
private:

    ICallback &callback_;
    bool active_;
    MsgQueue queue_;
    unsigned listenPort_;

    std::shared_ptr<Kontrol::KontrolModel> model_;
    std::shared_ptr<Kontrol::OSCReceiver> osc_receiver_;
    std::chrono::steady_clock::time_point lastPing_;
    std::vector<std::shared_ptr<Kontrol::OSCBroadcaster> > clients_;
    std::thread processor_;
};

} //namespace

