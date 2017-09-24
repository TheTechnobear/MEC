#include "mec_morph.h"
#include "../mec_log.h"

#include <src/sensel.h>

#include <vector>
#include <memory>
#include <unordered_set>
#include <cmath>
#include <set>
#include <map>

namespace mec {
namespace morph {

#define MAX_NUM_INTERPOLATION_STEPS 50
#define MAX_NUM_CONTACTS_PER_PANEL 16
#define MAX_NUM_CONCURRENT_OVERALL_CONTACTS 16 // has to be 0-15 atm as mec-api uses the touch ids as indices in the 0-15 voice array!
#define MAX_X_DELTA_TO_CONSIDER_TOUCHES_CLOSE 15
#define MAX_Y_DELTA_TO_CONSIDER_TOUCHES_CLOSE 15
#define EXPECTED_MEASUREMENT_ERROR 5
#define PANEL_WIDTH 230 //TODO: find out panel width via Sensel API
#define PANEL_HEIGHT 128
#define MAX_Z_PRESSURE 2048

struct TouchWithDeltas : public Touch {
    TouchWithDeltas() : delta_x_(0.0f), delta_y_(0.0f), delta_z_(0.0f) {}
    TouchWithDeltas(int id, SurfaceID surface, float x, float y, float z, float r, float c, float delta_x, float delta_y, float delta_z)
    : Touch(id, surface, x, y, z, r, c), delta_x_(delta_x), delta_y_(delta_y), delta_z_(delta_z) {}

    float delta_x_;
    float delta_y_;
    float delta_z_;
};

class Touches {
public:
    void addNew(const TouchWithDeltas& touch) {
        newTouches_.push_back(touch);
    }

    void addContinued(const TouchWithDeltas& touch) {
        continuedTouches_.push_back(touch);
    }

    void addEnded(const TouchWithDeltas& touch) {
        endedTouches_.push_back(touch);
    }

    const std::vector<TouchWithDeltas>& getNewTouches() const {
        return newTouches_;
    }

    const std::vector<TouchWithDeltas>& getContinuedTouches() const {
        return continuedTouches_;
    }

    const std::vector<TouchWithDeltas>& getEndedTouches() const {
        return endedTouches_;
    }

    void clear() {
        newTouches_.clear();
        continuedTouches_.clear();
        endedTouches_.clear();
    }
    
    void add(Touches& touches) {
        const std::vector<TouchWithDeltas> &newTouches = touches.getNewTouches();
        for(auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
            addNew(*touchIter);
        }
        const std::vector<TouchWithDeltas> &continuedTouches = touches.getContinuedTouches();
        for(auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
            addContinued(*touchIter);
        }
        const std::vector<TouchWithDeltas> &endedTouches = touches.getEndedTouches();
        for(auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
            addEnded(*touchIter);
        }
    }
private:
    std::vector<TouchWithDeltas> newTouches_;
    std::vector<TouchWithDeltas> continuedTouches_;
    std::vector<TouchWithDeltas> endedTouches_;
};

class Panel {
public:
    Panel(const SurfaceID &surfaceID) : surfaceID_(surfaceID) {}
    const SurfaceID& getSurfaceID() const {
        return surfaceID_;
    }
    virtual bool init() = 0;
    virtual bool readTouches(Touches &touches) = 0;

private:
    SurfaceID surfaceID_;
};

class SinglePanel : public Panel {
public:
    SinglePanel(const SenselDeviceID &deviceID, const SurfaceID &surfaceID) : Panel(surfaceID), deviceID_(deviceID), handle_(nullptr), frameData_(nullptr)
    {}

