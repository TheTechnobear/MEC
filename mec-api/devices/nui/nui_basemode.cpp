#include "nui_basemode.h"

namespace mec {

void NuiBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

}// mec
