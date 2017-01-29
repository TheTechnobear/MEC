#ifndef MecOscT3D_H 
#define MecOscT3D_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

class SoundplaneModel;
class MLAppState;

#include <memory>

class MecOscT3D : public MecDevice {

public:
    MecOscT3D(IMecCallback&);
    virtual ~MecOscT3D();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

private:
	IMecCallback& callback_;
    bool active_;
    MecMsgQueue queue_;
};

#endif // MecOscT3D_H