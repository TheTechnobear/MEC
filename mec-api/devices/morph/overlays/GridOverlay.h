#ifndef MEC_GRIDOVERLAY_H
#define MEC_GRIDOVERLAY_H

#include <panels/PanelDimensions.h>
#include <quantizers/XQuantizer.h>
#include "OverlayFunction.h"

namespace mec {
namespace morph {

class GridOverlay : public OverlayFunction {
public:
    GridOverlay(const std::string &name, ISurfaceCallback &surfaceCallback, ICallback &callback);

    virtual bool init(const Preferences &preferences, const PanelDimensions &dimensions);

    virtual bool processTouches(Touches &touches);

private:
    int rows_;
    int columns_;
    float semitones_;
    std::vector<float> semitoneOffsetForRow_;
    std::vector<float> xOffsetForRow_;
    float baseNote_;
    PanelDimensions dimensions_;
    float cellWidth_;
    float cellHeight_;
    XQuantizer quantizer_;
    bool roundNotePitchWhenNotMoving_;
    bool roundNoteOnPitch_;

    float xyPosToNote(float xPos, float yPos);

    float normalizeXPos(float xPos);

    float normalizeYPos(float yPos);

    float normalizeZPos(float zPos);

    void getCurrentCell(Rectangle &rectangle, const Touch &touch);
};

}
}


#endif //MEC_GRIDOVERLAY_H
