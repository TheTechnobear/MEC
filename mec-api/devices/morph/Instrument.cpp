#include "Instrument.h"
#include "../../mec_log.h"

namespace mec {
namespace morph {


Instrument::Instrument(const std::string name, std::shared_ptr <Panel> panel, std::unique_ptr <OverlayFunction> overlayFunction)
        :
        panel_(panel), name_(name), overlayFunction_(std::move(overlayFunction)) {}

bool Instrument::process() {
    LOG_3("morph::Instrument::process - reading touches");
    Touches touches;
    bool touchesRead = panel_->readTouches(touches);
    if (!touchesRead) {
        LOG_0("morph::Instrument::process - unable to read touches from panel " << panel_->getSurfaceID());
        return false;
    }
    LOG_3("morph::Instrument::process - touches read");
    LOG_3("morph::Instrument::process - interpreting touches");
    bool touchesInterpreted = overlayFunction_->processTouches(touches);
    LOG_3("morph::Instrument::process - touches interpreted");
    if (!touchesInterpreted) {
        LOG_0("morph::Instrument::process - unable to interpret touches from panel " << panel_->getSurfaceID()
                                                                                     << "via overlay function"
                                                                                     << overlayFunction_->getName());
        return false;
    }
    return true;
}

const std::string &Instrument::getName() {
    return name_;
}

}
}