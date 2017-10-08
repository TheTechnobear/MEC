#include <cmath>
#include "../morph_constants.h"
#include "XYZPlaneOverlay.h"
#include "../../../mec_log.h"

namespace mec {
namespace morph {

XYZPlaneOverlay::XYZPlaneOverlay(const std::string name, ISurfaceCallback &surfaceCallback, ICallback &callback)
        : OverlayFunction(name, surfaceCallback, callback) {}

bool XYZPlaneOverlay::init(const Preferences &preferences, const PanelDimensions &dimensions) {

    dimensions_ = dimensions;

    if (preferences.exists("semitones")) {
        semitones_ = preferences.getDouble("semitones");
    } else {
        semitones_ = 18;
        LOG_1("morph::XYZPlaneOverlay::init - property semitones not defined, using default 18");
    }

    if (preferences.exists("baseNote")) {
        baseNote_ = preferences.getDouble("baseNote");
    } else {
        baseNote_ = 32;
        LOG_1("morph::XYZPlaneOverlay::init - property baseNote not defined, using default 32");
    }
    return true;
}

bool XYZPlaneOverlay::processTouches(Touches &touches) {
    const std::vector<std::shared_ptr<TouchWithDeltas>> &newTouches = touches.getNewTouches();
    for (auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
        surfaceCallback_.touchOn(**touchIter);
        // remove as soon as surface support is fully implemented
        callback_.touchOn((*touchIter)->id_, xPosToNote((*touchIter)->x_), normalizeXPos((*touchIter)->x_),
                          normalizeYPos((*touchIter)->y_), normalizeZPos((*touchIter)->z_));
    }
    const std::vector<std::shared_ptr<TouchWithDeltas>> &continuedTouches = touches.getContinuedTouches();
    for (auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
        surfaceCallback_.touchContinue(**touchIter);
        // remove as soon as surface support is fully implemented
        callback_.touchContinue((*touchIter)->id_, xPosToNote((*touchIter)->x_), normalizeXPos((*touchIter)->x_),
                                normalizeYPos((*touchIter)->y_), normalizeZPos((*touchIter)->z_));
    }
    const std::vector<std::shared_ptr<TouchWithDeltas>> &endedTouches = touches.getEndedTouches();
    for (auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
        surfaceCallback_.touchOff(**touchIter);
        // remove as soon as surface support is fully implemented
        callback_.touchOff((*touchIter)->id_, xPosToNote((*touchIter)->x_), normalizeXPos((*touchIter)->x_),
                           normalizeYPos((*touchIter)->y_), normalizeZPos((*touchIter)->z_));
    }
    return true;
}

float XYZPlaneOverlay::xPosToNote(float xPos) {
    return xPos / dimensions_.width * semitones_ + baseNote_ - 0.5;
}

float XYZPlaneOverlay::normalizeXPos(float xPos) {
    return (xPos / dimensions_.width * semitones_);
}

float XYZPlaneOverlay::normalizeYPos(float yPos) {
    return 1.0 - yPos / dimensions_.height;
}

float XYZPlaneOverlay::normalizeZPos(float zPos) {
    float normalizedPressure = zPos / dimensions_.max_pressure;
    if (normalizedPressure < 0.01) {
        normalizedPressure = 0.01;
    }
    return normalizedPressure;
}

}
}