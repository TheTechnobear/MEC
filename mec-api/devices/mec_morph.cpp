#include "mec_morph.h"
#include "../mec_log.h"

#include <src/sensel.h>

#include <vector>
#include <memory>
#include <unordered_set>
#include <cmath>
#include <set>

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
    void addNew(TouchWithDeltas& touch) {
        newTouches_.push_back(touch);
    }

    void addContinued(TouchWithDeltas& touch) {
        continuedTouches_.push_back(touch);
    }

    void addEnded(TouchWithDeltas& touch) {
        endedTouches_.push_back(touch);
    }

    std::vector<TouchWithDeltas>& getNewTouches() {
        return newTouches_;
    }

    std::vector<TouchWithDeltas>& getContinuedTouches() {
        return continuedTouches_;
    }

    std::vector<TouchWithDeltas>& getEndedTouches() {
        return endedTouches_;
    }

    void clear() {
        newTouches_.clear();
        continuedTouches_.clear();
        endedTouches_.clear();
    }
    
    void add(Touches& touches) {
        std::vector<TouchWithDeltas> &newTouches = touches.getNewTouches();
        for(auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
            addNew(*touchIter);
        }
        std::vector<TouchWithDeltas> &continuedTouches = touches.getContinuedTouches();
        for(auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
            addContinued(*touchIter);
        }
        std::vector<TouchWithDeltas> &endedTouches = touches.getEndedTouches();
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
     virtual bool init() = 0;
     virtual bool readTouches(Touches &touches) = 0;
};

class SinglePanel : public Panel {
public:
    SinglePanel(unsigned char index, std::string surfaceID) : index_(index), handle_(nullptr), frameData_(nullptr),
                             surfaceID_(surfaceID) /*TODO*/ {}

    virtual bool init() {
        SenselStatus openStatus = senselOpenDeviceByID(&handle_, index_);
        if (openStatus != SENSEL_OK) {
            LOG_0("morph::Panel::init - unable to open device with id " << index_);
            return false;
        }
        SenselStatus setFrameContentStatus = senselSetFrameContent(handle_, FRAME_CONTENT_CONTACTS_MASK);
        if (setFrameContentStatus != SENSEL_OK) {
            LOG_0("morph::Panel::init - unable to set contacts mask for panel " << index_);
            return false;
        }
        SenselStatus allocFrameStatus = senselAllocateFrameData(handle_, &frameData_);
        if (allocFrameStatus != SENSEL_OK) {
            LOG_0("morph::Panel::init - unable to allocate frame data for panel " << index_);
            return false;
        }
        senselStartScanning(handle_);
        LOG_2("morph::Panel::init - panel " << index_ << " initialized");
        return true;
    }

    ~SinglePanel() {
        if (handle_ != nullptr) {
            SenselStatus stopStatus = senselStopScanning(handle_);
            //TODO - should we wait?
            if (stopStatus != SENSEL_OK) {
                LOG_0("morph::Panel::destructor - unable to stop panel " << index_);
            }
            if (frameData_ != nullptr) {
                SenselStatus freeFrameStatus = senselFreeFrameData(handle_, frameData_);
                if (freeFrameStatus != SENSEL_OK) {
                    LOG_0("morph::Panel::destructor - unable to free frame data for panel " << index_);
                }
            }
            SenselStatus closeStatus = senselClose(handle_);
            if (closeStatus != SENSEL_OK) {
                LOG_0("morph::Panel::destructor - unable to close panel " << index_);
            }
        }
        LOG_2("morph::Panel::destructor - panel " << index_ << " freed");
    }

