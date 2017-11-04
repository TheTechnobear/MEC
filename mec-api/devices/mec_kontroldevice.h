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

private:
    enum State {
        S_UNCONNECTED,
        S_CONNECT_REQUEST,
        S_METADATA_REQUEST,
        S_CONNECTED
    } state_;

    ICallback &callback_;
    bool active_;
    MsgQueue queue_;
    unsigned connectPort_;
    unsigned listenPort_;

    std::shared_ptr<Kontrol::KontrolModel> model_;
    std::shared_ptr<Kontrol::OSCReceiver> osc_receiver_;
    std::shared_ptr<Kontrol::OSCBroadcaster> osc_broadcaster_;
};

}
