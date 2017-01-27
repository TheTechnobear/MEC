
// Driver for Soundplane Model A.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __SOUNDPLANE_DRIVER__
#define __SOUNDPLANE_DRIVER__

#include <array>
#include <memory>
#include <string>

#include "SoundplaneModelA.h"

// device states
//
typedef enum
{
  kNoDevice = 0,  // No device has been found yet
  kDeviceConnected = 1,  // A device has been found, but isochronous transfer isn't yet up
  kDeviceHasIsochSync = 2,  // The main mode
  kDeviceIsTerminating = 3,  // The driver is shutting down
  kDeviceSuspend = 4,  // Seems to be unused (?)
  kDeviceResume = 5  // Seems to be unused (?)
} MLSoundplaneState;

class SoundplaneDriver;

class SoundplaneDriverListener
{
public:
	SoundplaneDriverListener() {}
	virtual ~SoundplaneDriverListener() {}

	/**
	 * This callback may be invoked from an arbitrary thread, even from an
	 * interrupt context, so be careful! However, when s is
	 * kDeviceIsTerminating, the callback is never invoked from an interrupt
	 * context, so in that particular case it is safe to block.
	 *
	 * For each state change, there will be exactly one deviceStateChanged
	 * invocation. However, these calls may arrive simultaneously or out of
	 * order, so be careful!
	 *
	 * Please note that by the time the callback is invoked with a given state
	 * s, the SoundplaneDriver might already have moved to another state, so
	 * it might be that s != driver->getDeviceState().
	 */
	virtual void deviceStateChanged(SoundplaneDriver &driver, MLSoundplaneState s) {}

	/**
	 * Invoked whenever the SoundplaneDriver receives a frame of data from the
	 * Soundplane. This is invoked on a processing thread of the driver, always
	 * from the same thread.
	 */
	virtual void receivedFrame(SoundplaneDriver &driver, const float* data, int size) {}

	/**
	 * This callback may be invoked from an arbitrary thread, but is never
	 * invoked in an interrupt context.
	 */
	virtual void handleDeviceError(int errorType, int di1, int di2, float df1, float df2) {}

	/**
	 * This callback may be invoked from an arbitrary thread, but is never
	 * invoked in an interrupt context.
	 */
	virtual void handleDeviceDataDump(const float* pData, int size) {}
};

class SoundplaneDriver
{
public:
	virtual ~SoundplaneDriver() = default;

	virtual MLSoundplaneState getDeviceState() const = 0;

	/**
	 * Returns the firmware version of the connected Soundplane device. The
	 * return value is undefined if getDeviceState() == kNoDevice
	 */
	virtual uint16_t getFirmwareVersion() const = 0;

	/**
	 * Returns the serial number of the connected Soundplane device. The return
	 * value is undefined if getDeviceState() == kNoDevice
	 */
	virtual std::string getSerialNumberString() const = 0;

	/**
	 * Returns a pointer to the array of current carriers. The array length
	 * is kSoundplaneSensorWidth.
	 */
	virtual const unsigned char *getCarriers() const = 0;

	using Carriers = std::array<unsigned char, kSoundplaneSensorWidth>;
	/**
	 * Calls to setCarriers fail if getDeviceState() == kNoDevice
	 */
	virtual void setCarriers(const Carriers& carriers) = 0;

	/**
	 * Calls to enableCarriers fail if getDeviceState() == kNoDevice
	 */
	virtual void enableCarriers(unsigned long mask) = 0;

	/**
	 * Helper function for getting the serial number as a number rather than
	 * as a string.
	 */
	virtual int getSerialNumber() const final;

	/**
	 * Helper function for printing the carrier frequencies.
	 */
	virtual void dumpCarriers() const final;

	/**
	 * Create a SoundplaneDriver object that is appropriate for the current
	 * platform.
	 *
	 * listener may be nullptr.
	 */
	static std::unique_ptr<SoundplaneDriver> create(SoundplaneDriverListener *listener);

	static float carrierToFrequency(int carrier);
};

#endif // __SOUNDPLANE_DRIVER__
