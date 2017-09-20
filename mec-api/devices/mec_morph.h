#ifndef MEC_MORPH_H
#define MEC_MORPH_H

#include "../mec_device.h"
#include "../mec_api.h"

namespace mec {

namespace morph {
    class Impl;
}

class Morph  : public Device {

public:
    Morph(ISurfaceCallback &surfaceCallback, ICallback &callback);

    virtual ~Morph();

    virtual bool init(void *);

    virtual bool process();

    virtual void deinit();

    virtual bool isActive();

private:
    std::unique_ptr <morph::Impl> morphImpl_;
};

}


#endif //MEC_MORPH_H
