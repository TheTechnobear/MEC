#include "OverlayFactory.h"
#include "XYZPlaneOverlay.h"
#include "GridOverlay.h"

namespace mec {
namespace morph {

std::unique_ptr <OverlayFunction>
OverlayFactory::create(const std::string &overlayName, ISurfaceCallback &surfaceCallback_, ICallback &callback_) {
    std::unique_ptr<OverlayFunction> overlay;
    if (overlayName == "xyzplane") {
        overlay.reset(new XYZPlaneOverlay(overlayName, surfaceCallback_, callback_));
    } else if (overlayName == "grid") {
        overlay.reset(new GridOverlay(overlayName, surfaceCallback_, callback_));
    }
    return overlay;
}

}
}