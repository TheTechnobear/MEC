#ifndef MEC_SINGLEPANEL_H
#define MEC_SINGLEPANEL_H


#include <src/sensel.h>
#include <map>
#include "Panel.h"

namespace mec {
namespace morph {

class SinglePanel : public Panel {
public:
    SinglePanel(const SenselDeviceID &deviceID, const SurfaceID &surfaceID);

    virtual bool init();

    ~SinglePanel();

    virtual bool readTouches(Touches &touches);

private:
    SENSEL_HANDLE handle_;
    SenselFrameData *frameData_;
    SenselDeviceID deviceID_;
    std::map<int, std::shared_ptr<TouchWithDeltas>> activeTouches_;

    void updateDeltas(const std::shared_ptr <TouchWithDeltas> &touch,
                      std::shared_ptr <TouchWithDeltas> foundTouch) const;
};

}
}


#endif //MEC_SINGLEPANEL_H