    virtual bool init() {
        SenselStatus openStatus = senselOpenDeviceByComPort(&handle_, deviceID_.com_port);
        if (openStatus != SENSEL_OK) {
            LOG_0("morph::Panel::init - unable to open device at com port " << deviceID_.com_port);
            return false;
        }
        SenselFirmwareInfo firmwareInfo;
        SenselStatus getFirmwareStatus = senselGetFirmwareInfo(&handle_, &firmwareInfo);
        if (getFirmwareStatus != SENSEL_OK) {
            LOG_0("morph::Panel::init - unable to retrieve firmware info for panel " << getSurfaceID());
            return false;
        }
        LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": device_id: " << +firmwareInfo.device_id);
        LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": revision: " << +firmwareInfo.device_revision);
        LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": protocol version: " << +firmwareInfo.fw_protocol_version);
        LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": fw_version_build: "  << +firmwareInfo.fw_version_build);
        LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": fw_version_major: "  << +firmwareInfo.fw_version_major);
        LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": fw_version_minor: "  << +firmwareInfo.fw_version_minor);
        LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": fw_version_release: " << +firmwareInfo.fw_version_release);
        SenselStatus setFrameContentStatus = senselSetFrameContent(handle_, FRAME_CONTENT_CONTACTS_MASK);
        if (setFrameContentStatus != SENSEL_OK) {
            LOG_0("morph::Panel::init - unable to set contacts mask for panel " << getSurfaceID());
            return false;
        }
        SenselStatus allocFrameStatus = senselAllocateFrameData(handle_, &frameData_);
        if (allocFrameStatus != SENSEL_OK) {
            LOG_0("morph::Panel::init - unable to allocate frame data for panel " << getSurfaceID());
            return false;
        }
        senselStartScanning(handle_);
        LOG_2("morph::Panel::init - panel " << getSurfaceID() << " initialized");
        return true;
    }

    ~SinglePanel() {
        if (handle_ != nullptr) {
            SenselStatus stopStatus = senselStopScanning(handle_);
            //TODO - should we wait?
            if (stopStatus != SENSEL_OK) {
                LOG_0("morph::Panel::destructor - unable to stop panel " << getSurfaceID());
            }
            if (frameData_ != nullptr) {
                SenselStatus freeFrameStatus = senselFreeFrameData(handle_, frameData_);
                if (freeFrameStatus != SENSEL_OK) {
                    LOG_0("morph::Panel::destructor - unable to free frame data for panel " << getSurfaceID());
                }
            }
            SenselStatus closeStatus = senselClose(handle_);
            if (closeStatus != SENSEL_OK) {
                LOG_0("morph::Panel::destructor - unable to close panel " << getSurfaceID());
            }
        }
        LOG_2("morph::Panel::destructor - panel " << getSurfaceID() << " freed");
    }

    virtual bool readTouches(Touches &touches) {
        //LOG_2("morph::Panel::readTouches - enter for panel " << getSurfaceID());
        bool sensorReadState = senselReadSensor(handle_);
        if(sensorReadState != SENSEL_OK) {
            LOG_0("morph::Panel::readTouches - unable to read sensor for panel " << getSurfaceID());
            return false;
        }
        unsigned int num_frames = 0;
        bool numFramesRetrievedState = senselGetNumAvailableFrames(handle_, &num_frames);
        if(numFramesRetrievedState != SENSEL_OK) {
            LOG_0("morph::Panel::readTouches - unable to determine number of frames for panel " << getSurfaceID());
            return false;
        }
        for(int current_frame = 0; current_frame < num_frames; ++current_frame) {
            bool frameRetrievedState = senselGetFrame(handle_, frameData_);
            if(frameRetrievedState != SENSEL_OK) {
                LOG_0("morph::Panel::readTouches - unable to get frame data for panel " << getSurfaceID());
                continue; // better try to continue, other frames might be usable
            }
            for(int current_contact = 0; current_contact < frameData_->n_contacts; ++current_contact) {
                unsigned int state = frameData_->contacts[current_contact].state;
                float x = frameData_->contacts[current_contact].x_pos;
                float x_delta = 0; //frameData_->contacts[current_contact].delta_x;
                float y = frameData_->contacts[current_contact].y_pos;
                float y_delta = 0; // frameData_->contacts[current_contact].delta_y;
                float z = frameData_->contacts[current_contact].total_force;
                float z_delta = 0; //frameData_->contacts[current_contact].delta_force;
                float c = y;
                float r = x;
                std::string stateString;
                int touchid = current_contact;
                std::shared_ptr<TouchWithDeltas> touch;
                touch.reset(new TouchWithDeltas(touchid, getSurfaceID(), x, y, z, r, c, x_delta, y_delta, z_delta));
                switch(state) {
                    case CONTACT_START: // 1
                        touches.addNew(*touch);
                        stateString = "CONTACT_START";
                        activeTouches_[touchid] = touch;
                        break;
                    case CONTACT_MOVE: // 2
                    {
                        // delta_x, delta_y and delta_z as returned from the firmware
                        // are either 0 or way off atm. (TODO: investigate/report)
                        // thus we have to calculate the deltas ourselves until this is fixed
                        // by memorizing the last (x,y,z) position per touchid
                        std::shared_ptr<TouchWithDeltas> foundTouch = activeTouches_[touchid];
                        updateDeltas(touch, foundTouch);
                        touches.addContinued(*foundTouch);
                        stateString = "CONTACT_MOVE";
                        break;
                    }
                    case CONTACT_END: // 3
                    {
                        std::shared_ptr<TouchWithDeltas> foundTouch = activeTouches_[touchid];
                        updateDeltas(touch, foundTouch);
                        touches.addEnded(*foundTouch);
                        if(foundTouch) {
                            activeTouches_.erase(touchid);
                        } else {
                            LOG_0("mec::readTouches - unable to find active touch for end event - this shouldn't happen.");
                        }
                        stateString = "CONTACT_END";
                        break;
                    }
                    case CONTACT_INVALID: // 0
                    default:
                        stateString = "CONTACT_INVALID";
                        LOG_0("morph::Panel::readTouches - invalid/unexpected touch state " << state);
                        // better try to continue, other contacts might be usable
                }
                LOG_2("mec::SinglePanel::readTouches: (id:" << touchid << ",x: " << x << ",y:" << y << ",dx:" << x_delta << ",dy:"
                                                            << y_delta << ",state:" << stateString << ")");
            }
        }
        //LOG_2("morph::Panel::readTouches - leave for panel " << getSurfaceID());
        return true;
    }

