#ifndef MEC_TOUCH_WITH_DELTAS_H
#define MEC_TOUCH_WITH_DELTAS_H
#include "../../../mec_api.h"

namespace mec {
namespace morph {

class TouchWithDeltas : public Touch {
public:
    TouchWithDeltas() {}
    TouchWithDeltas(int id, SurfaceID surface, float x, float y, float z, float r, float c, float delta_x,
                                     float delta_y, float delta_z)
            : Touch(id, surface, x, y, z, r, c), delta_x_(delta_x), delta_y_(delta_y), delta_z_(delta_z),
              quantizing_offset_x_(0)
    {}

    float delta_x_;
    float delta_y_;
    float delta_z_;
    float quantizing_offset_x_;
};

}
}
#endif //MEC_TOUCH_WITH_DELTAS_H