#ifndef MEC_PANEL_H
#define MEC_PANEL_H


#include "../../../mec_api.h"
#include "../touches/Touches.h"

namespace mec {
namespace morph {

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

}
}

#endif //MEC_PANEL_H
