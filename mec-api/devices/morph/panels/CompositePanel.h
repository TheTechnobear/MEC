#ifndef MEC_COMPOSITEPANEL_H
#define MEC_COMPOSITEPANEL_H

#include <unordered_set>
#include <set>
#include <map>
#include "Panel.h"

namespace mec {
namespace morph {

class CompositePanel : public Panel {
public:
    CompositePanel(SurfaceID surfaceID, const std::vector <std::shared_ptr<Panel>> containedPanels,
                   int transitionAreaWidth, int maxInterpolationSteps);

    virtual bool init();

    virtual bool readTouches(Touches &touches);

    virtual const PanelDimensions& getDimensions();

private:
    struct MappedTouch : public TouchWithDeltas {
        int numInterpolationSteps_;

        void update(TouchWithDeltas &touch) {
            x_ = touch.x_;
            y_ = touch.y_;
            z_ = touch.z_;
            c_ = touch.c_;
            r_ = touch.r_;
            delta_x_ = touch.delta_x_;
            delta_y_ = touch.delta_y_;
            delta_z_ = touch.delta_z_;
        }
    };

    float transitionAreaWidth_;
    int maxInterpolationSteps_;
    std::unordered_set <std::shared_ptr<MappedTouch>> mappedTouches_;
    std::unordered_set <std::shared_ptr<MappedTouch>> interpolatedTouches_;
    std::set<int> availableTouchIDs;
    std::vector <std::shared_ptr<Panel>> containedPanels_;
    std::map<SurfaceID, int> containedPanelsPositions_;
    static std::shared_ptr <MappedTouch> NO_MAPPED_TOUCH;
    PanelDimensions dimensions_;

    void remapTouches(Touches &collectedTouches, Touches &remappedTouches);

    bool isInTransitionArea(TouchWithDeltas &touch);

    std::shared_ptr <MappedTouch> findCloseMappedTouch(TouchWithDeltas &touch);

    std::shared_ptr <MappedTouch> findMappedTouch(TouchWithDeltas &touch);

    void interpolatePosition(MappedTouch &touch);

    void remapStartedTouches(Touches &collectedTouches, Touches &remappedTouches,
                             std::unordered_set <std::shared_ptr<MappedTouch>> &associatedMappedTouches);

    void remapContinuedTouches(Touches &collectedTouches, Touches &remappedTouches,
                               std::unordered_set <std::shared_ptr<MappedTouch>> &associatedMappedTouches);

    void remapEndedTouches(Touches &collectedTouches, Touches &remappedTouches);

    void remapInTransitionInterpolatedTouches(Touches &remappedTouches,
                                              std::unordered_set <std::shared_ptr<MappedTouch>> &associatedMappedTouches);

    void endUnassociatedMappedTouches(Touches &remappedTouches,
                                      const std::unordered_set <std::shared_ptr<MappedTouch>> &associatedMappedTouches);

    void transposeTouch(const std::shared_ptr <TouchWithDeltas> &collectedTouch, TouchWithDeltas &transposedTouch);
};

}
}

#endif //MEC_COMPOSITEPANEL_H
