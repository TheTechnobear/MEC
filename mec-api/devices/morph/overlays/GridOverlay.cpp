#include <morph_constants.h>
#include <cmath>
#include "GridOverlay.h"
#include "../../../mec_log.h"

namespace mec {
namespace morph {

GridOverlay::GridOverlay(const std::string &name, mec::ISurfaceCallback &surfaceCallback,
                                     mec::ICallback &callback) : OverlayFunction(name, surfaceCallback, callback) {}

bool GridOverlay::init(const Preferences &preferences) {
    if (preferences.exists("rows")) {
        rows_ = preferences.getInt("rows");
    } else {
        rows_ = 10;
        LOG_1("morph::GridOverlay::init - property rows not defined, using default 10");
    }

    if (preferences.exists("columns")) {
        columns_ = preferences.getInt("columns");
    } else {
        columns_ = 18;
        LOG_1("morph::GridOverlay::init - property columns not defined, using default 18");
    }

    if (preferences.exists("baseNote")) {
        baseNote_ = preferences.getDouble("baseNote");
    } else {
        baseNote_ = 32;
        LOG_1("morph::GridOverlay::init - property baseNote not defined, using default 32");
    }

    if (preferences.exists("semitoneOffsetPerRow")) {
        semitoneOffsetPerRow_ = preferences.getDouble("semitoneOffsetPerRow");
    } else {
        semitoneOffsetPerRow_ = 5.0; // 4th tuning
        LOG_1("morph::GridOverlay::init - property semitoneOffsetPerRow not defined, using default 5.0");
    }

    return true;
}

bool GridOverlay::interpretTouches(const Touches &touches) {
    const std::vector<std::shared_ptr<TouchWithDeltas>> &newTouches = touches.getNewTouches();
    for (auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
        surfaceCallback_.touchOn(**touchIter);
        // remove as soon as surface support is fully implemented
        callback_.touchOn((*touchIter)->id_, xyPosToNote((*touchIter)->x_, (*touchIter)->y_), normalizeXPos((*touchIter)->x_),
                          normalizeYPos((*touchIter)->y_), normalizeZPos((*touchIter)->z_));
    }
    const std::vector<std::shared_ptr<TouchWithDeltas>> &continuedTouches = touches.getContinuedTouches();
    for (auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
        surfaceCallback_.touchContinue(**touchIter);
        // remove as soon as surface support is fully implemented
        callback_.touchContinue((*touchIter)->id_, xyPosToNote((*touchIter)->x_, (*touchIter)->y_), normalizeXPos((*touchIter)->x_),
                                normalizeYPos((*touchIter)->y_), normalizeZPos((*touchIter)->z_));
    }
    const std::vector<std::shared_ptr<TouchWithDeltas>> &endedTouches = touches.getEndedTouches();
    for (auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
        surfaceCallback_.touchOff(**touchIter);
        // remove as soon as surface support is fully implemented
        callback_.touchOff((*touchIter)->id_, xyPosToNote((*touchIter)->x_, (*touchIter)->y_), normalizeXPos((*touchIter)->x_),
                           normalizeYPos((*touchIter)->y_), normalizeZPos((*touchIter)->z_));
    }
    return true;
}

float GridOverlay::xyPosToNote(float xPos, float yPos) {
    float invertedYPos = MEC_MORPH_PANEL_HEIGHT - yPos;
    int rowNumber = floor(invertedYPos / MEC_MORPH_PANEL_HEIGHT * rows_);
    float yBasedNoteOffset = rowNumber * semitoneOffsetPerRow_;
    return xPos / MEC_MORPH_PANEL_WIDTH * 18 //TODO: columns: get panel width for composite panels
           + yBasedNoteOffset
           + baseNote_;
}

float GridOverlay::normalizeXPos(float xPos) {
    return (xPos / MEC_MORPH_PANEL_WIDTH * 18); //TODO: columns
}

float GridOverlay::normalizeYPos(float yPos) {
    float invertedYPos = MEC_MORPH_PANEL_HEIGHT - yPos;
    int rowNumber = floor(invertedYPos / MEC_MORPH_PANEL_HEIGHT * rows_);
    float cellHeight = MEC_MORPH_PANEL_HEIGHT / rows_;
    float yOffsetInCell = (invertedYPos - rowNumber * cellHeight) / cellHeight;
    return yOffsetInCell;
}

float GridOverlay::normalizeZPos(float zPos) {
    float normalizedPressure = zPos / MEC_MORPH_MAX_Z_PRESSURE;
    if (normalizedPressure < 0.01) {
        normalizedPressure = 0.01;
    }
    return normalizedPressure;
}

}
}