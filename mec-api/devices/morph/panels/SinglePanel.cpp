#include "SinglePanel.h"
#include "../../../mec_log.h"

namespace mec {
namespace morph {

SinglePanel::SinglePanel(const SenselDeviceID &deviceID, const SurfaceID &surfaceID) : Panel(surfaceID),
                                                                          deviceID_(deviceID),
                                                                          handle_(nullptr),
                                                                          frameData_(nullptr) {}

bool SinglePanel::init() {
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
    LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": device_id: "
                                                        << +firmwareInfo.device_id);
    LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": revision: "
                                                        << +firmwareInfo.device_revision);
    LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": protocol version: "
                                                        << +firmwareInfo.fw_protocol_version);
    LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": fw_version_build: "
                                                        << +firmwareInfo.fw_version_build);
    LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": fw_version_major: "
                                                        << +firmwareInfo.fw_version_major);
    LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": fw_version_minor: "
                                                        << +firmwareInfo.fw_version_minor);
    LOG_1("morph::Panel::init firmware info for panel " << getSurfaceID() << ": fw_version_release: "
                                                        << +firmwareInfo.fw_version_release);
    SenselStatus setFrameContentStatus = senselSetFrameContent(handle_, FRAME_CONTENT_CONTACTS_MASK);
    if (setFrameContentStatus != SENSEL_OK) {
        LOG_0("morph::Panel::init - unable to set frame content status for panel " << getSurfaceID());
        return false;
    }
    SenselStatus allocFrameStatus = senselAllocateFrameData(handle_, &frameData_);
    if (allocFrameStatus != SENSEL_OK) {
        LOG_0("morph::Panel::init - unable to allocate frame data for panel " << getSurfaceID());
        return false;
    }
    unsigned char contacts_mask = 0;
    SenselStatus getContactsMaskStatus = senselGetContactsMask(handle_, &contacts_mask);
    if (allocFrameStatus != SENSEL_OK) {
        LOG_0("morph::Panel::init - unable to get contacts mask for panel " << getSurfaceID());
        return false;
    }
    SenselStatus setContactsMaskStatus = senselSetContactsMask(handle_,
                                                               contacts_mask | CONTACT_MASK_DELTAS);
    if (allocFrameStatus != SENSEL_OK) {
        LOG_0("morph::Panel::init - unable to set contacts mask for panel " << getSurfaceID());
        return false;
    }
    senselStartScanning(handle_);
    LOG_2("morph::Panel::init - panel " << getSurfaceID() << " initialized");
    return true;
}

