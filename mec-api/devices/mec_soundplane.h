#ifndef MecSoundplane_H 
#define MecSoundplane_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

class SoundplaneModel;
class MLAppState;

#include <memory>

class MecSoundplane : public MecDevice {

public:
    MecSoundplane(IMecCallback&);
    virtual ~MecSoundplane();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

private:
	IMecCallback& callback_;
    std::unique_ptr<SoundplaneModel> model_;
    std::unique_ptr<MLAppState> modelState_;
    bool active_;
    MecMsgQueue queue_;
};

#endif // MecSoundplane_H