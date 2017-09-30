#pragma once

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

#include "ParameterModel.h"
#include "OSCBroadcaster.h"
#include "OSCReceiver.h"

#include <memory>
#include <vector>

namespace mec {

class Kontrol : public Device {

public:
    Kontrol(ICallback&);
    virtual ~Kontrol();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

private:
    ICallback& callback_;
    bool active_;
    bool requestMetaData_;
    MsgQueue queue_;
    unsigned connectPort_;
    unsigned listenPort_;

    std::shared_ptr<oKontrol::ParameterModel> param_model_;
    std::shared_ptr<oKontrol::OSCReceiver> osc_receiver_;
    std::shared_ptr<oKontrol::OSCBroadcaster> osc_broadcaster_;
};

}