    virtual bool readTouches(Touches &touches) {
        bool sensorReadState = senselReadSensor(handle_);
        if(sensorReadState != SENSEL_OK) {
            LOG_0("morph::Panel::readTouches - unable to read sensor for panel " << index_);
            return false;
        }
        unsigned int num_frames = 0;
        bool numFramesRetrievedState = senselGetNumAvailableFrames(handle_, &num_frames);
        if(numFramesRetrievedState != SENSEL_OK) {
            LOG_0("morph::Panel::readTouches - unable to determine number of frames for panel " << index_);
            return false;
        }
        for(int current_frame = 0; current_frame < num_frames; ++current_frame) {
            bool frameRetrievedState = senselGetFrame(handle_, frameData_);
            if(frameRetrievedState != SENSEL_OK) {
                LOG_0("morph::Panel::readTouches - unable to get frame data for panel " << index_);
                continue; // better try to continue, other frames might be usable
            }
            for(int current_contact = 0; current_contact < frameData_->n_contacts; ++current_contact) {
                unsigned int state = frameData_->contacts[current_contact].state;
                float x = frameData_->contacts[current_contact].x_pos;
                float x_delta = frameData_->contacts[current_contact].delta_x;
                float y = frameData_->contacts[current_contact].y_pos;
                float y_delta = frameData_->contacts[current_contact].delta_y;
                float z = frameData_->contacts[current_contact].total_force;
                float z_delta = frameData_->contacts[current_contact].delta_force;
                float c = y;
                float r = x;
                int touchid = current_contact + index_ * MAX_NUM_CONTACTS_PER_PANEL;
                TouchWithDeltas touch(touchid, surfaceID_, x, y, z, r, c, x_delta, y_delta, z_delta);
                switch(state) {
                    case 1: // CONTACT_START
                        touches.addNew(touch);
                        break;
                    case 2: // CONTACT_MOVE
                        touches.addContinued(touch);
                        break;
                    case 3: // CONTACT_END
                        touches.addEnded(touch);
                        break;
                    case 0: // CONTACT_INVALID
                    default:
                        LOG_0("morph::Panel::readTouches - invalid/unexpected touch state " << state);
                        // better try to continue, other contacts might be usable
                }
            }
        }
        return true;
    }

private:
    SENSEL_HANDLE handle_;
    SenselFrameData *frameData_;
    unsigned char index_;
    SurfaceID surfaceID_;
};

class CompositePanel {
public:
    CompositePanel(std::string surfaceID, int transitionAreaWidth) :
            surfaceID_(surfaceID), transitionAreaWidth_(transitionAreaWidth)
    {
        for(int i = 0; i < MAX_NUM_CONCURRENT_OVERALL_CONTACTS; ++i) {
            availableTouchIDs.insert(i);
        }
    }

