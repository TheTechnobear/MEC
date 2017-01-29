#ifndef MecOscT3D_H 
#define MecOscT3D_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"


#include <memory>
#include <thread>

class UdpListeningReceiveSocket;

class MecOscT3D : public MecDevice {

public:
    MecOscT3D(IMecCallback&);
    virtual ~MecOscT3D();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    void listenProc();

private:
	IMecCallback& callback_;
    bool active_;
    MecMsgQueue queue_;
    std::unique_ptr<UdpListeningReceiveSocket> socket_;
    std::thread listenThread_;

    unsigned int port_;
};

#endif // MecOscT3D_H