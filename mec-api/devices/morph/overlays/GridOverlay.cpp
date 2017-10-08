#include <morph_constants.h>
#include <cmath>
#include "GridOverlay.h"
#include "../../../mec_log.h"

namespace mec {
namespace morph {

GridOverlay::GridOverlay(const std::string &name, mec::ISurfaceCallback &surfaceCallback,
                                     mec::ICallback &callback) : OverlayFunction(name, surfaceCallback, callback) {}

bool GridOverlay::init(const Preferences &preferences, const PanelDimensions &dimensions) {

    dimensions_ = dimensions;

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

    if (preferences.exists("semitones")) {
        semitones_ = preferences.getInt("semitones");
    } else {
        semitones_ = 18;
        LOG_1("morph::GridOverlay::init - property semitones not defined, using default 18");
    }

    if (preferences.exists("baseNote")) {
        baseNote_ = preferences.getDouble("baseNote");
    } else {
        baseNote_ = 32;
        LOG_1("morph::GridOverlay::init - property baseNote not defined, using default 32");
    }

    float semitoneOffsetForAllRows;
    if (preferences.exists("semitoneOffsetForAllRows")) {
        semitoneOffsetForAllRows = preferences.getDouble("semitoneOffsetForAllRows");
    } else {
        semitoneOffsetForAllRows = 5.0; // 4th tuning
        LOG_1("morph::GridOverlay::init - property semitoneOffsetForAllRows not defined, using default 5.0");
    }
    semitoneOffsetForRow_.push_back(0.0);
    for(int i = 1; i < rows_; ++i) {
        semitoneOffsetForRow_.push_back(semitoneOffsetForAllRows);
    }

    for(int i = 0; i < rows_; ++i) {
        if(preferences.exists("semitoneOffsetForRow" + std::to_string(i + 1))) {
            semitoneOffsetForRow_[i] = preferences.getDouble("semitoneOffsetForRow" + std::to_string(i + 1));
        }
    }

    for(int i = 0; i < rows_; ++i) {
        xOffsetForRow_.push_back(0.0);
        if(preferences.exists("xOffsetForRow" + std::to_string(i + 1))) {
            xOffsetForRow_[i] = preferences.getDouble("xOffsetForRow" + std::to_string(i + 1));
        } else {
            LOG_1("morph::GridOverlay::init - property xOffsetForRow" << std::to_string(i + 1) << " not defined, using default 0.0");
        }
    }

    if (preferences.exists("roundNoteOnPitch")) {
        roundNoteOnPitch_ = preferences.getBool("roundNoteOnPitch");
    } else {
        roundNoteOnPitch_ = true;
        LOG_1("morph::GridOverlay::init - property roundNoteOnPitch not defined, using default true");
    }

    if (preferences.exists("roundNotePitchWhenNotMoving")) {
        roundNotePitchWhenNotMoving_ = preferences.getBool("roundNotePitchWhenNotMoving");
    } else {
        roundNotePitchWhenNotMoving_ = true;
        LOG_1("morph::GridOverlay::init - property roundNotePitchWhenNotMoving not defined, using default true");
    }

    cellWidth_ = dimensions.width / columns_;
    cellHeight_ = dimensions.height / rows_;

    return true;
}

bool GridOverlay::processTouches(Touches &touches) {
    const std::vector<std::shared_ptr<TouchWithDeltas>> &newTouches = touches.getNewTouches();
    TouchWithDeltas quantizedTouch;
    for (auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
        TouchWithDeltas *touch;
        if(roundNoteOnPitch_) {
            Rectangle currentCell;
            getCurrentCell(currentCell, **touchIter);
            quantizer_.quantizeNewTouch(quantizedTouch, **touchIter, currentCell);
            touch = &quantizedTouch;
        } else {
            touch = &**touchIter;
        }
        surfaceCallback_.touchOn(*touch);
        // remove as soon as surface support is fully implemented
        callback_.touchOn(touch->id_, floor(xyPosToNote(touch->x_, touch->y_)), normalizeXPos(touch->x_),
                          normalizeYPos(touch->y_), normalizeZPos(touch->z_));
    }
    const std::vector<std::shared_ptr<TouchWithDeltas>> &continuedTouches = touches.getContinuedTouches();
    for (auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
        Rectangle currentCell;
        getCurrentCell(currentCell, **touchIter);
        TouchWithDeltas quantizedTouch;
        quantizer_.quantizeContinuedTouch(quantizedTouch, **touchIter, currentCell, roundNotePitchWhenNotMoving_);
        surfaceCallback_.touchContinue(quantizedTouch);
        // remove as soon as surface support is fully implemented
        callback_.touchContinue(quantizedTouch.id_, xyPosToNote(quantizedTouch.x_, quantizedTouch.y_), normalizeXPos(quantizedTouch.x_),
                                normalizeYPos(quantizedTouch.y_), normalizeZPos(quantizedTouch.z_));
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

void GridOverlay::getCurrentCell(Rectangle &currentCell, const Touch &touch) {
    float invertedYPos = dimensions_.height - touch.y_;
    int rowNumber = floor(invertedYPos / dimensions_.height * (float)rows_);
    currentCell.x = (floor(touch.x_ / cellWidth_ + xOffsetForRow_[rowNumber]) - xOffsetForRow_[rowNumber]) * cellWidth_;
    currentCell.y = floor(touch.y_ / cellHeight_) * cellHeight_;
    currentCell.width = cellWidth_;
    currentCell.height = cellHeight_;
    LOG_3("mec::GridOverlay::getCurrentCell x:" << currentCell.x << ",y:" << currentCell.y << ",offset:" << xOffsetForRow_[rowNumber] << ",row:" << rowNumber
    << ",panelHeight:" << dimensions_.height << ",touchY:" << touch.y_ << ",rows:" << rows_);
}

float GridOverlay::xyPosToNote(float xPos, float yPos) {
    float invertedYPos = dimensions_.height - yPos;
    int rowNumber = floor(invertedYPos / dimensions_.height * rows_);
    float yBasedNoteOffset = 0;
    for(int i = 0; i < rowNumber; ++i) {
        yBasedNoteOffset += semitoneOffsetForRow_[i];
    }
    return xPos / dimensions_.width * semitones_
           + yBasedNoteOffset
           + baseNote_;
}

float GridOverlay::normalizeXPos(float xPos) {
    return (xPos / dimensions_.width * semitones_);
}

float GridOverlay::normalizeYPos(float yPos) {
    float invertedYPos = dimensions_.height - yPos;
    int rowNumber = floor(invertedYPos / dimensions_.height * rows_);
    float cellHeight = dimensions_.height / rows_;
    float yOffsetInCell = invertedYPos - rowNumber * cellHeight;
    float normalizedYOffset = yOffsetInCell / cellHeight;
    return normalizedYOffset;
}

float GridOverlay::normalizeZPos(float zPos) {
    float normalizedPressure = zPos / dimensions_.max_pressure;
    if (normalizedPressure < 0.01) {
        normalizedPressure = 0.01;
    }
    return normalizedPressure;
}

}
}