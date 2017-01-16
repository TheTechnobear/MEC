#ifndef MecEigenharp_H 
#define MecEigenharp_H

#include "mec_api.h"
#include "mec_device.h"

#include <eigenfreed/eigenfreed.h>
#include <memory>

class MecEigenharp : public MecDevice {

public:
    MecEigenharp(MecCallback&);
    virtual ~MecEigenharp();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

private:
	MecCallback& callback_;
    std::unique_ptr<EigenApi::Eigenharp> eigenD_;
    bool active_;
};

#endif // MecEigenharp