#pragma once

#include "KontrolDevice.h"

#include <KontrolModel.h>


class MidiControl : public KontrolDevice {
public:
    MidiControl();
    virtual ~MidiControl();

    //KontrolDevice
    bool init() override;

private:
};
