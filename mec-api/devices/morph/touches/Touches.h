#ifndef MEC_TOUCHES_H
#define MEC_TOUCHES_H

#include <vector>
#include "TouchWithDeltas.h"

namespace mec {
namespace morph {


class Touches {
public:
    void addNew(const std::shared_ptr<TouchWithDeltas> touch);

    void addContinued(const std::shared_ptr<TouchWithDeltas> touch);

    void addEnded(const std::shared_ptr<TouchWithDeltas> touch);

    const std::vector<std::shared_ptr<TouchWithDeltas>>& getNewTouches() const;

    const std::vector<std::shared_ptr<TouchWithDeltas>>& getContinuedTouches() const;

    const std::vector<std::shared_ptr<TouchWithDeltas>>& getEndedTouches() const;

    void clear();

    void add(Touches& touches);
private:
    std::vector<std::shared_ptr<TouchWithDeltas>> newTouches_;
    std::vector<std::shared_ptr<TouchWithDeltas>> continuedTouches_;
    std::vector<std::shared_ptr<TouchWithDeltas>> endedTouches_;
};

}
}

#endif //MEC_TOUCHES_H
