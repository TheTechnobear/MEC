#ifndef MEC_XYZPLANEOVERLAY_H
#define MEC_XYZPLANEOVERLAY_H

#include <panels/PanelDimensions.h>
#include "OverlayFunction.h"

namespace mec {
namespace morph {

class XYZPlaneOverlay : public OverlayFunction {
public:
    XYZPlaneOverlay(const std::string name, ISurfaceCallback &surfaceCallback, ICallback &callback);

    virtual bool init(const Preferences &preferences, const PanelDimensions &dimensions);

    virtual bool interpretTouches(const Touches &touches);

private:
    float semitones_;
    float baseNote_;
    PanelDimensions dimensions_;

    float xPosToNote(float xPos);

    float normalizeXPos(float xPos);

    float normalizeYPos(float yPos);

    float normalizeZPos(float zPos);
};

}
}

#endif //MEC_XYZPLANEOVERLAY_H
