#include <quantizers/Rectangle.h>
#include <cmath>
#include "ContinuanoOverlay.h"
#include "../../../mec_log.h"

namespace mec {
namespace morph {

#define BUTTONBAR_HEIGHT 5
#define BUTTONBAR_MARGIN 5

ContinuanoOverlay::ContinuanoOverlay(const std::string &name, mec::ISurfaceCallback &surfaceCallback,
                                     mec::ICallback &callback) : OverlayFunction(name, surfaceCallback,
                                                                                 callback) {}
bool ContinuanoOverlay::init(const Preferences &preferences, const PanelDimensions &dimensions) {

    dimensions_ = dimensions;

    if (preferences.exists("semitones")) {
        semitones_ = preferences.getDouble("semitones");
    } else {
        semitones_ = 18;
        LOG_1("morph::ContinuanoOverlay::init - property semitones not defined, using default 18");
    }

    if (preferences.exists("baseNote")) {
        baseNote_ = preferences.getDouble("baseNote");
    } else {
        baseNote_ = 32;
        LOG_1("morph::ContinuanoOverlay::init - property baseNote not defined, using default 32");
    }

    float buttonZoneHeight = BUTTONBAR_HEIGHT + BUTTONBAR_MARGIN;
    keyZoneHeight_ = (dimensions_.height - buttonZoneHeight) / 3; // same for all three keyzones
    freePitchZoneUpperY_ = buttonZoneHeight;
    chromaticZoneUpperY_ = freePitchZoneUpperY_ + keyZoneHeight_;
    diatonicZoneUpperY_ = chromaticZoneUpperY_ + keyZoneHeight_;

    octaveWidth_ = (float)dimensions_.width / semitones_ * 12.0;
    chromaticKeyWidth_ = octaveWidth_ / 12;

    keyWidthCDE_ = 5.0 / 3.0 * chromaticKeyWidth_;
    keyWidthFGAB_ = 7.0 / 4.0 * chromaticKeyWidth_;

    return true;
}

bool ContinuanoOverlay::processTouches(Touches &touches) {
    const std::vector<std::shared_ptr<TouchWithDeltas>> &newTouches = touches.getNewTouches();
    TouchWithDeltas quantizedTouch;
    for (auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
        TouchWithDeltas *touch;
        Rectangle key;
        KeyName keyName;
        bool keyFound = getKey(key, **touchIter, keyName);
        //if(keyFound) {
            //quantizer_.quantizeNewTouch(quantizedTouch, **touchIter, key);
            //touch = &quantizedTouch;
        //} else {
            touch = &**touchIter;
        //}
        //TODO: how should a downpath scaler know which parts to scale in which way?  (Probably the Continuano Overlay might have to be split into sub-overlays)
        surfaceCallback_.touchOn(*touch);
        if(quantizedTouch.y_ >= diatonicZoneUpperY_) {
            applyDiatonicScale(quantizedTouch, keyName);
        }
        // remove as soon as surface support is fully implemented
        callback_.touchOn(touch->id_, xPosToNote(touch->x_), normalizeXPos(touch->x_),
                          normalizeYPos(touch->y_), normalizeZPos(touch->z_));
    }
    const std::vector<std::shared_ptr<TouchWithDeltas>> &continuedTouches = touches.getContinuedTouches();
    for (auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
        Rectangle key;

        KeyName keyName;
        bool keyFound = getKey(key, **touchIter, keyName);
        if(keyFound) {
            //quantizer_.quantizeContinuedTouch(quantizedTouch, **touchIter, key, true);
        } else {
            //quantizer_.retuneToOriginalPitch(quantizedTouch, **touchIter);
        }
        quantizedTouch = **touchIter;
        surfaceCallback_.touchContinue(quantizedTouch);
        if(quantizedTouch.y_ >= diatonicZoneUpperY_) {
            applyDiatonicScale(quantizedTouch, keyName);
        }
        // remove as soon as surface support is fully implemented
        callback_.touchContinue(quantizedTouch.id_, xPosToNote(quantizedTouch.x_), normalizeXPos(quantizedTouch.x_),
                                normalizeYPos(quantizedTouch.y_), normalizeZPos(quantizedTouch.z_));
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

bool ContinuanoOverlay::getKey(Rectangle &key, const Touch &touch, KeyName &keyName) const {
    if(touch.y_ > diatonicZoneUpperY_) {
        return getDiatonicKey(key, touch, keyName);
    } else if(touch.y_ > chromaticZoneUpperY_) {
        keyName = KeyName::NonDiatonic;
        return getChromaticKey(key, touch);
    }
    LOG_2("mec::ContinuanoOverlay::getKey freePitch");
    keyName = KeyName::NonDiatonic;
    return false;
}

bool ContinuanoOverlay::getDiatonicKey(Rectangle &key, const Touch &touch, KeyName &keyName) const {
    LOG_2("mec::ContinuanoOverlay::getDiatonicKey");
    float octaveStart = floor(touch.x_ / octaveWidth_) * octaveWidth_;
    float keyPos = octaveStart;
    float keyWidth;
    key.y = chromaticZoneUpperY_;
    key.height = keyZoneHeight_;
    for(int diatonicKeyInOctave = 0; diatonicKeyInOctave < 7; ++diatonicKeyInOctave) {
        if(diatonicKeyInOctave < 3) { // c, d and e have a width of 5 / 3 * 1/12th of an octave-width
            keyWidth = keyWidthCDE_;
        } else { // f, g, a and b have a width of 7 / 4 * 1/12th of an octave-width
            keyWidth = keyWidthFGAB_;
        }
        if(touch.x_ >= keyPos && touch.x_ < keyPos + keyWidth) {
            key.x = keyPos;
            key.width = keyWidth;
            keyName = static_cast<KeyName>(diatonicKeyInOctave);
            return true;
        }
        keyPos += keyWidth;
    }
    return false;
}

void ContinuanoOverlay::applyDiatonicScale(Touch &touch, const KeyName &keyName) const {
    float octaveStart = floor(touch.x_ / octaveWidth_) * octaveWidth_;
    float oldTouchX = touch.x_;;
    float posOnKey;
    float correspondingPosOnChromaticKey;
    switch(keyName) {
        case C: {
            posOnKey = touch.x_ - octaveStart;
            correspondingPosOnChromaticKey = posOnKey / keyWidthCDE_ * chromaticKeyWidth_;
            touch.x_ = octaveStart + correspondingPosOnChromaticKey;
            break;
        }
        case D: {
            posOnKey = touch.x_ - octaveStart - keyWidthCDE_;
            correspondingPosOnChromaticKey = posOnKey / keyWidthCDE_ * chromaticKeyWidth_;
            touch.x_ = octaveStart + chromaticKeyWidth_ * 2.0 + correspondingPosOnChromaticKey;
            break;
        }
        case E: {
            posOnKey = touch.x_ - octaveStart - 2 * keyWidthCDE_;
            correspondingPosOnChromaticKey = posOnKey / keyWidthCDE_ * chromaticKeyWidth_;
            touch.x_ = octaveStart + chromaticKeyWidth_ * 4.0 + correspondingPosOnChromaticKey;
            break;
        }
        case F: {
            posOnKey = touch.x_ - octaveStart - 3 * keyWidthCDE_;
            correspondingPosOnChromaticKey = posOnKey / keyWidthFGAB_ * chromaticKeyWidth_;
            touch.x_ = octaveStart + chromaticKeyWidth_ * 5.0 + correspondingPosOnChromaticKey;
            break;
        }
        case G: {
            posOnKey = touch.x_ - octaveStart - 3 * keyWidthCDE_ - keyWidthFGAB_;
            correspondingPosOnChromaticKey = posOnKey / keyWidthFGAB_ * chromaticKeyWidth_;
            touch.x_ = octaveStart + chromaticKeyWidth_ * 7.0 + correspondingPosOnChromaticKey;
            break;
        }
        case A: {
            posOnKey = touch.x_ - octaveStart - 3 * keyWidthCDE_ - 2 * keyWidthFGAB_;
            correspondingPosOnChromaticKey = posOnKey / keyWidthFGAB_ * chromaticKeyWidth_;
            touch.x_ = octaveStart + chromaticKeyWidth_ * 9.0 + correspondingPosOnChromaticKey;
            break;
        }
        case B:  {
            posOnKey = touch.x_ - octaveStart - 3 * keyWidthCDE_ - 3 * keyWidthFGAB_;
            correspondingPosOnChromaticKey = posOnKey / keyWidthFGAB_ * chromaticKeyWidth_;
            touch.x_ = octaveStart + chromaticKeyWidth_ * 11.0 + correspondingPosOnChromaticKey;
            break;
        }
        default:
            LOG_0("unexpected keyname " << keyName);
    }
    LOG_2("mec::ContinuanoOverlay::applyDiatonicScale keyName:" << keyName
             << ",posOnKey:" << posOnKey << ",corrPosOnKey:" << correspondingPosOnChromaticKey
             << ",touchX:" << touch.x_ << ",oldTouchX:" << oldTouchX << ",octaveStart:" << octaveStart);
}

bool ContinuanoOverlay::getChromaticKey(Rectangle &key, const Touch &touch) const {
    key.x = floor(touch.x_ / chromaticKeyWidth_) * chromaticKeyWidth_;
    key.y = chromaticZoneUpperY_;
    key.width = chromaticKeyWidth_;
    key.height = keyZoneHeight_;
    LOG_2("mec::ContinuanoOverlay::getChromaticKey");
    return true;
}

float ContinuanoOverlay::xPosToNote(float xPos) const {
    return (xPos / dimensions_.width * semitones_ + baseNote_);
}

float ContinuanoOverlay::normalizeXPos(float xPos) const {
    return (xPos / dimensions_.width * semitones_);
}

float ContinuanoOverlay::normalizeYPos(float yPos) const {
    return 1.0 - yPos / dimensions_.height;
}

float ContinuanoOverlay::normalizeZPos(float zPos) const {
    float normalizedPressure = zPos / dimensions_.max_pressure;
    if (normalizedPressure < 0.01) {
        normalizedPressure = 0.01;
    }
    return normalizedPressure;
}


}
}
