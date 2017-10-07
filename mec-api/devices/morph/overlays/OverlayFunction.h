#ifndef MEC_OVERLAYFUNCTION_H
#define MEC_OVERLAYFUNCTION_H

#include "../../../mec_api.h"
#include "../../../mec_prefs.h"
#include "../touches/Touches.h"
#include "../panels/PanelDimensions.h"

namespace mec {
namespace morph {

class OverlayFunction {
public:
    OverlayFunction(const std::string name, ISurfaceCallback &surfaceCallback, ICallback &callback)
            : name_(name), surfaceCallback_(surfaceCallback), callback_(callback) {}

    virtual bool init(const Preferences &preferences, const PanelDimensions& dimensions) = 0;

    virtual bool processTouches(Touches &touches) = 0;

    const std::string &getName() const {
        return name_;
    }

protected:
    std::string name_;
    ISurfaceCallback &surfaceCallback_;
    ICallback &callback_;
};

}
}

#endif //MEC_OVERLAYFUNCTION_H
