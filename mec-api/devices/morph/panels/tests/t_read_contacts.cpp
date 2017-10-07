#include <unistd.h>
#include <iostream>
#include "../../../mec_morph.h"
#include "../SinglePanel.h"
#include "../../../../mec_log.h"
#include "../../morph_constants.h"

int main(int argc, char **argv) {
    int sleepTimeMS = 0;
    if(argc > 1) {
        sleepTimeMS = atoi(argv[1]) * 1000;
    }

    SenselDeviceList device_list;

    SenselStatus deviceStatus = senselGetDeviceList(&device_list);
    if (deviceStatus != SENSEL_OK || device_list.num_devices == 0) {
        LOG_0("morph::t_read_contacts - unable to detect Sensel Morph panel(s)");
    }

    if (device_list.num_devices < 1) {
        LOG_0("morph::t_read_contacts - at least one panel has to be connected");
        return -1;
    }

    SenselDeviceID &deviceID = device_list.devices[0];
    mec::SurfaceID surfaceID = "myPanel";

    mec::morph::SinglePanel singlePanel(deviceID, surfaceID);

    bool isInitialized = singlePanel.init();
    if (!isInitialized) {
        LOG_0("morph::t_read_contacts - unable to initialize");
        return -1;
    }

    mec::morph::Touches touches;
    while(true) {
        touches.clear();
        singlePanel.readTouches(touches);
        // exit when touching the lower 10th of the panel
        const std::vector<std::shared_ptr<mec::morph::TouchWithDeltas>> &newTouches = touches.getNewTouches();
        if(newTouches.size() > 0 && newTouches[0]->y_ > (singlePanel.getDimensions().height * 9.0) / 10.0) {
            return 0;
        }
        usleep(sleepTimeMS);
    }
}