    void remapTouches(Touches& collectedTouches, Touches& remappedTouches) {
        remappedTouches.clear();

        remapEndedTouches(collectedTouches, remappedTouches);

        remapContinuedTouches(collectedTouches, remappedTouches);

        remapStartedTouches(collectedTouches, remappedTouches);

        remapInTransitionInterpolatedTouches(remappedTouches);
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

    bool isInTransitionArea(TouchWithDeltas& touch) {
        double xpos_on_panel = fmod(touch.x_, (float)PANEL_WIDTH);
        return xpos_on_panel >= PANEL_WIDTH - transitionAreaWidth_ / 2 ||
                xpos_on_panel <= transitionAreaWidth_ / 2;
    }

    std::shared_ptr<MappedTouch> findCloseMappedTouch(TouchWithDeltas& touch) {
        LOG_2("map::CompositePanel::findCloseMappedTouch - touch to find: (x: " << touch.x_ << ",y:" << touch.y_ << ")");
        for(auto touchIter = mappedTouches_.begin(); touchIter != mappedTouches_.end(); ++touchIter) {
            LOG_2("map::CompositePanel::findCloseMappedTouch - mapped touch: (x: " << (*touchIter)->x_ << ",y:" << (*touchIter)->y_ << ")");
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
        //LOG_2("map::CompositePanel::findMappedTouch - touch to find: (x: " << touch.x_ << ",y:" << touch.y_
        //      << ",dx:" << touch.delta_x_ << ",dy:" << touch.delta_y_ << ")");
        for(auto touchIter = mappedTouches_.begin(); touchIter != mappedTouches_.end(); ++touchIter) {
            //LOG_2("map::CompositePanel::findCloseMappedTouch - mapped touch: (x: " << (*touchIter)->x_
            //      << ",y:" << (*touchIter)->y_ << ",dx:" << (*touchIter)->delta_x_ << ",dy:" << (*touchIter)->delta_y_ << ")");
            if(fabs((*touchIter)->x_- touch.x_ + touch.delta_x_) <= EXPECTED_MEASUREMENT_ERROR &&
                    fabs((*touchIter)->y_- touch.y_ + touch.delta_y_) <= EXPECTED_MEASUREMENT_ERROR) {
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
        std::vector<TouchWithDeltas> &newTouches = collectedTouches.getNewTouches();
        for(auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
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
                mappedTouch->surface_ = surfaceID_;
                mappedTouches_.insert(mappedTouch);
                LOG_2("mec::CompositePanel::remapStartedTouches - new. num mapped touches" << mappedTouches_.size() << " num interp.steps: " << mappedTouch->numInterpolationSteps_);
                remappedTouches.addNew(*mappedTouch);
            }
        }
    }

    void remapEndedTouches(Touches &collectedTouches, Touches &remappedTouches) {
        TouchWithDeltas transposedTouch;
        std::vector<TouchWithDeltas> &endedTouches = collectedTouches.getEndedTouches();
        for(auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
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
        }
    }

    void remapContinuedTouches(Touches &collectedTouches, Touches &remappedTouches) {
        TouchWithDeltas transposedTouch;
        std::vector<TouchWithDeltas> &continuedTouches = collectedTouches.getContinuedTouches();
        for(auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
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
        }
    }

    void remapInTransitionInterpolatedTouches(Touches &remappedTouches) {
        std::unordered_set<std::shared_ptr<MappedTouch>> endedInterpolatedTouches;
        for(auto touchIter = interpolatedTouches_.begin(); touchIter != interpolatedTouches_.end(); ++touchIter) {
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
        }
        for(auto touchIter = endedInterpolatedTouches.begin(); touchIter != endedInterpolatedTouches.end(); ++touchIter) {
            interpolatedTouches_.erase(*touchIter);
        }
    }

    std::string surfaceID_;
    float transitionAreaWidth_;

    void transposeTouch(Touch &collectedTouch, Touch &transposedTouch) {
        transposedTouch.id_ = collectedTouch.id_;
        //LOG_2("mec::CompositePanel::transposeTouch - collectedTouch: (x:" << collectedTouch.x_ << ",y:" << collectedTouch.y_ << ",id:" << collectedTouch.id_<< ")");
        transposedTouch.x_ = collectedTouch.x_ + (collectedTouch.id_ / 16) * PANEL_WIDTH;
        transposedTouch.y_ = collectedTouch.y_;
        transposedTouch.z_ = collectedTouch.z_;
        transposedTouch.surface_ = collectedTouch.surface_;
        transposedTouch.c_ = transposedTouch.y_;
        transposedTouch.r_ = transposedTouch.x_;
    }

    std::unordered_set<std::shared_ptr<MappedTouch>> mappedTouches_;
    std::unordered_set<std::shared_ptr<MappedTouch>> interpolatedTouches_;
    std::set<int> availableTouchIDs;
    static std::shared_ptr<MappedTouch> NO_MAPPED_TOUCH;
};

std::shared_ptr<CompositePanel::MappedTouch> CompositePanel::NO_MAPPED_TOUCH;

class Impl {
public:
    Impl(ISurfaceCallback &surfaceCallback, ICallback &callback) :
            surfaceCallback_(surfaceCallback), callback_(callback), active_(false), base_note_(64.0) {}

    ~Impl() {}

    bool init(void *preferences) {
        Preferences prefs(preferences);

        if (active_) {
            LOG_2("morph::Impl::init - already active, calling deinit");
            deinit();
        }

        SenselDeviceList device_list;

        SenselStatus deviceStatus = senselGetDeviceList(&device_list);
        if (deviceStatus != SENSEL_OK || device_list.num_devices == 0) {
            LOG_0("morph::Impl::init - unable to detect Sensel Morph panel(s), calling deinit");
            deinit();
            return false;
        }

        panels_.reset(new std::vector<std::unique_ptr<SinglePanel>>());
        for (int i = 0; i < device_list.num_devices; ++i) {
            std::string surfaceID;
            std::string surfaceid_propertyname = "surface_for_panel" + std::to_string(i);
            if(prefs.exists(surfaceid_propertyname)) {
                surfaceID = prefs.getString(surfaceid_propertyname);
            } else {
                LOG_1("morph::Impl::init - property surface_for_panel" << i << " not defined, using surface " << i);
                surfaceID = std::to_string(i);
            }
            std::unique_ptr<SinglePanel> panel;
            panel.reset(new SinglePanel(device_list.devices[i].idx, surfaceID));
            bool initSuccessful = panel->init();
            if (initSuccessful) {
                panels_->push_back(std::move(panel));
            } else {
                LOG_0("morph::Impl::init - unable to initialize panel " << i);
            }
        }

        std::string transition_area_properyname = "transition_area_width";
        int transitionAreaWidth;
        if(prefs.exists(transition_area_properyname)) {
            transitionAreaWidth = prefs.getInt(transition_area_properyname);
        } else {
            transitionAreaWidth = 20;
            LOG_1("morph::Impl::init - property transition_area_width not defined, using default 20");
        }
        std::string composite_surface_id_properyname = "composite_surface_id";
        std::string composite_surfaceID;
        if(prefs.exists(composite_surface_id_properyname)) {
            composite_surfaceID = prefs.getInt(composite_surface_id_properyname);
        } else {
            composite_surfaceID = std::to_string(3);
            LOG_1("morph::Impl::init - property composite_surface_id_properyname not defined, using default 3");
        }

        size_t num_panels = panels_->size();
        compositePanel_.reset(new CompositePanel(composite_surfaceID, transitionAreaWidth));

        std::string base_note_propertyname = "base_note";
        if(prefs.exists(base_note_propertyname)) {
            base_note_ = stof(prefs.getString(base_note_propertyname));
        } else {
            LOG_1("morph::Impl::init - property base_note not defined, using default " << base_note_);
        }

        LOG_1("Morph - " << num_panels << " panels initialized");

        active_ = true;
        return active_;
    }

    void deinit() {
        compositePanel_.release();
        panels_.release();
        LOG_1("Morph - panels freed");
    }

    bool isActive() {
        return active_;
    }

    float FIX = 0.5; //TODO: why is one semitone 0.5 instead of 1.0?
    float xPosToNote(float xPos) {
        return (xPos / PANEL_WIDTH * 12 + base_note_) * FIX;
    }

    float normalizeXPos(float xPos) {
        return (xPos / PANEL_WIDTH * 12) * FIX;
    }

    float normalizeYPos(float yPos) {
        return yPos / PANEL_HEIGHT;
    }

    float normalizeZPos(float zPos) {
        return zPos / MAX_Z_PRESSURE;
    }

    int currnote = 0;

    bool process() {
        Touches touches;
        Touches allTouches;
        // send touch events for individual panels...
        for (auto panelIter = panels_->begin(); panelIter != panels_->end(); ++panelIter) {
            bool touchesRead = (*panelIter)->readTouches(touches);
            /*touchesRead = true;
            TouchWithDeltas touch = TouchWithDeltas(0, "surfacex", currnote, 2, 3, currnote, 2, 0, 0, 0);
            currnote += 1;
            if(currnote > 100) currnote = 0;
            touches.addNew(touch);*/
            /*if(touchesRead) {
                std::vector<TouchWithDeltas> &newTouches = touches.getNewTouches();
                for (auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
                    surfaceCallback_.touchOn(*touchIter);
                }
                std::vector<TouchWithDeltas> &continuedTouches = touches.getContinuedTouches();
               for (auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
                   surfaceCallback_.touchContinue(*touchIter);
                }
                std::vector<TouchWithDeltas> &endedTouches = touches.getEndedTouches();
                for (auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
                    surfaceCallback_.touchOff(*touchIter);
                }
            }*/
            allTouches.add(touches);
            touches.clear();
        }

        // ...and for composite panel
        Touches remappedTouches;
        compositePanel_->remapTouches(allTouches, remappedTouches);
        std::vector<TouchWithDeltas> &newTouches = remappedTouches.getNewTouches();
        for (auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
            surfaceCallback_.touchOn(*touchIter);
            // remove as soon as surface support is fully implemented
            callback_.touchOn(touchIter->id_, xPosToNote(touchIter->x_), normalizeXPos(touchIter->x_),
                              normalizeYPos(touchIter->y_), normalizeZPos(touchIter->z_));
        }
        std::vector<TouchWithDeltas> &continuedTouches = remappedTouches.getContinuedTouches();
        for (auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
            surfaceCallback_.touchContinue(*touchIter);
            // remove as soon as surface support is fully implemented
            callback_.touchContinue(touchIter->id_, xPosToNote(touchIter->x_), normalizeXPos(touchIter->x_),
                                    normalizeYPos(touchIter->y_), normalizeZPos(touchIter->z_));
        }
        std::vector<TouchWithDeltas> &endedTouches = remappedTouches.getEndedTouches();
        for (auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
            surfaceCallback_.touchOff(*touchIter);
            // remove as soon as surface support is fully implemented
            callback_.touchOff(touchIter->id_, xPosToNote(touchIter->x_), normalizeXPos(touchIter->x_),
                               normalizeYPos(touchIter->y_), normalizeZPos(touchIter->z_));
        }
        allTouches.clear();
        return true;
    }

private:
    ICallback &callback_;
    ISurfaceCallback &surfaceCallback_;
    bool active_;
    std::unique_ptr<CompositePanel> compositePanel_;
    std::unique_ptr<std::vector<std::unique_ptr<SinglePanel>>> panels_;
    float base_note_;
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