SinglePanel::~SinglePanel() {
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

bool SinglePanel::readTouches(Touches &touches) {
    LOG_3("morph::SinglePanel::readTouches - reading touches for panel " << getSurfaceID());
    bool sensorReadState = senselReadSensor(handle_);
    if (sensorReadState != SENSEL_OK) {
        LOG_0("morph::SinglePanel::readTouches - unable to read sensor for panel " << getSurfaceID());
        return false;
    }
    unsigned int num_frames = 0;
    bool numFramesRetrievedState = senselGetNumAvailableFrames(handle_, &num_frames);
    if (numFramesRetrievedState != SENSEL_OK) {
        LOG_0("morph::SinglePanel::readTouches - unable to determine number of frames for panel "
                      << getSurfaceID());
        return false;
    }
    for (int current_frame = 0; current_frame < num_frames; ++current_frame) {
        bool frameRetrievedState = senselGetFrame(handle_, frameData_);
        if (frameRetrievedState != SENSEL_OK) {
            LOG_0("morph::SinglePanel::readTouches - unable to get frame data for panel "
                          << getSurfaceID());
            continue; // better try to continue, other frames might be usable
        }
        for (int current_contact = 0; current_contact < frameData_->n_contacts; ++current_contact) {
            unsigned int state = frameData_->contacts[current_contact].state;
            float x = frameData_->contacts[current_contact].x_pos;
            float x_delta = frameData_->contacts[current_contact].delta_x;
            float y = frameData_->contacts[current_contact].y_pos;
            float y_delta = frameData_->contacts[current_contact].delta_y;
            float z = frameData_->contacts[current_contact].total_force;
            float z_delta = frameData_->contacts[current_contact].delta_force;
            float c = y;
            float r = x;
            std::string stateString;
            int touchid = frameData_->contacts[current_contact].id;
            std::shared_ptr <TouchWithDeltas> touch;
            LOG_2("---");
            LOG_2("mec::SinglePanel::readTouches(o): (id:" << touchid << ",x: " << x << ",y:" << y
                                                           << ",dx:" << x_delta
                                                           << ",dy:" << y_delta
                                                           << ",dz:" << z_delta
                                                           << ",state:" << state << ")");
            // not using the frameData (x, y) deltas for now because they are smaller than expected - for z we don't have a choice
            x_delta = 0;
            y_delta = 0;
            touch.reset(
                    new TouchWithDeltas(touchid, getSurfaceID(), x, y, z, r, c, x_delta, y_delta, z_delta));
            std::shared_ptr <TouchWithDeltas> foundTouch;
            switch (state) {
                case CONTACT_START: // 1
                    foundTouch = touch;
                    touches.addNew(foundTouch);
                    stateString = "CONTACT_START";
                    activeTouches_[touchid] = touch;
                    break;
                case CONTACT_MOVE: // 2
                {
                    // delta_x, delta_y and delta_z as returned from the firmware
                    // are either 0 or way off atm. (TODO: investigate/report)
                    // thus we have to calculate the deltas ourselves until this is fixed
                    // by memorizing the last (x,y,z) position per touchid
                    foundTouch = activeTouches_[touchid];
                    updateDeltas(touch, foundTouch);
                    if (!foundTouch) {
                        foundTouch = touch;
                        LOG_0("mec::SinglePanel::readTouches CONTACT_MOVE - unable to find active touch for end event - this shouldn't happen.");
                    }
                    touches.addContinued(foundTouch);
                    stateString = "CONTACT_MOVE";
                    break;
                }
                case CONTACT_END: // 3
                {
                    foundTouch = activeTouches_[touchid];
                    updateDeltas(touch, foundTouch);
                    if (foundTouch) {
                        activeTouches_.erase(touchid);
                    } else {
                        foundTouch = touch;
                        LOG_0("mec::SinglePanel::readTouches CONTACT_END - unable to find active touch for end event - this shouldn't happen.");
                    }
                    touches.addEnded(foundTouch);
                    stateString = "CONTACT_END";
                    break;
                }
                case CONTACT_INVALID: // 0
                default:
                    stateString = "CONTACT_INVALID";
                    LOG_0("morph::SinglePanel::readTouches - invalid/unexpected touch state " << state);
                    // better try to continue, other contacts might be usable
            }
            LOG_2("mec::SinglePanel::readTouches(m): (id:" << touchid << ",x: " << foundTouch->x_ << ",y:"
                                                           << foundTouch->y_
                                                           << ",dx:" << foundTouch->delta_x_
                                                           << ",dy:" << foundTouch->delta_y_
                                                           << ",dz:" << foundTouch->delta_z_
                                                           << ",state:" << stateString << ")");
        }
    }
    LOG_3("morph::SinglePanel::readTouches - touches read for panel " << getSurfaceID());
    return true;
}

void SinglePanel::updateDeltas(const std::shared_ptr <TouchWithDeltas> &touch,
                  std::shared_ptr <TouchWithDeltas> foundTouch) const {
    if (foundTouch) {
        foundTouch->delta_x_ = touch->x_ - foundTouch->x_;
        foundTouch->delta_y_ = touch->y_ - foundTouch->y_;
        foundTouch->delta_z_ = touch->z_ - foundTouch->z_;
        LOG_2("mec::updateDeltas - old x:" << foundTouch->x_ << ",new x:" << touch->x_ << ",delta_x:"
                                           << foundTouch->delta_x_);
        LOG_2("mec::updateDeltas - old y:" << foundTouch->y_ << ",new y:" << touch->x_ << ",delta_y:"
                                           << foundTouch->delta_y_);
        LOG_2("mec::updateDeltas - old z:" << foundTouch->z_ << ",new z:" << touch->x_ << ",delta_z:"
                                           << foundTouch->delta_z_);
        foundTouch->x_ = touch->x_;
        foundTouch->y_ = touch->y_;
        foundTouch->z_ = touch->z_;
    } else {
        LOG_0("mec::updateDeltas - unable to find active touch for move event - this shouldn't happen.");
        foundTouch = touch;
    }
}

}
}