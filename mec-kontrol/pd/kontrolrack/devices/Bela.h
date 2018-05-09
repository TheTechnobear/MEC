#pragma once

#include "KontrolDevice.h"

#include <KontrolModel.h>


class Bela : public KontrolDevice {
public:
    Bela();
    virtual ~Bela();

    //KontrolDevice
    bool init() override;

private:
};
