#ifndef MEC_RECTANGULARQUANTIZER_H
#define MEC_RECTANGULARQUANTIZER_H

#include "Quantizer.h"

namespace mec {
namespace morph {

class XQuantizer : public Quantizer {
public:
    XQuantizer();
    virtual void quantizeNewTouch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch, const Rectangle &boundaries);
    virtual void quantizeContinuedTouch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch, const Rectangle &boundaries,
    bool roundNotePitchWhenNotMoving);
    virtual void retuneToOriginalPitch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch);
private:
    static const float PITCH_CORRECTION_STEP;
    static const float NO_MOVEMENT_THRESHOLD;
};

}
}

#endif //MEC_RECTANGULARQUANTIZER_H
