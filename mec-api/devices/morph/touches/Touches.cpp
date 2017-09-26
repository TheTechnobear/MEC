#include "Touches.h"

namespace mec {
namespace morph {

void Touches::addNew(const std::shared_ptr<TouchWithDeltas> touch) {
    newTouches_.push_back(touch);
}

void Touches::addContinued(const std::shared_ptr<TouchWithDeltas> touch) {
    continuedTouches_.push_back(touch);
}

void Touches::addEnded(const std::shared_ptr<TouchWithDeltas> touch) {
    endedTouches_.push_back(touch);
}

const std::vector<std::shared_ptr<TouchWithDeltas>>& Touches::getNewTouches() const {
    return newTouches_;
}

const std::vector<std::shared_ptr<TouchWithDeltas>>& Touches::getContinuedTouches() const {
    return continuedTouches_;
}

const std::vector<std::shared_ptr<TouchWithDeltas>>& Touches::getEndedTouches() const {
    return endedTouches_;
}

void Touches::clear() {
    newTouches_.clear();
    continuedTouches_.clear();
    endedTouches_.clear();
}

void Touches::add(Touches& touches) {
    const std::vector<std::shared_ptr<TouchWithDeltas>> &newTouches = touches.getNewTouches();
    for(auto touchIter = newTouches.begin(); touchIter != newTouches.end(); ++touchIter) {
        addNew(*touchIter);
    }
    const std::vector<std::shared_ptr<TouchWithDeltas>> &continuedTouches = touches.getContinuedTouches();
    for(auto touchIter = continuedTouches.begin(); touchIter != continuedTouches.end(); ++touchIter) {
        addContinued(*touchIter);
    }
    const std::vector<std::shared_ptr<TouchWithDeltas>> &endedTouches = touches.getEndedTouches();
    for(auto touchIter = endedTouches.begin(); touchIter != endedTouches.end(); ++touchIter) {
        addEnded(*touchIter);
    }
}

}
}