private:
    SENSEL_HANDLE handle_;
    SenselFrameData *frameData_;
    SenselDeviceID deviceID_;
    std::map<int, std::shared_ptr<TouchWithDeltas>> activeTouches_;

    void updateDeltas(const std::shared_ptr<TouchWithDeltas> &touch,
                                                   std::shared_ptr<TouchWithDeltas> &foundTouch) const {
        if (foundTouch) {
            foundTouch->delta_x_ = touch->x_ - foundTouch->x_;
            foundTouch->delta_y_ = touch->y_ - foundTouch->y_;
            foundTouch->delta_z_ = touch->z_ - foundTouch->z_;
            foundTouch->x_ = touch->x_;
            foundTouch->y_ = touch->y_;
            foundTouch->z_ = touch->z_;
        } else {
            LOG_0("mec::readTouches - unable to find active touch for move event - this shouldn't happen.");
            foundTouch = touch;
        }
        LOG_2("mec::SinglePanel::updateDeltas (corrected deltas): (x: " << foundTouch->x_ << ",y:" << foundTouch->y_
                                                                       << ",dx:" << foundTouch->delta_x_ << ",dy:"
                                                                       << foundTouch->delta_y_ << ")");
    }
};

class CompositePanel : public Panel {
public:
    CompositePanel(SurfaceID surfaceID, const std::vector<std::shared_ptr<Panel>> containedPanels, int transitionAreaWidth) :
            Panel(surfaceID), containedPanels_(containedPanels), transitionAreaWidth_(transitionAreaWidth)
    {
        for(int i = 0; i < MAX_NUM_CONCURRENT_OVERALL_CONTACTS; ++i) {
            availableTouchIDs.insert(i);
        }
        for(int i = 0; i < containedPanels.size(); ++i) {
            containedPanelsPositions_[containedPanels[i]->getSurfaceID()] = i;
        }
    }

    virtual bool init() {
        return true;
    }

    virtual bool readTouches(Touches &touches) {
        touches.clear();
        Touches newTouches;
        for(auto panelIter = containedPanels_.begin(); panelIter != containedPanels_.end(); ++panelIter) {
            bool touchesRead = (*panelIter)->readTouches(newTouches);
            if(!touchesRead) {
                LOG_0("morph::CompositePanel::readTouches - unable to read touches for contained panel " << (*panelIter)->getSurfaceID());
                continue; // perhaps we have more luck with some of the other panels
            }
        }
        remapTouches(newTouches, touches);
        return true;
    }

private:
    struct MappedTouch : public TouchWithDeltas {
        int numInterpolationSteps_;

