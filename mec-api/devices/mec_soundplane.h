#ifndef MecSoundplane_H
#define MecSoundplane_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

#include <SPLiteDevice.h>


#include <memory>

namespace mec {


class Soundplane : public Device {

public:
    Soundplane(ICallback &);
    virtual ~Soundplane();
    virtual bool init(void *);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

private:
    ICallback &callback_;
    std::unique_ptr<SPLiteDevice> device_;
    bool active_;
    MsgQueue queue_;
};

}
#endif // MecSoundplane_H