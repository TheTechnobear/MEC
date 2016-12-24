// InertSoundplaneDriver.cpp
//
// A dummy implementation of a Soundplane driver for testing purposes.

#include "InertSoundplaneDriver.h"

MLSoundplaneState InertSoundplaneDriver::getDeviceState() const
{
	return kNoDevice;
}

uint16_t InertSoundplaneDriver::getFirmwareVersion() const
{
	return 0;
}

std::string InertSoundplaneDriver::getSerialNumberString() const
{
	return "test";
}

const unsigned char *InertSoundplaneDriver::getCarriers() const
{
	return mCurrentCarriers.data();
}

void InertSoundplaneDriver::setCarriers(const Carriers& carriers)
{
}

void InertSoundplaneDriver::enableCarriers(unsigned long mask)
{
}
