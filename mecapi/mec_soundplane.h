#ifndef MecSoundplane_H 
#define MecSoundplane_H

#include "mec_api.h"
#include "mec_device.h"

class SoundplaneModel;
class MLAppState;

#include <memory>

class MecSoundplane : public MecDevice {

public:
    MecSoundplane(MecCallback&);
    virtual ~MecSoundplane();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

private:
	MecCallback& callback_;
    std::unique_ptr<SoundplaneModel> model_;
    std::unique_ptr<MLAppState> modelState_;
    bool active_;
};

#endif // MecSoundplane_H