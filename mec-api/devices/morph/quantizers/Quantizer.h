#ifndef MEC_QUANTIZER_H
#define MEC_QUANTIZER_H

#include <touches/TouchWithDeltas.h>
#include "Rectangle.h"

namespace mec {
namespace morph {

class Quantizer {
public:
    Quantizer() {
    }
    virtual void quantizeNewTouch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch, const Rectangle &boundaries) = 0;
    virtual void quantizeContinuedTouch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch, const Rectangle &boundaries,
                                        bool roundNotePitchWhenNotMoving) = 0;
    virtual void retuneToOriginalPitch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch) = 0;

private:
};

}
}

#endif //MEC_QUANTIZER_H
