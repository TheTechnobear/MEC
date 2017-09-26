#ifndef MEC_OVERLAYFACTORY_H
#define MEC_OVERLAYFACTORY_H

#include "OverlayFunction.h"

namespace mec {
namespace morph {

class OverlayFactory {
public:
    static std::unique_ptr <OverlayFunction>
    create(const std::string &overlayName, ISurfaceCallback &surfaceCallback_, ICallback &callback_);
};

}
}

#endif //MEC_OVERLAYFACTORY_H
