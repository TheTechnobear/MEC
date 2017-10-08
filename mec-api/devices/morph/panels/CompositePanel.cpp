#include "CompositePanel.h"
#include "../../../mec_log.h"
#include "../morph_constants.h"

#define LOG_ONPRESS(x) //LOG_2(x)
#define LOG_ONCYCLE(x) //LOG_3(x)

namespace mec {
namespace morph {

#define MAX_NUM_CONCURRENT_OVERALL_CONTACTS 16 // has to be 0-15 atm as mec-api uses the touch ids as indices in the 0-15 voice array!
#define MAX_NUM_CONTACTS_PER_PANEL 16
#define MAX_X_DELTA_TO_CONSIDER_TOUCHES_CLOSE 15
#define MAX_Y_DELTA_TO_CONSIDER_TOUCHES_CLOSE 15
#define EXPECTED_MEASUREMENT_ERROR 5

CompositePanel::CompositePanel(SurfaceID surfaceID, const std::vector <std::shared_ptr<Panel>> containedPanels,
               int transitionAreaWidth, int maxInterpolationSteps) :
        Panel(surfaceID), containedPanels_(containedPanels), transitionAreaWidth_(transitionAreaWidth),
        maxInterpolationSteps_(maxInterpolationSteps) {
    for (int i = 0; i < MAX_NUM_CONCURRENT_OVERALL_CONTACTS; ++i) {
        availableTouchIDs.insert(i);
    }
    for (int i = 0; i < containedPanels.size(); ++i) {
        containedPanelsPositions_[containedPanels[i]->getSurfaceID()] = i;
    }
}

bool CompositePanel::init() {
    compositePanelDimensions_.max_pressure = 0;
    compositePanelDimensions_.height = 0;
    compositePanelDimensions_.width = 0;
    for(auto panelIter = containedPanels_.begin(); panelIter != containedPanels_.end(); ++ panelIter) {
        const PanelDimensions& panelDimensions = (*panelIter)->getDimensions();
        compositePanelDimensions_.width += panelDimensions.width;
        if(panelDimensions.height > compositePanelDimensions_.height) {
            compositePanelDimensions_.height = panelDimensions.height;
        }
        if(panelDimensions.max_pressure > compositePanelDimensions_.max_pressure) {
            compositePanelDimensions_.max_pressure = panelDimensions.max_pressure;
        }
    }
    singlePanelDimensions_.max_pressure = compositePanelDimensions_.max_pressure;
    singlePanelDimensions_.height = compositePanelDimensions_.height;
    singlePanelDimensions_.width = compositePanelDimensions_.width / containedPanels_.size();
    return true;
}

const PanelDimensions& CompositePanel::getDimensions() {
    return compositePanelDimensions_;
}

bool CompositePanel::readTouches(Touches &touches) {
    touches.clear();
    Touches newTouches;
    LOG_ONCYCLE("morph::CompositePanel::readTouches - reading touches for contained panel " << getSurfaceID());
    for (auto panelIter = containedPanels_.begin(); panelIter != containedPanels_.end(); ++panelIter) {
        bool touchesRead = (*panelIter)->readTouches(newTouches);
        if (!touchesRead) {
            LOG_0("morph::CompositePanel::readTouches - unable to read touches for contained panel "
                          << (*panelIter)->getSurfaceID());
            continue; // perhaps we have more luck with some of the other panels
        }
    }
    LOG_ONCYCLE("morph::CompositePanel::readTouches - read touches for contained panel " << getSurfaceID());
    LOG_ONCYCLE("morph::CompositePanel::readTouches - remapping touches for contained panel " << getSurfaceID());
    remapTouches(newTouches, touches);
    LOG_ONCYCLE("morph::CompositePanel::readTouches - remapped touches for contained panel " << getSurfaceID());
    return true;
}

void CompositePanel::remapTouches(Touches &collectedTouches, Touches &remappedTouches) {
    remappedTouches.clear();
    std::unordered_set <std::shared_ptr<MappedTouch>> associatedMappedTouches;

    remapStartedTouches(collectedTouches, remappedTouches, associatedMappedTouches);

    remapContinuedTouches(collectedTouches, remappedTouches, associatedMappedTouches);

    remapEndedTouches(collectedTouches, remappedTouches);

    remapInTransitionInterpolatedTouches(remappedTouches, associatedMappedTouches);

    endUnassociatedMappedTouches(remappedTouches, associatedMappedTouches);
}

bool CompositePanel::isInTransitionArea(TouchWithDeltas &touch) {
    double xpos_on_panel = fmod(touch.x_, (float) singlePanelDimensions_.width);
    return xpos_on_panel >= singlePanelDimensions_.width - transitionAreaWidth_ / 2 ||
           xpos_on_panel <= transitionAreaWidth_ / 2;
}

std::shared_ptr <CompositePanel::MappedTouch> CompositePanel::findCloseMappedTouch(TouchWithDeltas &touch) {
    LOG_ONPRESS("map::CompositePanel::findCloseMappedTouch - touch to find: (x: " << touch.x_ << ",y:" << touch.y_
                                                                            << ",dx:" << touch.delta_x_ << ",dy:"
                                                                            << touch.delta_y_ << ")");
    for (auto touchIter = mappedTouches_.begin(); touchIter != mappedTouches_.end(); ++touchIter) {
        LOG_ONPRESS("map::CompositePanel::findCloseMappedTouch - mapped touch: (x: "
                      << (*touchIter)->x_ << ",y:" << (*touchIter)->y_
                      << ",dx:" << (*touchIter)->delta_x_ << ",dy:" << (*touchIter)->delta_y_ << ")");
        LOG_ONPRESS("map::CompositePanel::findCloseMappedTouch - interpolated mapped touch: (x: "
                      << (*touchIter)->x_ + (*touchIter)->delta_x_ << ",y:"
                      << (*touchIter)->y_ + (*touchIter)->delta_y_
                      << ",dx:" << (*touchIter)->delta_x_ << ",dy:" << (*touchIter)->delta_y_ << ")");
        // find a mapped touch that would be expected to be at the approximate position of the current touch in the next step
        if (fabs((*touchIter)->x_ + (*touchIter)->delta_x_ - touch.x_) <= MAX_X_DELTA_TO_CONSIDER_TOUCHES_CLOSE &&
            fabs((*touchIter)->y_ + (*touchIter)->delta_y_ - touch.y_) <= MAX_Y_DELTA_TO_CONSIDER_TOUCHES_CLOSE) {
            LOG_ONPRESS("map::CompositePanel::findCloseMappedTouch - mapped touch found");
            return *touchIter;
        }
    }
    LOG_ONPRESS("map::CompositePanel::findCloseMappedTouch - no close mapped touch found");
    return NO_MAPPED_TOUCH;
}

std::shared_ptr <CompositePanel::MappedTouch> CompositePanel::findMappedTouch(TouchWithDeltas &touch) {
    LOG_ONPRESS("map::CompositePanel::findMappedTouch - touch to find: (x: " << touch.x_ << ",y:" << touch.y_
                                                                       << ",dx:" << touch.delta_x_ << ",dy:"
                                                                       << touch.delta_y_ << ")");
    for (auto touchIter = mappedTouches_.begin(); touchIter != mappedTouches_.end(); ++touchIter) {
        LOG_ONPRESS("map::CompositePanel::findMappedTouch - mapped touch: (x: " << (*touchIter)->x_
                                                                          << ",y:" << (*touchIter)->y_ << ",dx:"
                                                                          << (*touchIter)->delta_x_ << ",dy:"
                                                                          << (*touchIter)->delta_y_ << ")");
        LOG_ONPRESS("map::CompositePanel::findMappedTouch - interpolated mapped touch: (x: "
                      << (*touchIter)->x_ + (*touchIter)->delta_x_
                      << ",y:" << (*touchIter)->y_ + (*touchIter)->delta_y_ << ",dx:" << (*touchIter)->delta_x_
                      << ",dy:" << (*touchIter)->delta_y_ << ")");
        // find a mapped touch at the position where the current touch previously was
        if (fabs((*touchIter)->x_ - touch.x_ + touch.delta_x_) <= EXPECTED_MEASUREMENT_ERROR &&
            fabs((*touchIter)->y_ - touch.y_ + touch.delta_y_) <= EXPECTED_MEASUREMENT_ERROR) {
            LOG_ONPRESS("map::CompositePanel::findMappedTouch - mapped touch found");
            return *touchIter;
        }
    }
    return NO_MAPPED_TOUCH;
}

void CompositePanel::interpolatePosition(CompositePanel::MappedTouch &touch) {
    touch.x_ += touch.delta_x_;
    touch.y_ += touch.delta_y_;
    touch.numInterpolationSteps_ += 1;
}

void CompositePanel::remapStartedTouches(Touches &collectedTouches, Touches &remappedTouches,
                         std::unordered_set <std::shared_ptr<MappedTouch>> &associatedMappedTouches) {
    const std::vector <std::shared_ptr<TouchWithDeltas>> &newTouches = collectedTouches.getNewTouches();
    for (auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
        LOG_ONPRESS("morph::CompositePanel::remapStartedTouches - creating new touch for panel " << getSurfaceID()
                                                                                           << "; num mapped touches: "
                                                                                           << mappedTouches_.size());
        TouchWithDeltas transposedTouch;
        transposeTouch(*touchIter, transposedTouch);
        bool createNewMappingEntry = true;
        // a new touch in a transition area that is close to an existing touch in the transition area
        // moves that touch to the new position instead of creating a new touch.
        if(isInTransitionArea(transposedTouch)) {
            std::shared_ptr <MappedTouch> closeMappedTouch = findCloseMappedTouch(transposedTouch);
            LOG_ONPRESS("mec::CompositePanel::remapStartedTouches - in transition. num mapped touches: "
                      << mappedTouches_.size());
        if (closeMappedTouch != NO_MAPPED_TOUCH) {
            LOG_ONPRESS("mec::CompositePanel::remapStartedTouches - close touch found");
            interpolatedTouches_.erase(closeMappedTouch);
            float oldZ = closeMappedTouch->z_;
            closeMappedTouch->update(transposedTouch);
            closeMappedTouch->z_ = oldZ;
            closeMappedTouch->numInterpolationSteps_ = 0;
            LOG_ONPRESS("mec::CompositePanel::remapStartedTouches - continued. num interp.steps: "
                          << closeMappedTouch->numInterpolationSteps_);
            associatedMappedTouches.insert(closeMappedTouch);
            remappedTouches.addContinued(closeMappedTouch);
            createNewMappingEntry = false;
        } else {
            LOG_ONPRESS("mec::CompositePanel::remapStartedTouches - no close touch found");
        }
        }
        if (createNewMappingEntry) {
            std::shared_ptr <MappedTouch> mappedTouch;
            mappedTouch.reset(new MappedTouch(transposedTouch));
            int touchID = *availableTouchIDs.begin();
            availableTouchIDs.erase(touchID);
            mappedTouch->id_ = touchID;
            mappedTouch->surface_ = getSurfaceID();
            mappedTouches_.insert(mappedTouch);
            LOG_ONPRESS("mec::CompositePanel::remapStartedTouches - new. num mapped touches " << mappedTouches_.size()
                                                                                        << " num interp.steps: "
                                                                                        << mappedTouch->numInterpolationSteps_);
            associatedMappedTouches.insert(mappedTouch);
            remappedTouches.addNew(mappedTouch);
        }
        LOG_ONPRESS("morph::CompositePanel::remapStartedTouches - new touch created for panel " << getSurfaceID()
                                                                                          << "; num mapped touches: "
                                                                                          << mappedTouches_.size());
    }
}

void CompositePanel::remapContinuedTouches(Touches &collectedTouches, Touches &remappedTouches,
                           std::unordered_set <std::shared_ptr<MappedTouch>> &associatedMappedTouches) {
    TouchWithDeltas transposedTouch;
    const std::vector <std::shared_ptr<TouchWithDeltas>> &continuedTouches = collectedTouches.getContinuedTouches();
    for (auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
        LOG_ONPRESS("morph::CompositePanel::remapContinuedTouches - continuing touch for panel " << getSurfaceID()
                                                                                           << "; num mapped touches: "
                                                                                           << mappedTouches_.size());
        transposeTouch(*touchIter, transposedTouch);
        std::shared_ptr <MappedTouch> mappedTouch = findMappedTouch(transposedTouch);
        if (mappedTouch != NO_MAPPED_TOUCH) {
            // for continued touches in the transition area the force is kept constant because force sensing
            // becomes unreliable at the boundaries of the panel (only x and y are still taken over)
            if (isInTransitionArea(transposedTouch)) {
                float oldZ = mappedTouch->z_;
                mappedTouch->update(transposedTouch);
                mappedTouch->z_ = oldZ;
            } else {
                mappedTouch->numInterpolationSteps_ = 0;
                mappedTouch->update(transposedTouch);
            }
            //LOG_2("mec::CompositePanel::remapContinuedTouches - continued");
            associatedMappedTouches.insert(mappedTouch);
            remappedTouches.addContinued(mappedTouch);
        } else {
            LOG_ONPRESS("mec::CompositePanel::remapContinuedTouches - no mapped touch found for: (x:"
                          << transposedTouch.x_ << ",y:" << transposedTouch.y_ << ")");
        }
        LOG_ONPRESS("morph::CompositePanel::remapContinuedTouches - touch continued for panel " << getSurfaceID()
                                                                                          << "; num mapped touches: "
                                                                                          << mappedTouches_.size());
    }
}

void CompositePanel::remapEndedTouches(Touches &collectedTouches, Touches &remappedTouches) {
    TouchWithDeltas transposedTouch;
    const std::vector <std::shared_ptr<TouchWithDeltas>> &endedTouches = collectedTouches.getEndedTouches();
    for (auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
        LOG_ONPRESS("morph::CompositePanel::remapEndedTouches - ending touch for panel " << getSurfaceID()
                                                                                   << "; num mapped touches: "
                                                                                   << mappedTouches_.size());
        transposeTouch(*touchIter, transposedTouch);
        std::shared_ptr <MappedTouch> mappedTouch = findMappedTouch(transposedTouch);
        if (mappedTouch != NO_MAPPED_TOUCH) {
            // an ended touch in the transition area is interpolated for MAX_NUM_INTERPOLATION_STEPS before being
            // either taken over by a close new touch or being ended
            if (isInTransitionArea(transposedTouch) &&
                mappedTouch->numInterpolationSteps_ < maxInterpolationSteps_) {
                interpolatedTouches_.insert(mappedTouch);
                LOG_ONPRESS("mec::CompositePanel::remapEndedTouches - continued. num interp.steps: "
                              << mappedTouch->numInterpolationSteps_);
                remappedTouches.addContinued(mappedTouch);
            } else {
                std::shared_ptr <MappedTouch> &endedTouch = mappedTouch;
                mappedTouches_.erase(mappedTouch);
                endedTouch->update(transposedTouch);
                LOG_ONPRESS("mec::CompositePanel::remapEndedTouches - ended. num mapped touches: "
                              << mappedTouches_.size() << " num interp.steps: "
                              << endedTouch->numInterpolationSteps_);
                remappedTouches.addEnded(endedTouch);
                availableTouchIDs.insert(endedTouch->id_);
            }
        } else {
            LOG_0("morph::CompositePanel::remapEndedTouches - no mapped touch found! ");
        }
        //else: nothing to do, touch was not mapped
        LOG_ONPRESS("morph::CompositePanel::remapEndedTouches - touch ended for panel " << getSurfaceID()
                                                                                  << "; num mapped touches: "
                                                                                  << mappedTouches_.size());
    }
}

void CompositePanel::remapInTransitionInterpolatedTouches(Touches &remappedTouches,
                                          std::unordered_set <std::shared_ptr<MappedTouch>> &associatedMappedTouches) {
    std::unordered_set <std::shared_ptr<MappedTouch>> endedInterpolatedTouches;
    for (auto touchIter = interpolatedTouches_.begin(); touchIter != interpolatedTouches_.end(); ++touchIter) {
        LOG_ONPRESS("morph::CompositePanel::remapInTransitionInterpolatedTouches - transitioning touch for panel "
                      << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
        // remaining continued touches inside the transpsition area (where the original touches were already ended
        // in a previous cycle) are still interpolated up to MAX_NUM_INTERPOLATION_STEPS times.
        // If they were not taken over by a close new touch by then they are ended.
        bool removeTouch = true;
        if (isInTransitionArea(**touchIter) && (*touchIter)->numInterpolationSteps_ < maxInterpolationSteps_) {
            interpolatePosition(**touchIter);
            LOG_ONPRESS("mec::CompositePanel::remapInTransitionInterpolatedTouches - continued. num interp.steps: "
                          << (*touchIter)->numInterpolationSteps_);
            associatedMappedTouches.insert(*touchIter);
            remappedTouches.addContinued(*touchIter);
            removeTouch = false;
        } else {
            LOG_ONPRESS("mec::CompositePanel::remapInTransitionInterpolatedTouches - not in transition area, ending");
        }
        if (removeTouch) {
            std::shared_ptr <MappedTouch> endedTouch = *touchIter;
            endedInterpolatedTouches.insert(endedTouch);
            mappedTouches_.erase(*touchIter);
            availableTouchIDs.insert(endedTouch->id_);
            LOG_ONPRESS("mec::CompositePanel::remapInTransitionInterpolatedTouches - ended. mapped touches: "
                          << mappedTouches_.size() << " num interp.steps: " << endedTouch->numInterpolationSteps_);
            LOG_ONPRESS("mec::CompositePanel::remapInTransitionInterpolatedTouched - ended touch: (x:" << endedTouch->x_
                                                                                                 << ",y:"
                                                                                                 << endedTouch->y_
                                                                                                 << ")");
            remappedTouches.addEnded(endedTouch);
        }
        LOG_ONPRESS("morph::CompositePanel::remapInTransitionInterpolatedTouches - touch transitioned for panel "
                      << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
    }
    for (auto touchIter = endedInterpolatedTouches.begin();
         touchIter != endedInterpolatedTouches.end(); ++touchIter) {
        interpolatedTouches_.erase(*touchIter);
    }
}

void CompositePanel::endUnassociatedMappedTouches(Touches &remappedTouches,
                                  const std::unordered_set <std::shared_ptr<MappedTouch>> &associatedMappedTouches) {
    if (associatedMappedTouches.size() != mappedTouches_.size()) {
        std::unordered_set <std::shared_ptr<MappedTouch>> intersection;
        std::set_intersection(mappedTouches_.begin(), mappedTouches_.end(),
                              associatedMappedTouches.begin(), associatedMappedTouches.end(),
                              std::inserter(intersection, intersection.begin()));
        for (auto intersectionIter = intersection.begin();
             intersectionIter != intersection.end(); ++intersectionIter) {
            std::shared_ptr <MappedTouch> endedTouch = *intersectionIter;
            mappedTouches_.erase(*intersectionIter);
            availableTouchIDs.insert(endedTouch->id_);
            LOG_ONPRESS("mec::CompositePanel::endUnassociatedMappedTouches - ended. mapped touches: "
                          << mappedTouches_.size() << " num interp.steps: " << endedTouch->numInterpolationSteps_);
            LOG_ONPRESS("mec::CompositePanel::endUnassociatedMappedTouches - ended touch: (x:" << endedTouch->x_ << ",y:"
                                                                                         << endedTouch->y_ << ")");
            remappedTouches.addEnded(endedTouch);
        }
    }
}

void CompositePanel::transposeTouch(const std::shared_ptr <TouchWithDeltas> &collectedTouch, TouchWithDeltas &transposedTouch) {
    transposedTouch.id_ = collectedTouch->id_;
    LOG_ONPRESS("mec::CompositePanel::transposeTouch - collectedTouch: (x:" << collectedTouch->x_ << ",y:"
                                                                      << collectedTouch->y_
                                                                      << ",dx:" << collectedTouch->delta_x_
                                                                      << ",dy:" << collectedTouch->delta_y_
                                                                      << ",id:" << collectedTouch->id_ << ")");
    auto panelPositionIter = containedPanelsPositions_.find(collectedTouch->surface_);
    int panelPosition = 0;
    if (panelPositionIter != containedPanelsPositions_.end()) {
        panelPosition = panelPositionIter->second;
    } else {
        LOG_0("morph::CompositePanel::transposeTouch - surface " << collectedTouch->surface_
                                                                 << " is not registered for composite panel "
                                                                 << getSurfaceID()
                                                                 << "but still received a touch event from it");
    }
    transposedTouch.x_ = collectedTouch->x_ + panelPosition * singlePanelDimensions_.width;
    transposedTouch.y_ = collectedTouch->y_;
    transposedTouch.z_ = collectedTouch->z_;
    transposedTouch.surface_ = collectedTouch->surface_;
    transposedTouch.c_ = transposedTouch.y_;
    transposedTouch.r_ = transposedTouch.x_;
    transposedTouch.delta_x_ = collectedTouch->delta_x_;
    transposedTouch.delta_y_ = collectedTouch->delta_y_;
    transposedTouch.delta_z_ = collectedTouch->delta_z_;
    transposedTouch.quantizing_offset_x_ = collectedTouch->quantizing_offset_x_;
    LOG_ONPRESS("mec::CompositePanel::transposeTouch - transposedTouch: (x:" << transposedTouch.x_ << ",y:"
                                                                       << transposedTouch.y_
                                                                       << ",dx:" << transposedTouch.delta_x_
                                                                       << ",dy:" << transposedTouch.delta_y_
                                                                       << ",id:" << transposedTouch.id_ << ")");
}

std::shared_ptr <CompositePanel::MappedTouch> CompositePanel::NO_MAPPED_TOUCH;

}
}