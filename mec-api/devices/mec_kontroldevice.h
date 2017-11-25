#pragma once

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

#include "KontrolModel.h"
#include "OSCBroadcaster.h"
#include "OSCReceiver.h"

#include <memory>
#include <vector>

namespace mec {

class KontrolDevice : public Device {
public:
    KontrolDevice(ICallback &);
    virtual ~KontrolDevice();
    virtual bool init(void *);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    void newClient(const std::string &host, unsigned port);
private:

    ICallback &callback_;
    bool active_;
    MsgQueue queue_;
    unsigned listenPort_;

    std::shared_ptr<Kontrol::KontrolModel> model_;
    std::shared_ptr<Kontrol::OSCReceiver> osc_receiver_;
    std::shared_ptr<Kontrol::OSCBroadcaster> osc_broadcaster_;
    std::chrono::steady_clock::time_point lastPing_;
};

}
