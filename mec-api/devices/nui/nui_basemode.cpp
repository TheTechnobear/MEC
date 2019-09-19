#include "nui_basemode.h"

void NuiBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

