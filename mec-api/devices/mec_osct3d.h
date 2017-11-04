#ifndef MecOscT3D_H
#define MecOscT3D_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"


#include <memory>
#include <thread>

class UdpListeningReceiveSocket;

namespace mec {

class OscT3D : public Device {

public:
    OscT3D(ICallback &);
    virtual ~OscT3D();
    virtual bool init(void *);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    void listenProc();

private:
    ICallback &callback_;
    bool active_;
    MsgQueue queue_;
    std::unique_ptr<UdpListeningReceiveSocket> socket_;
    std::thread listenThread_;

    unsigned int port_;
};

}

#endif // MecOscT3D_H