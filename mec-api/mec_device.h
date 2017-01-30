#ifndef MEC_DEVICE_H
#define MEC_DEVICE_H

#include "mec_prefs.h"

namespace mec {

class Device {
public:
    virtual ~Device() {};
    virtual bool init(void*)=0;
    virtual bool process()=0 ;
    virtual void deinit()=0;
    virtual bool isActive()=0;
};

}

#endif //MEC_DEVICE