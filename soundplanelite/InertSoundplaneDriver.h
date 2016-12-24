// Driver for Soundplane Model A that doesn't do anything at all.
//
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __INERT_SOUNDPLANE_DRIVER__
#define __INERT_SOUNDPLANE_DRIVER__

#include "SoundplaneDriver.h"

class InertSoundplaneDriver : public SoundplaneDriver
{
public:
	virtual MLSoundplaneState getDeviceState() const override;
	virtual uint16_t getFirmwareVersion() const override;
	virtual std::string getSerialNumberString() const override;

	virtual const unsigned char *getCarriers() const override;
	virtual void setCarriers(const Carriers& carriers) override;
	virtual void enableCarriers(unsigned long mask) override;

private:

	/**
	 * Only there to have some allocated memory that getCarriers can return.
	 */
	Carriers mCurrentCarriers;
};

#endif // __INERT_SOUNDPLANE_DRIVER__
