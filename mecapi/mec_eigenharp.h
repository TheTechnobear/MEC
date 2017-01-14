#ifndef MecEigenharp_H 
#define MecEigenharp_H

#include "mec_device.h"

#include <eigenfreed/eigenfreed.h>

class MecEigenharp : public MecDevice {

public:
    MecEigenharp();
    virtual ~MecEigenharp();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

private:
    std::unique_ptr<EigenApi::Eigenharp> eigenD_;
    bool active_;
};

#endif // MecEigenharp