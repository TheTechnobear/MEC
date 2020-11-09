#include "e1_basemode.h"

namespace mec {

void ElectraOneBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

}// mec
