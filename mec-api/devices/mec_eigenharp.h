#ifndef MECEigenharp_H
#define MECEigenharp_H

#include "../mec_api.h"
#include "../mec_device.h"

#include <eigenfreed/eigenfreed.h>
#include <memory>

namespace mec {
class Eigenharp : public Device {

public:
    Eigenharp(ICallback&);
    virtual ~Eigenharp();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

private:
    ICallback& callback_;
    std::unique_ptr<EigenApi::Eigenharp> eigenD_;
    bool active_;
    long minPollTime_;
};

}

#endif // MECEigenharp_H
