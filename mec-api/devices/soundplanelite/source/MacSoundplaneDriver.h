
// Driver for Soundplane Model A.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __MAC_SOUNDPLANE_DRIVER__
#define __MAC_SOUNDPLANE_DRIVER__

#include <thread>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/usb/USB.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOReturn.h>
#include <mach/kern_return.h>

#include <sys/time.h>
#include <list>

#include "SoundplaneDriver.h"
#include "SoundplaneModelA.h"

class MacSoundplaneDriver : public SoundplaneDriver
{
public:
	MacSoundplaneDriver(SoundplaneDriverListener* listener);
	~MacSoundplaneDriver();

	void init();

	virtual MLSoundplaneState getDeviceState() const override;
	virtual uint16_t getFirmwareVersion() const override;
	virtual std::string getSerialNumberString() const override;

	virtual const unsigned char *getCarriers() const override;
	virtual void setCarriers(const Carriers& carriers) override;
	virtual void enableCarriers(unsigned long mask) override;

private:
	struct K1IsocTransaction
	{
		UInt64						busFrameNumber = 0;
		MacSoundplaneDriver			*parent = 0;
		IOUSBLowLatencyIsocFrame	*isocFrames = nullptr;
		unsigned char				*payloads = nullptr;
		uint8_t						endpointNum = 0;
		uint8_t						endpointIndex = 0;
		uint8_t						bufIndex = 0;

		uint16_t getTransactionSequenceNumber(int f);
		void setSequenceNumber(int f, uint16_t s);
	};

	IOReturn scheduleIsoch(K1IsocTransaction *t);
	static void isochComplete(void *refCon, IOReturn result, void *arg0);

	void addOffset(int& buffer, int& frame, int offset);
	uint16_t getTransferBytesReceived(int endpoint, int buffer, int frame, int offset = 0);
	AbsoluteTime getTransferTimeStamp(int endpoint, int buffer, int frame, int offset = 0);
	IOReturn getTransferStatus(int endpoint, int buffer, int frame, int offset = 0);
	uint16_t getSequenceNumber(int endpoint, int buf, int frame, int offset = 0);
	unsigned char* getPayloadPtr(int endpoint, int buf, int frame, int offset = 0);

	IOReturn setBusFrameNumber();
	void removeDevice();
	static void deviceAdded(void *refCon, io_iterator_t iterator);
	static void deviceNotifyGeneral(void *refCon, io_service_t service, natural_t messageType, void *messageArgument);

	void grabThread();
	void processThread();

	void reclockFrameToBuffer(const SoundplaneOutputFrame& frame);
	void setDeviceState(MLSoundplaneState n);
	void reportDeviceError(int errCode, int d1, int d2, float df1, float df2);
	void dumpDeviceData(float* pData, int size);

	static int getStringDescriptor(IOUSBDeviceInterface187 **dev, uint8_t descIndex, char *destBuf, uint16_t maxLen, uint16_t lang);
	void dumpTransactions(int bufferIndex, int frameIndex);
	K1IsocTransaction* getTransactionData(int endpoint, int buf) { return transactionData + kSoundplaneABuffers*endpoint + buf; }

	int mTransactionsInFlight;
	int startupCtr;

	std::thread					mGrabThread;
	std::thread					mProcessThread;

	IONotificationPortRef		notifyPort;
	io_iterator_t				matchedIter;
	io_object_t					notification;

	IOUSBDeviceInterface187		**dev;
	IOUSBInterfaceInterface192	**intf;

	UInt64						busFrameNumber[kSoundplaneANumEndpoints];
	K1IsocTransaction			transactionData[kSoundplaneANumEndpoints * kSoundplaneABuffers];
	uint8_t						payloadIndex[kSoundplaneANumEndpoints];

	std::atomic<MLSoundplaneState> mState;
	unsigned char mCurrentCarriers[kSoundplaneSensorWidth];

	// mListener may be nullptr
	SoundplaneDriverListener* const mListener;
};

#endif // __MAC_SOUNDPLANE_DRIVER__
