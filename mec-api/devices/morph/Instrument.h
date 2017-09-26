//
// Created by Ferdinand on 26.09.17.
//

#ifndef MEC_INSTRUMENT_H
#define MEC_INSTRUMENT_H

#include "panels/Panel.h"
#include "overlays/OverlayFunction.h"

namespace mec {
namespace morph {

class Instrument {
public:
    Instrument(const std::string name, std::shared_ptr <Panel> panel, std::unique_ptr <OverlayFunction> overlayFunction);

    bool process();

    const std::string &getName();

private:
    std::string name_;
    std::shared_ptr <Panel> panel_;
    std::unique_ptr <OverlayFunction> overlayFunction_;
};

}
}


#endif //MEC_INSTRUMENT_H
