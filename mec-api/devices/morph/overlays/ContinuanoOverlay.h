#ifndef MEC_CONTINUANOOVERLAY_H
#define MEC_CONTINUANOOVERLAY_H

#include <quantizers/XQuantizer.h>
#include "OverlayFunction.h"

namespace mec {
namespace morph {

class ContinuanoOverlay : public OverlayFunction {
public:
    ContinuanoOverlay(const std::string &name, ISurfaceCallback &surfaceCallback, ICallback &callback);

    virtual bool init(const Preferences &preferences, const PanelDimensions &dimensions);

    virtual bool processTouches(Touches &touches);

private:
    float semitones_;
    float baseNote_;
    bool roundNotePitchWhenNotMoving_;
    bool roundNoteOnPitch_;
    PanelDimensions dimensions_;
    XQuantizer quantizer_;
    float diatonicZoneUpperY_;
    float chromaticZoneUpperY_;
    float freePitchZoneUpperY_;
    float keyZoneHeight_;
    float octaveWidth_;
    float chromaticKeyWidth_;
    float keyWidthCDE_;
    float keyWidthFGAB_;
    enum KeyName {C = 0, D = 1, E = 2, F = 3, G = 4, A = 5, B = 6, NonDiatonic = 7};

    float xPosToNote(float xPos) const;

    float normalizeXPos(float xPos) const;

    float normalizeYPos(float yPos) const;

    float normalizeZPos(float zPos) const;

    bool getKey(Rectangle &key, const Touch &touch, KeyName &keyName) const;

    bool getDiatonicKey(Rectangle &key, const Touch &touch, KeyName &keyName) const;

    bool getChromaticKey(Rectangle &key, const Touch &touch) const;

    void applyDiatonicScale(Touch &touch, const KeyName &keyName) const;
};

}
}


#endif //MEC_CONTINUANOOVERLAY_H