        void update(TouchWithDeltas& touch) {
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
    std::unordered_set<std::shared_ptr<MappedTouch>> mappedTouches_;
    std::unordered_set<std::shared_ptr<MappedTouch>> interpolatedTouches_;
    std::set<int> availableTouchIDs;
    std::vector<std::shared_ptr<Panel>> containedPanels_;
    std::map<SurfaceID, int> containedPanelsPositions_;
    static std::shared_ptr<MappedTouch> NO_MAPPED_TOUCH;

    void remapTouches(Touches& collectedTouches, Touches& remappedTouches) {
        remappedTouches.clear();

        remapEndedTouches(collectedTouches, remappedTouches);

        remapContinuedTouches(collectedTouches, remappedTouches);

        remapStartedTouches(collectedTouches, remappedTouches);

        remapInTransitionInterpolatedTouches(remappedTouches);
    }

    bool isInTransitionArea(TouchWithDeltas& touch) {
        double xpos_on_panel = fmod(touch.x_, (float)PANEL_WIDTH);
        return xpos_on_panel >= PANEL_WIDTH - transitionAreaWidth_ / 2 ||
                xpos_on_panel <= transitionAreaWidth_ / 2;
    }

    std::shared_ptr<MappedTouch> findCloseMappedTouch(TouchWithDeltas& touch) {
        LOG_2("map::CompositePanel::findCloseMappedTouch - touch to find: (x: " << touch.x_ << ",y:" << touch.y_
                                                                           << ",dx:" << touch.delta_x_ << ",dy:" << touch.delta_y_ << ")");
        for(auto touchIter = mappedTouches_.begin(); touchIter != mappedTouches_.end(); ++touchIter) {
            LOG_2("map::CompositePanel::findCloseMappedTouch - mapped touch: (x: "
                          << (*touchIter)->x_ << ",y:" << (*touchIter)->y_
                          << ",dx:" << (*touchIter)->delta_x_ << ",dy:" << (*touchIter)->delta_y_ << ")");
            if(fabs((*touchIter)->x_ - touch.x_) <= MAX_X_DELTA_TO_CONSIDER_TOUCHES_CLOSE &&
                    fabs((*touchIter)->y_ - touch.y_) <= MAX_Y_DELTA_TO_CONSIDER_TOUCHES_CLOSE) {
                LOG_2("map::CompositePanel::findCloseMappedTouch - mapped touch found");
                return *touchIter;
            }
        }
        LOG_2("map::CompositePanel::findCloseMappedTouch - no close mapped touch found");
        return NO_MAPPED_TOUCH;
    }

    std::shared_ptr<MappedTouch> findMappedTouch(TouchWithDeltas& touch) {
        LOG_2("map::CompositePanel::findMappedTouch - touch to find: (x: " << touch.x_ << ",y:" << touch.y_
              << ",dx:" << touch.delta_x_ << ",dy:" << touch.delta_y_ << ")");
        for(auto touchIter = mappedTouches_.begin(); touchIter != mappedTouches_.end(); ++touchIter) {
            LOG_2("map::CompositePanel::findMappedTouch - mapped touch: (x: " << (*touchIter)->x_
                  << ",y:" << (*touchIter)->y_ << ",dx:" << (*touchIter)->delta_x_ << ",dy:" << (*touchIter)->delta_y_ << ")");
            if(fabs((*touchIter)->x_- touch.x_ + touch.delta_x_) <= EXPECTED_MEASUREMENT_ERROR &&
                    fabs((*touchIter)->y_- touch.y_ + touch.delta_y_) <= EXPECTED_MEASUREMENT_ERROR) {
                LOG_2("map::CompositePanel::findMappedTouch - mapped touch found");
                return *touchIter;
            }
        }
        return NO_MAPPED_TOUCH;
    }

    void interpolatePosition(MappedTouch& touch) {
        touch.x_ += touch.delta_x_;
        touch.y_ += touch.delta_y_;
        touch.numInterpolationSteps_ += 1;
    }

    void remapStartedTouches(Touches &collectedTouches, Touches &remappedTouches) {
        TouchWithDeltas transposedTouch;
        const std::vector<TouchWithDeltas> &newTouches = collectedTouches.getNewTouches();
        for(auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
            LOG_2("morph::CompositePanel::remapStartedTouches - creating new touch for panel " << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
            transposeTouch(*touchIter, transposedTouch);
            bool createNewMappingEntry = true;
            // a new touch in a transition area that is close to an existing touch in the transition area
            // moves that touch to the new position instead of creating a new touch.
            if(isInTransitionArea(transposedTouch)) {
                std::shared_ptr<MappedTouch> closeMappedTouch = findCloseMappedTouch(transposedTouch);
                LOG_2("mec::CompositePanel::remapStartedTouches - in transition. num mapped touches: " << mappedTouches_.size());
                if(closeMappedTouch != NO_MAPPED_TOUCH) {
                    LOG_2("mec::CompositePanel::remapStartedTouches - close touch found");
                    interpolatedTouches_.erase(closeMappedTouch);
                    float oldZ = closeMappedTouch->z_;
                    closeMappedTouch->update(transposedTouch);
                    closeMappedTouch->z_ = oldZ;
                    closeMappedTouch->numInterpolationSteps_ = 0;
                    LOG_2("mec::CompositePanel::remapStartedTouches - continued. num interp.steps: "  << closeMappedTouch->numInterpolationSteps_);
                    remappedTouches.addContinued(*closeMappedTouch);
                    createNewMappingEntry = false;
                } else {
                    LOG_2("mec::CompositePanel::remapStartedTouches - no close touch found");
                }
            }
            if(createNewMappingEntry) {
                std::shared_ptr<MappedTouch> mappedTouch;
                mappedTouch.reset(new MappedTouch());
                mappedTouch->update(transposedTouch);
                int touchID = *availableTouchIDs.begin();
                availableTouchIDs.erase(touchID);
                mappedTouch->id_ = touchID;
                mappedTouch->surface_ = getSurfaceID();
                mappedTouches_.insert(mappedTouch);
                LOG_2("mec::CompositePanel::remapStartedTouches - new. num mapped touches" << mappedTouches_.size() << " num interp.steps: " << mappedTouch->numInterpolationSteps_);
                remappedTouches.addNew(*mappedTouch);
            }
            LOG_2("morph::CompositePanel::remapStartedTouches - new touch created for panel " << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
        }
    }

    void remapEndedTouches(Touches &collectedTouches, Touches &remappedTouches) {
        TouchWithDeltas transposedTouch;
        const std::vector<TouchWithDeltas> &endedTouches = collectedTouches.getEndedTouches();
        for(auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
            LOG_2("morph::CompositePanel::remapEndedTouches - ending touch for panel " << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
            transposeTouch(*touchIter, transposedTouch);
            std::shared_ptr<MappedTouch> mappedTouch = findMappedTouch(transposedTouch);
            if(mappedTouch != NO_MAPPED_TOUCH) {
                // an ended touch in the transition area is interpolated for MAX_NUM_INTERPOLATION_STEPS before being
                // either taken over by a close new touch or being ended
                if (isInTransitionArea(transposedTouch) && mappedTouch->numInterpolationSteps_ < MAX_NUM_INTERPOLATION_STEPS) {
                    interpolatedTouches_.insert(mappedTouch);
                    LOG_2("mec::CompositePanel::remapEndedTouches - continued. num interp.steps: " << mappedTouch->numInterpolationSteps_);
                    remappedTouches.addContinued(*mappedTouch);
                } else {
                    std::shared_ptr<MappedTouch> &endedTouch = mappedTouch;
                    mappedTouches_.erase(mappedTouch);
                    endedTouch->update(transposedTouch);
                    LOG_2("mec::CompositePanel::remapEndedTouches - ended. num mapped touches: " << mappedTouches_.size() << " num interp.steps: " << endedTouch->numInterpolationSteps_);
                    remappedTouches.addEnded(*endedTouch);
                    availableTouchIDs.insert(endedTouch->id_);
                }
            }
            //else: nothing to do, touch was not mapped
            LOG_2("morph::CompositePanel::remapEndedTouches - touch ended for panel " << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
        }
    }

    void remapContinuedTouches(Touches &collectedTouches, Touches &remappedTouches) {
        TouchWithDeltas transposedTouch;
        const std::vector<TouchWithDeltas> &continuedTouches = collectedTouches.getContinuedTouches();
        for(auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
            LOG_2("morph::CompositePanel::remapContinuedTouches - continuing touch for panel " << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
            transposeTouch(*touchIter, transposedTouch);
            std::shared_ptr<MappedTouch> mappedTouch = findMappedTouch(transposedTouch);
            if(mappedTouch != NO_MAPPED_TOUCH) {
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
                remappedTouches.addContinued(*mappedTouch);
            } else {
                LOG_2("mec::CompositePanel::remapContinuedTouches - no mapped touch found for: (x:" << transposedTouch.x_ << ",y:" << transposedTouch.y_ << ")");
            }
            LOG_2("morph::CompositePanel::remapContinuedTouches - touch continued for panel " << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
        }
    }

    void remapInTransitionInterpolatedTouches(Touches &remappedTouches) {
        std::unordered_set<std::shared_ptr<MappedTouch>> endedInterpolatedTouches;
        for(auto touchIter = interpolatedTouches_.begin(); touchIter != interpolatedTouches_.end(); ++touchIter) {
            LOG_2("morph::CompositePanel::remapInTransitionInterpolatedTouches - transitioning touch for panel " << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
            // remaining continued touches inside the transpsition area (where the original touches were already ended
            // in a previous cycle) are still interpolated up to MAX_NUM_INTERPOLATION_STEPS times.
            // If they were not taken over by a close new touch by then they are ended.
            bool removeTouch = true;
            if(isInTransitionArea(**touchIter) && (*touchIter)->numInterpolationSteps_ < MAX_NUM_INTERPOLATION_STEPS) {
                interpolatePosition(**touchIter);
                LOG_2("mec::CompositePanel::remapInTransitionInterpolatedTouches - continued. num interp.steps: " << (*touchIter)->numInterpolationSteps_);
                remappedTouches.addContinued(**touchIter);
                removeTouch = false;
            } else {
                LOG_2("mec::CompositePanel::remapInTransitionInterpolatedTouches - not in transition area, ending");
            }
            if(removeTouch) {
                std::shared_ptr<MappedTouch> endedTouch = *touchIter;
                endedInterpolatedTouches.insert(endedTouch);
                mappedTouches_.erase(*touchIter);
                availableTouchIDs.insert(endedTouch->id_);
                LOG_2("mec::CompositePanel::remapInTransitionInterpolatedTouches - ended. mapped touches: " << mappedTouches_.size() << " num interp.steps: " << endedTouch->numInterpolationSteps_);
                LOG_2("mec::CompositePanel::remapInTransitionInterpolatedTouched - ended touch: (x:" << endedTouch->x_ << ",y:" << endedTouch->y_ << ")");
                remappedTouches.addEnded(*endedTouch);
            }
            LOG_2("morph::CompositePanel::remapInTransitionInterpolatedTouches - touch transitioned for panel " << getSurfaceID() << "; num mapped touches: " << mappedTouches_.size());
        }
        for(auto touchIter = endedInterpolatedTouches.begin(); touchIter != endedInterpolatedTouches.end(); ++touchIter) {
            interpolatedTouches_.erase(*touchIter);
        }
    }

    void transposeTouch(const TouchWithDeltas &collectedTouch, TouchWithDeltas &transposedTouch) {
        transposedTouch.id_ = collectedTouch.id_;
        LOG_2("mec::CompositePanel::transposeTouch - collectedTouch: (x:" << collectedTouch.x_ << ",y:" << collectedTouch.y_ << ",id:" << collectedTouch.id_<< ")");
        auto panelPositionIter = containedPanelsPositions_.find(collectedTouch.surface_);
        int panelPosition = 0;
        if(panelPositionIter != containedPanelsPositions_.end()) {
            panelPosition = panelPositionIter->second;
        } else {
            LOG_0("morph::CompositePanel::transposeTouch - surface " << collectedTouch.surface_
                                                                     << " is not registered for composite panel " << getSurfaceID()
                                                                     << "but still received a touch event from it");
        }
        transposedTouch.x_ = collectedTouch.x_ + panelPosition * PANEL_WIDTH;
        transposedTouch.y_ = collectedTouch.y_;
        transposedTouch.z_ = collectedTouch.z_;
        transposedTouch.surface_ = collectedTouch.surface_;
        transposedTouch.c_ = transposedTouch.y_;
        transposedTouch.r_ = transposedTouch.x_;
        transposedTouch.delta_x_ = collectedTouch.delta_x_;
        transposedTouch.delta_y_ = collectedTouch.delta_y_;
        transposedTouch.delta_z_ = collectedTouch.delta_z_;
    }
};

std::shared_ptr<CompositePanel::MappedTouch> CompositePanel::NO_MAPPED_TOUCH;

class OverlayFunction {
public:
    OverlayFunction(const std::string name, ISurfaceCallback &surfaceCallback, ICallback &callback)
            : name_(name), surfaceCallback_(surfaceCallback), callback_(callback)
    {}
    virtual bool init(const Preferences &preferences) = 0;
    virtual bool interpretTouches(const Touches &touches) = 0;
    const std::string& getName() const {
        return name_;
    }
protected:
    std::string name_;
    ISurfaceCallback &surfaceCallback_;
    ICallback &callback_;
};

class XYZPlaneOverlay : public OverlayFunction {
public:
    XYZPlaneOverlay(const std::string name, ISurfaceCallback &surfaceCallback, ICallback &callback)
            : OverlayFunction(name, surfaceCallback, callback)
    {}
    virtual bool init(const Preferences &preferences) {
        if(preferences.exists("semitones")) {
            semitones_ = preferences.getDouble("semitones");
        } else {
            semitones_ = 12;
            LOG_1("morph::OverlayFunction::init - property semitones not defined, using default 12");
        }

        if(preferences.exists("baseNote")) {
            baseNote_ = preferences.getDouble("baseNote");
        } else {
            baseNote_ = 32;
            LOG_1("morph::OverlayFunction::init - property baseNote not defined, using default 32");
        }
        return true;
    }
    virtual bool interpretTouches(const Touches &touches) {
        const std::vector<TouchWithDeltas> &newTouches = touches.getNewTouches();
        for (auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
            surfaceCallback_.touchOn(*touchIter);
            // remove as soon as surface support is fully implemented
            callback_.touchOn(touchIter->id_, xPosToNote(touchIter->x_), normalizeXPos(touchIter->x_),
                              normalizeYPos(touchIter->y_), normalizeZPos(touchIter->z_));
        }
        const std::vector<TouchWithDeltas> &continuedTouches = touches.getContinuedTouches();
        for (auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
            surfaceCallback_.touchContinue(*touchIter);
            // remove as soon as surface support is fully implemented
            callback_.touchContinue(touchIter->id_, xPosToNote(touchIter->x_), normalizeXPos(touchIter->x_),
                                    normalizeYPos(touchIter->y_), normalizeZPos(touchIter->z_));
        }
        const std::vector<TouchWithDeltas> &endedTouches = touches.getEndedTouches();
        for (auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
            surfaceCallback_.touchOff(*touchIter);
            // remove as soon as surface support is fully implemented
            callback_.touchOff(touchIter->id_, xPosToNote(touchIter->x_), normalizeXPos(touchIter->x_),
                               normalizeYPos(touchIter->y_), normalizeZPos(touchIter->z_));
        }
        return true;
    }
private:
    float semitones_;
    float baseNote_;

    float xPosToNote(float xPos) {
        return (xPos / PANEL_WIDTH * 12 + baseNote_); //TODO: semitones: get panel width for composite panels
    }

    float normalizeXPos(float xPos) {
        return (xPos / PANEL_WIDTH * 12);
    }

    float normalizeYPos(float yPos) {
        return yPos / PANEL_HEIGHT;
    }

    float normalizeZPos(float zPos) {
        return zPos / MAX_Z_PRESSURE;
    }
};

class OverlayFactory {
public:
    static std::unique_ptr<OverlayFunction> create(const std::string &overlayName, ISurfaceCallback &surfaceCallback_, ICallback &callback_) {
        std::unique_ptr<OverlayFunction> overlay;
        if(overlayName == "xyzplane") {
            overlay.reset(new XYZPlaneOverlay(overlayName, surfaceCallback_, callback_));
        }
        return overlay;
    }
};

class Instrument {
public:
    Instrument(const std::string name, std::shared_ptr<Panel> panel, std::unique_ptr<OverlayFunction> overlayFunction) :
            panel_(panel), name_(name), overlayFunction_(std::move(overlayFunction))
    {}
    bool process() {
        Touches touches;
        bool touchesRead = panel_->readTouches(touches);
        if(!touchesRead) {
            LOG_0("morph::Instrument::process - unable to read touches from panel " << panel_->getSurfaceID());
            return false;
        }
        bool touchesInterpreted = overlayFunction_->interpretTouches(touches);
        if(!touchesInterpreted) {
            LOG_0("morph::Instrument::process - unable to interpret touches from panel " << panel_->getSurfaceID() << "via overlay function" << overlayFunction_->getName());
            return false;
        }
        return true;
    }

    const std::string &getName() {
        return name_;
    }
private:
    std::string name_;
    std::shared_ptr<Panel> panel_;
    std::unique_ptr<OverlayFunction> overlayFunction_;
};

class Impl {
public:
    Impl(ISurfaceCallback &surfaceCallback, ICallback &callback) :
            surfaceCallback_(surfaceCallback), callback_(callback), active_(false) {}

    ~Impl() {}

    bool init(void *preferences) {
        Preferences prefs(preferences);

        if (active_) {
            LOG_2("morph::Impl::init - already active, calling deinit");
            deinit();
        }

        int numPanels = initPanels(prefs);
        if(numPanels < 1) {
            LOG_0("morph::Impl::init - umable to initialize panels, exiting");
            return false;
        }

        bool compositePanelsInitialized = initCompositePanels(prefs);
        if(!compositePanelsInitialized) {
            LOG_0("morph::Impl::init - umable to initialize composite panels, exiting");
            return false;
        }

        bool instrumentsInitialized = initInstruments(prefs);
        if(!instrumentsInitialized) {
            LOG_0("morph::Impl::init - umable to initialize instruments, exiting");
            return false;
        }

        LOG_1("Morph - " << numPanels << " panels initialized");

        active_ = true;
        return active_;
    }

    void deinit() {
        panels_.clear();
        LOG_1("Morph - panels freed");
    }

    bool isActive() {
        return active_;
    }

    bool process() {
        bool allProcessingSucceeded = true;
        for(auto instrumentIter = instruments_.begin(); instrumentIter != instruments_.end(); ++instrumentIter) {
            bool processSucceeded = (*instrumentIter)->process();
            if(!processSucceeded) {
                LOG_0("unable to process touches for instrument " << (*instrumentIter)->getName());
                allProcessingSucceeded = false;
            }
        }
        return allProcessingSucceeded;
    }

private:
    ICallback &callback_;
    ISurfaceCallback &surfaceCallback_;
    bool active_;
    std::unordered_set<std::shared_ptr<Panel>> panels_;
    std::unordered_set<std::unique_ptr<Instrument>> instruments_;

    int initPanels(Preferences& preferences) {
        Preferences panelsPrefs(preferences.getSubTree("panels"));
        if (!panelsPrefs.valid()) {
            LOG_0("morph::Impl::initPanels - no panels defined, in preferences, add 'panels' section.");
            return 0;
        }

        std::vector<std::string> remainingPanelNames = panelsPrefs.getKeys();

        SenselDeviceList device_list;

        SenselStatus deviceStatus = senselGetDeviceList(&device_list);
        if (deviceStatus != SENSEL_OK || device_list.num_devices == 0) {
            LOG_0("morph::Impl::initPanels - unable to detect Sensel Morph panel(s), calling deinit");
            deinit();
            return 0;
        }

        int numPanels = 0;
        for (int i = 0; i < device_list.num_devices; ++i) {
            bool deviceFound = false;
            // we run four matching passes.
            // 0) First we look for panel entries where both serial number and overlayId match,
            // 1) then for entries with at least the serial number (and no overlay specified),
            // 2) then for entries with at least the overlayId (and no serial number specified),
            // 3) then for entries that have neither serial number nor overlayId specified
            for(int matchingPass = 0; matchingPass < 4 && !deviceFound; ++matchingPass) {
                for (auto panelNameIter = remainingPanelNames.begin(); panelNameIter != remainingPanelNames.end() && !deviceFound;) {
                    Preferences panelPrefs(panelsPrefs.getSubTree(*panelNameIter));
                    const std::string &serial = panelPrefs.getString("serial", "NO_SERIAL");
                    if (matchingPass > 1 || std::strcmp(serial.c_str(), (char *) device_list.devices[i].serial_num) == 0) {
                        const std::string &overlayId = panelPrefs.getString("overlayId", "NO_OVERLAY_ID");
                        //if(matchingPass > 0 || std::strcmp(overlayId.c_str(), (char*) device_list.devices[i].overlay_id) == 0) {} -- overlay id not retrievable yet via public API, always match for the moment
                        {
                            initPanel(device_list.devices[i], *panelNameIter);
                            panelNameIter = remainingPanelNames.erase(panelNameIter);
                            ++numPanels;
                            deviceFound = true;
                            break;
                        }
                    }
                    ++panelNameIter;
                }
            }
        }
        return numPanels;
    }

    bool initCompositePanels(Preferences &preferences) {
        Preferences compositePanelsPrefs(preferences.getSubTree("composite panels"));
        if (!compositePanelsPrefs.valid()) {
            LOG_2("morph::Impl::initCompositePanels - no composite panels defined in preferences.");
            return true;
        }

        const std::vector<std::string> &compositePanelNames = compositePanelsPrefs.getKeys();

        for(auto compositePanelNameIter = compositePanelNames.begin(); compositePanelNameIter != compositePanelNames.end(); ++compositePanelNameIter) {
            const std::string &compositePanelName = *compositePanelNameIter;
            Preferences compositePanelPrefs(compositePanelsPrefs.getSubTree(compositePanelName));
            if (!compositePanelPrefs.valid()) {
                LOG_2("morph::Impl::initCompositePanels - cannot read composite panel definition for " << compositePanelName);
                continue;
            }
            void* subPanelNames = compositePanelPrefs.getArray("panels");
            if(subPanelNames == nullptr) {
                LOG_0("morph::Impl::initCompositePanels - composite panel definition '" << compositePanelName << "' is missing the 'panels' list");
                continue; //perhaps we find other valid composite panel definitions
            }
            std::vector<std::shared_ptr<Panel>> panels;
            for (int i = 0; i < compositePanelPrefs.getArraySize(subPanelNames); i++) {
                SurfaceID panelID = compositePanelPrefs.getArrayString(subPanelNames, i, "");
                for(auto panelIter = panels_.begin(); panelIter != panels_.end(); ++panelIter) {
                    if((*panelIter)->getSurfaceID() == panelID) {
                        panels.push_back(*panelIter);
                        break;
                    }
                }
            }
            std::string transition_area_properyname = "transitionAreaWidth";
            int transitionAreaWidth;
            if(compositePanelPrefs.exists(transition_area_properyname)) {
                transitionAreaWidth = compositePanelPrefs.getInt(transition_area_properyname);
            } else {
                transitionAreaWidth = 20;
                LOG_1("morph::Impl::init - property transition_area_width not defined, using default 20");
            }
            std::shared_ptr<Panel> compositePanel;
            compositePanel.reset(new CompositePanel(*compositePanelNameIter, panels, transitionAreaWidth));
            bool initSuccessful = compositePanel->init();
            if (initSuccessful) {
                panels_.insert(compositePanel);
            } else {
                LOG_0("morph::Impl::initCompositePanels - unable to initialize panel " << *compositePanelNameIter);
            }
        }
        return true;
    }

    bool initInstruments(Preferences &preferences) {
        Preferences instrumentsPrefs(preferences.getSubTree("instruments"));
        if (!instrumentsPrefs.valid()) {
            LOG_0("morph::Impl::initInstruments - no instruments defined in preferences, add 'instruments' subsection.");
            return false;
        }

        const std::vector<std::string> &instrumentNames = instrumentsPrefs.getKeys();
        for(auto instrumentNameIter = instrumentNames.begin(); instrumentNameIter != instrumentNames.end(); ++instrumentNameIter) {
            Preferences instrumentPrefs(instrumentsPrefs.getSubTree(*instrumentNameIter));
            if(!instrumentPrefs.valid()) {
                LOG_0("morph::Impl::initInstruments - unable to read preferences for instrument " << *instrumentNameIter);
                continue;
            }
            SurfaceID panelID;
            if(instrumentPrefs.exists("panel")) {
                panelID = instrumentPrefs.getString("panel");
            } else {
                LOG_0("morph::Impl::initInstruments - panel not defined, add 'panel' mapping to valid panel id");
                continue;
            }
            std::shared_ptr<Panel> panel;
            for(auto panelIter = panels_.begin(); panelIter != panels_.end(); ++panelIter) {
                if((*panelIter)->getSurfaceID() == panelID) {
                    panel = *panelIter;
                    break;
                }
            }
            if(!panel) {
                LOG_0("morph::Impl::initInstruments - unable to find panel with id " << panelID);
                continue;
            }

            Preferences overlayPrefs(instrumentPrefs.getSubTree("overlay"));
            if(!overlayPrefs.valid()) {
                LOG_0("morph::Impl::initInstruments - unable to find overlay preferences for instrument '" << *instrumentNameIter << "'");
                continue;
            }
            std::string overlayName;
            if(overlayPrefs.exists("name")) {
                overlayName = overlayPrefs.getString("name");
            } else {
                LOG_0("morph::Impl::initInstruments - unable to find overlay name preferences for instrument '" << *instrumentNameIter
                                                                                                               << "'. add 'name' entry to overlay section");
                continue;
            }

            Preferences overlayParamPrefs(overlayPrefs.getSubTree("params"));
            if(!overlayParamPrefs.valid()) {
                LOG_0("morph::Impl::initInstruments - unable to find overlay parameter preferences for instrument '" << *instrumentNameIter
                << "', add 'param' section to overlay section");
                continue;
            }
            std::unique_ptr<OverlayFunction> overlayFunction;
            overlayFunction = std::move(OverlayFactory::create(overlayName, surfaceCallback_, callback_));
            overlayFunction->init(overlayParamPrefs);
            std::unique_ptr<Instrument> instrument;
            instrument.reset(new Instrument(*instrumentNameIter, panel, std::move(overlayFunction)));
            instruments_.insert(std::move(instrument));
        }
        return true;
    }

    void initPanel(const SenselDeviceID &deviceID, const std::string& panelName) {
        LOG_1("morph::Impl::init - device id for panel " << panelName << ": (idx:" << +deviceID.idx
                                                         << ",com_port:" << deviceID.com_port
                                                         << ",serial_number:" << deviceID.serial_num << ")");
        std::shared_ptr<Panel> panel;
        panel.reset(new SinglePanel(deviceID, panelName));
        bool initSuccessful = panel->init();
        if (initSuccessful) {
            panels_.insert(panel);
        } else {
            LOG_0("morph::Impl::init - unable to initialize panel " << panelName);
        }
    }
};

} // namespace morph

Morph::Morph(ISurfaceCallback &surfaceCallback, ICallback &callback) :
    morphImpl_(new morph::Impl(surfaceCallback, callback)) {}

Morph::~Morph() {}

bool Morph::init(void *preferences) {
    return morphImpl_->init(preferences);
}

bool Morph::process() {
    return morphImpl_->process();
}

void Morph::deinit() {
    morphImpl_->deinit();
}

bool Morph::isActive() {
    return morphImpl_->isActive();
}

} // namespace mec