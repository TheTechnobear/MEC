// SoundplaneDriver.cpp
//
// Returns raw data frames from the Soundplane.  The frames are reclocked if needed (TODO)
// to reconstruct a steady sample rate.
//
// Two threads are used to do this work.  A grab thread maintains a stream of
// low-latency isochronous transfers. A process thread looks through the buffers
// specified by these transfers every ms or so.  When new frames of data arrive,
// the process thread reclocks them and pushes them to a ring buffer where they
// can be read by clients.

#include "MacSoundplaneDriver.h"

#include <vector>

#include "ThreadUtility.h"

#if DEBUG
	#define VERBOSE
#endif
//#define SHOW_BUS_FRAME_NUMBER
//#define SHOW_ALL_SEQUENCE_NUMBERS

namespace {

// --------------------------------------------------------------------------------
#pragma mark error handling

// REVIEW: IOKit has documentation on handling errors
const char *io_err_string(IOReturn err)
{
	static char other[32];

	switch (err)
	{
		case kIOReturnSuccess:
			break;
		case KERN_INVALID_ADDRESS:
			return "Specified address is not currently valid";
		case KERN_PROTECTION_FAILURE:
			return "Specified memory is valid, but does not permit the required forms of access";
		case kIOReturnNoDevice:
			return "no such device";
		case kIOReturnAborted:
			return "operation aborted";
		case kIOReturnUnderrun:
			return "data underrun";
		case kIOReturnNoBandwidth:
			return "No Bandwidth: bus bandwidth would be exceeded";
		case kIOReturnIsoTooOld:
			return "isochronous I/O request for distant past!";
		case kIOUSBNotSent2Err:
			return "USB: Transaction not sent";
		case kIOUSBTransactionTimeout:
			return "USB: Transaction timed out";
		case kIOUSBPipeStalled:
			return "Pipe has stalled, error needs to be cleared";
		case kIOUSBLowLatencyFrameListNotPreviouslyAllocated:
			return "Attempted to use user land low latency isoc calls w/out calling PrepareBuffer (on the frame list) first";
		default:
			sprintf(other, "result %#x", err);
			return other;
	}
	return NULL;
}

void show_io_err(const char *msg, IOReturn err)
{
	fprintf(stderr, "%s (%08x) %s\n", msg, err, io_err_string(err));
}

void show_kern_err(const char *msg, kern_return_t kr)
{
	fprintf(stderr, "%s (%08x)\n", msg, kr);
}

}

// -------------------------------------------------------------------------------
#pragma mark MacSoundplaneDriver

std::unique_ptr<SoundplaneDriver> SoundplaneDriver::create(SoundplaneDriverListener *listener)
{
	auto *driver = new MacSoundplaneDriver(listener);
	driver->init();
	return std::unique_ptr<MacSoundplaneDriver>(driver);
}

MacSoundplaneDriver::MacSoundplaneDriver(SoundplaneDriverListener* listener) :
	mTransactionsInFlight(0),
	startupCtr(0),
	dev(0),
	intf(0),
	mState(kNoDevice),
	mListener(listener)
{
	assert(listener);

	for(int i=0; i<kSoundplaneANumEndpoints; ++i)
	{
		busFrameNumber[i] = 0;
	}

	for(int i=0; i < kSoundplaneSensorWidth; ++i)
	{
		mCurrentCarriers[i] = kDefaultCarriers[i];
	}
}

MacSoundplaneDriver::~MacSoundplaneDriver()
{
	printf("SoundplaneDriver shutting down...\n");

	kern_return_t	kr;

	setDeviceState(kDeviceIsTerminating);

	if(notifyPort)
	{
		IONotificationPortDestroy(notifyPort);
	}

	if (matchedIter)
	{
        IOObjectRelease(matchedIter);
        matchedIter = 0;
	}

	// wait for any pending transactions to finish
	//
	int totalWaitTime = 0;
	int waitTimeMicrosecs = 1000;
	while (mTransactionsInFlight > 0)
	{
		usleep(waitTimeMicrosecs);
		totalWaitTime += waitTimeMicrosecs;

		// waiting too long-- bail without cleaning up
		if (totalWaitTime > 100*1000)
		{
			printf("WARNING: Soundplane driver could not finish pending transactions!\n");
			return;
		}
	}

	// wait for process thread to terminate
	//
	if (mProcessThread.joinable())
	{
		mProcessThread.join();
		printf("process thread terminated.\n");
	}

	// wait some more
	//
	usleep(100*1000);

	if (dev)
	{
		kr = (*dev)->USBDeviceClose(dev);
		kr = (*dev)->Release(dev);
		dev = NULL;
	}
	kr = IOObjectRelease(notification);

	// clean up transaction data.
	//
	// Doing this with any transactions pending WILL cause a kernel panic!
	//
	if (intf)
	{
		unsigned i, n;
		for (n = 0; n < kSoundplaneANumEndpoints; n++)
		{
			for (i = 0; i < kSoundplaneABuffers; i++)
			{
				K1IsocTransaction* t = getTransactionData(n, i);
				if (t->payloads)
				{
					(*intf)->LowLatencyDestroyBuffer(intf, t->payloads);
					t->payloads = NULL;
				}
				if (t->isocFrames)
				{
					(*intf)->LowLatencyDestroyBuffer(intf, t->isocFrames);
					t->isocFrames = NULL;
				}
			}
		}
		kr = (*intf)->Release(intf);
		intf = NULL;
	}}

void MacSoundplaneDriver::init()
{
	// create device grab thread
	mGrabThread = std::thread(&MacSoundplaneDriver::grabThread, this);
	mGrabThread.detach();  // REVIEW: mGrabThread is leaked

	// create isochronous read and process thread
	mProcessThread = std::thread(&MacSoundplaneDriver::processThread, this);

	// set thread to real time priority
	setThreadPriority(mProcessThread.native_handle(), 96, true);
}

MLSoundplaneState MacSoundplaneDriver::getDeviceState() const
{
	return mState.load(std::memory_order_acquire);
}

uint16_t MacSoundplaneDriver::getFirmwareVersion() const
{
	if(getDeviceState() < kDeviceConnected) return 0;
	uint16_t r = 0;
	IOReturn err;
	if (dev)
	{
		uint16_t version = 0;
		err = (*dev)->GetDeviceReleaseNumber(dev, &version);
		if (err == kIOReturnSuccess)
		{
			r = version;
		}
	}
	return r;
}

std::string MacSoundplaneDriver::getSerialNumberString() const
{
	if (getDeviceState() < kDeviceConnected) return 0;
	char buffer[64];
	uint8_t idx;
	int r = 0;
	IOReturn err;
	if (dev)
	{
		err = (*dev)->USBGetSerialNumberStringIndex(dev, &idx);
		if (err == kIOReturnSuccess)
		{
			r = getStringDescriptor(dev, idx, buffer, sizeof(buffer), 0);
		}

		// exctract returned string from wchars.
		for(int i=0; i < r - 2; ++i)
		{
			buffer[i] = buffer[i*2 + 2];
		}

		return std::string(buffer, r / 2 - 1);
	}
	return "";
}

// -------------------------------------------------------------------------------
#pragma mark carriers

const unsigned char *MacSoundplaneDriver::getCarriers() const {
	return mCurrentCarriers;
}

void MacSoundplaneDriver::setCarriers(const Carriers& cData)
{
	if (!dev) return;
	if (getDeviceState() < kDeviceConnected) return;
	IOUSBDevRequest request;
	std::copy(cData.begin(), cData.end(), mCurrentCarriers);

	// wait for data to settle after setting carriers
	// TODO understand this startup behavior better
	startupCtr = 0;

	request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
    request.bRequest = kRequestCarriers;
	request.wValue = 0;
	request.wIndex = kRequestCarriersIndex;
	request.wLength = 32;
	request.pData = mCurrentCarriers;
	(*dev)->DeviceRequest(dev, &request);
}

void MacSoundplaneDriver::enableCarriers(unsigned long mask)
{
	if (!dev) return;
    IOUSBDevRequest	request;

	startupCtr = 0;

    request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
    request.bRequest = kRequestMask;
	request.wValue = mask >> 16;
	request.wIndex = mask;
	request.wLength = 0;
	request.pData = NULL;

	(*dev)->DeviceRequest(dev, &request);
}


// --------------------------------------------------------------------------------
#pragma mark K1IsocTransaction

uint16_t MacSoundplaneDriver::K1IsocTransaction::getTransactionSequenceNumber(int f)
{
	if (!payloads) return 0;
	SoundplaneADataPacket* p = (SoundplaneADataPacket*)payloads;
	return p[f].seqNum;
}

void MacSoundplaneDriver::K1IsocTransaction::setSequenceNumber(int f, uint16_t s)
{
	SoundplaneADataPacket* p = (SoundplaneADataPacket*)payloads;
	p[f].seqNum = s;
}


/*

notes on isoch scheduling:

if you take a look at AppleUSBOHCI.cpp, you'll find that kIOUSBNotSent2Err
corresponds to OHCI status 15. You can look that up in the OHCI spec. A not
sent error occurs because your request is never put onto the bus. This
usually happens if the request is for a frame number in the past, or too far
in the future. I think that all your requests are for a time in the past, so
they never get put onto the bus by the controller.

We add about 50 to 100 frames to the value returned by GetBusFrameNumber, so
the first read happens about 100ms in the future. There is a considerable
and unpredictable latency between sending an isoch read request to the USB
controller and that request appearing on the bus. Subsequent reads have
their frame start time incremented by a fixed amount, governed by the size
of the reads. You should queue up several reads at once - don't wait until
your first read completes before you calculate the parameters for the second
read - when the first read completes, you should be calculating the
parameters for perhaps the eighth read.

The first few reads you requested may come back empty, because their frame
number has already passed. This is normal.

*/


// --------------------------------------------------------------------------------
#pragma mark isochronous data

// Schedule an isochronous transfer. LowLatencyReadIsochPipeAsync() is used for all transfers
// to get the lowest possible latency. In order to complete, the transfer should be scheduled for a
// time in the future, but as soon as possible for low latency.
//
// The OS X low latency async code requires all transfer data to be in special blocks created by
// LowLatencyCreateBuffer() to manage communication with the kernel.
// These are made in deviceAdded() when a Soundplane is connected.
//
IOReturn MacSoundplaneDriver::scheduleIsoch(K1IsocTransaction *t)
{
	if (!dev) return kIOReturnNoDevice;
	MLSoundplaneState state = getDeviceState();
	if (state == kNoDevice) return kIOReturnNotReady;
	if (state == kDeviceIsTerminating) return kIOReturnNotReady;

	IOReturn err;
	assert(t);

	t->parent = this;
	t->busFrameNumber = busFrameNumber[t->endpointIndex];

	for (int k = 0; k < kSoundplaneANumIsochFrames; k++)
	{
		t->isocFrames[k].frStatus = 0;
		t->isocFrames[k].frReqCount = sizeof(SoundplaneADataPacket);
		t->isocFrames[k].frActCount = 0;
		t->isocFrames[k].frTimeStamp.hi = 0;
		t->isocFrames[k].frTimeStamp.lo = 0;
		t->setSequenceNumber(k, 0);
	}

	size_t payloadSize = sizeof(SoundplaneADataPacket) * kSoundplaneANumIsochFrames;
	bzero(t->payloads, payloadSize);

#ifdef SHOW_BUS_FRAME_NUMBER
	fprintf(stderr, "read(%d, %p, %llu, %p, %p)\n", t->endpointNum, t->payloads, t->busFrameNumber, t->isocFrames, t);
#endif
	err = (*intf)->LowLatencyReadIsochPipeAsync(intf, t->endpointNum, t->payloads,
		t->busFrameNumber, kSoundplaneANumIsochFrames, kSoundplaneAUpdateFrequency, t->isocFrames, isochComplete, t);

	busFrameNumber[t->endpointIndex] += kSoundplaneANumIsochFrames;
	mTransactionsInFlight++;

	return err;
}

// isochComplete() is the callback executed whenever an isochronous transfer completes.
// Since this is called at main interrupt time, it must return as quickly as possible.
// It is only responsible for scheduling the next transfer into the next transaction buffer.
//
void MacSoundplaneDriver::isochComplete(void *refCon, IOReturn result, void *arg0)
{
	IOReturn err;

	K1IsocTransaction *t = (K1IsocTransaction *)refCon;
	MacSoundplaneDriver *k1 = t->parent;
	assert(k1);

	k1->mTransactionsInFlight--;

	if (!k1->dev) return;
	MLSoundplaneState state = k1->getDeviceState();
	if (state == kNoDevice) return;
	if (state == kDeviceIsTerminating) return;
	if (state == kDeviceSuspend) return;

	switch (result)
	{
		case kIOReturnSuccess:
		case kIOReturnUnderrun:
			break;

		case kIOReturnIsoTooOld:
		default:
			printf("isochComplete error: %s\n", io_err_string(result));
			// try to recover.
			k1->setDeviceState(kDeviceConnected);
			break;
	}

	K1IsocTransaction *pNextTransactionBuffer = k1->getTransactionData(t->endpointIndex, (t->bufIndex + kSoundplaneABuffersInFlight) & kSoundplaneABuffersMask);

#ifdef SHOW_ALL_SEQUENCE_NUMBERS
	int index = pNextTransactionBuffer->endpointIndex;
	int start = getTransactionSequenceNumber(pNextTransaction, 0);
	int end = getTransactionSequenceNumber(pNextTransactionBuffer, kSoundplaneANumIsochFrames - 1);
	printf("endpoint %d: %d - %d\n", index, start, end);
	if (start > end)
	{
		for(int f=0; f<kSoundplaneANumIsochFrames; ++f)
		{
			if (f % 5 == 0) printf("\n");
			printf("%d ", getTransactionSequenceNumber(pNextTransactionBuffer, f));
		}
		printf("\n\n");
	}
#endif

	err = k1->scheduleIsoch(pNextTransactionBuffer);
}


// --------------------------------------------------------------------------------
#pragma mark transfer utilities

// add a positive or negative offset to the current (buffer, frame) position.
//
void MacSoundplaneDriver::addOffset(int& buffer, int& frame, int offset)
{
	// add offset to (buffer, frame) position
	if(!offset) return;
	int totalFrames = buffer*kSoundplaneANumIsochFrames + frame + offset;
	int remainder = totalFrames % kSoundplaneANumIsochFrames;
	if (remainder < 0)
	{
		remainder += kSoundplaneANumIsochFrames;
		totalFrames -= kSoundplaneANumIsochFrames;
	}
	frame = remainder;
	buffer = (totalFrames / kSoundplaneANumIsochFrames) % kSoundplaneABuffers;
	if (buffer < 0)
	{
		buffer += kSoundplaneABuffers;
	}
}

uint16_t MacSoundplaneDriver::getTransferBytesReceived(int endpoint, int buffer, int frame, int offset)
{
	if(getDeviceState() < kDeviceConnected) return 0;
	uint16_t b = 0;
	addOffset(buffer, frame, offset);
	K1IsocTransaction* t = getTransactionData(endpoint, buffer);

	if (t)
	{
		IOUSBLowLatencyIsocFrame* pf = &t->isocFrames[frame];
		b = pf->frActCount;
	}
	return b;
}

AbsoluteTime MacSoundplaneDriver::getTransferTimeStamp(int endpoint, int buffer, int frame, int offset)
{
	if(getDeviceState() < kDeviceConnected) return AbsoluteTime();
	AbsoluteTime b;
	addOffset(buffer, frame, offset);
	K1IsocTransaction* t = getTransactionData(endpoint, buffer);

	if (t)
	{
		IOUSBLowLatencyIsocFrame* pf = &t->isocFrames[frame];
		b = pf->frTimeStamp;
	}
	return b;
}

IOReturn MacSoundplaneDriver::getTransferStatus(int endpoint, int buffer, int frame, int offset)
{
	if(getDeviceState() < kDeviceConnected) return kIOReturnNoDevice;
	IOReturn b = kIOReturnSuccess;
	addOffset(buffer, frame, offset);
	K1IsocTransaction* t = getTransactionData(endpoint, buffer);

	if (t)
	{
		IOUSBLowLatencyIsocFrame* pf = &t->isocFrames[frame];
		b = pf->frStatus;
	}
	return b;
}

uint16_t MacSoundplaneDriver::getSequenceNumber(int endpoint, int buffer, int frame, int offset)
{
	if(getDeviceState() < kDeviceConnected) return 0;
	uint16_t s = 0;
	addOffset(buffer, frame, offset);
	K1IsocTransaction* t = getTransactionData(endpoint, buffer);

	if (t && t->payloads)
	{
		SoundplaneADataPacket* p = (SoundplaneADataPacket*)t->payloads;
		s = p[frame].seqNum;
	}
	return s;
}

unsigned char* MacSoundplaneDriver::getPayloadPtr(int endpoint, int buffer, int frame, int offset)
{
	if(getDeviceState() < kDeviceConnected) return 0;
	unsigned char* p = 0;
	addOffset(buffer, frame, offset);
	K1IsocTransaction* t = getTransactionData(endpoint, buffer);
	if (t && t->payloads)
	{
		p = t->payloads + frame*sizeof(SoundplaneADataPacket);
	}
	return p;
}

// --------------------------------------------------------------------------------
#pragma mark device utilities

namespace {

IOReturn ConfigureDevice(IOUSBDeviceInterface187 **dev)
{
	uint8_t							numConf;
	IOReturn						err;
	IOUSBConfigurationDescriptorPtr	confDesc;

	err = (*dev)->GetNumberOfConfigurations(dev, &numConf);
	if (err || !numConf)
        return err;
#ifdef VERBOSE
	printf("%d configuration(s)\n", numConf);
#endif

	// get the configuration descriptor for index 0
	err = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc);
	if (err)
	{
        show_io_err("unable to get config descriptor for index 0", err);
        return err;
	}

	/*
	REVIEW: Important: Because a composite class device is configured
	by the AppleUSBComposite driver, setting the configuration again
	from your application will result in the destruction of the IOUSBInterface
	nub objects and the creation of new ones. In general, the only reason
	to set the configuration of a composite class device that? matched by the
	AppleUSBComposite driver is to choose a configuration other than the first one.
	NOTE: This seems to be necessary
	*/

	err = (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue);
	if (err)
	{
        show_io_err("unable to set configuration to index 0", err);
        return err;
	}
#ifdef VERBOSE
	printf("%d interface(s)\n", confDesc->bNumInterfaces);
#endif
	return kIOReturnSuccess;
}

IOReturn SelectIsochronousInterface(IOUSBInterfaceInterface192 **intf, int n)
{
	IOReturn	err;

	// GetInterfaceNumber, GetAlternateSetting
	err = (*intf)->SetAlternateInterface(intf, n);
	if (err)
	{
		show_io_err("unable to set alternate interface", err);
		return err;
	}

	if (!n)
	{
		err = (*intf)->GetPipeStatus(intf, 0);
		if (err)
		{
			show_io_err("pipe #0 status failed", err);
			return err;
		}
	}
	else
	{
		err = (*intf)->GetPipeStatus(intf, 1);
		if (err)
		{
			show_io_err("pipe #1 status failed", err);
			return err;
		}
		err = (*intf)->GetPipeStatus(intf, 2);
		if (err)
		{
			show_io_err("pipe #2 status failed", err);
			return err;
		}
	}
	return kIOReturnSuccess;
}

}

IOReturn MacSoundplaneDriver::setBusFrameNumber()
{
	IOReturn err;
	AbsoluteTime atTime;

	err = (*dev)->GetBusFrameNumber(dev, &busFrameNumber[0], &atTime);
	if (kIOReturnSuccess != err)
		return err;
#ifdef VERBOSE
	printf("Bus Frame Number: %llu @ %X%08X\n", busFrameNumber[0], (int)atTime.hi, (int)atTime.lo);
#endif
	busFrameNumber[0] += 50;	// schedule 50 ms into the future
	busFrameNumber[1] = busFrameNumber[0];
	return kIOReturnSuccess;
}


void MacSoundplaneDriver::removeDevice()
{
	IOReturn err;
	kern_return_t	kr;
	setDeviceState(kNoDevice);

	printf("Soundplane A removed.\n");

	if (intf)
	{
		unsigned i, n;

		for (n = 0; n < kSoundplaneANumEndpoints; n++)
		{
			for (i = 0; i < kSoundplaneABuffers; i++)
			{
				K1IsocTransaction* t = getTransactionData(n, i);
				if (t->payloads)
				{
					err = (*intf)->LowLatencyDestroyBuffer(intf, t->payloads);
					t->payloads = NULL;
				}
				if (t->isocFrames)
				{
					err = (*intf)->LowLatencyDestroyBuffer(intf, t->isocFrames);
					t->isocFrames = NULL;
				}
			}
		}
		kr = (*intf)->Release(intf);
		intf = NULL;
	}

	if (dev)
	{
		printf("closing device.\n");
		kr = (*dev)->USBDeviceClose(dev);
		kr = (*dev)->Release(dev);
		dev = NULL;
	}
	kr = IOObjectRelease(notification);
}

// deviceAdded() is called by the callback set up in the grab thread when a new Soundplane device is found.
//
void MacSoundplaneDriver::deviceAdded(void *refCon, io_iterator_t iterator)
{
	MacSoundplaneDriver			*k1 = static_cast<MacSoundplaneDriver *>(refCon);
	kern_return_t				kr;
	IOReturn					err;
	io_service_t				usbDeviceRef;
	io_iterator_t				interfaceIterator;
	io_service_t				usbInterfaceRef;
	IOCFPlugInInterface			**plugInInterface = NULL;
	IOUSBDeviceInterface187		**dev = NULL;	// 182, 187, 197
	IOUSBInterfaceInterface192	**intf;
	IOUSBFindInterfaceRequest	req;
	ULONG						res;
	SInt32						score;
	UInt32						powerAvailable;
#ifdef VERBOSE
	uint16_t					vendor;
	uint16_t					product;
	uint16_t					release;
#endif
	int							i, j;
	uint8_t						n;

	while ((usbDeviceRef = IOIteratorNext(iterator)))
	{
        kr = IOCreatePlugInInterfaceForService(usbDeviceRef, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        if ((kIOReturnSuccess != kr) || !plugInInterface)
        {
            show_kern_err("unable to create a device plugin", kr);
            continue;
        }
        // have device plugin, need device interface
        err = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (void**)(LPVOID)&dev);
        IODestroyPlugInInterface(plugInInterface);
		plugInInterface = NULL;
        if (err || !dev)
        {
            show_io_err("could not create device interface", err);
            continue;
        }
		assert(!kr);
		err = (*dev)->GetDeviceBusPowerAvailable(dev, &powerAvailable);
		if (err)
		{
			show_io_err("could not get bus power available", err);
			res = (*dev)->Release(dev);
			if (kIOReturnSuccess != res)
				show_io_err("unable to release device", res);
			dev = NULL;
			continue;
		}
		printf("Available Bus Power: %d mA\n", (int)(2*powerAvailable));
		// REVIEW: GetDeviceSpeed()
#ifdef VERBOSE

        // REVIEW: technically should check these err values
        err = (*dev)->GetDeviceVendor(dev, &vendor);
        err = (*dev)->GetDeviceProduct(dev, &product);
        err = (*dev)->GetDeviceReleaseNumber(dev, &release);
		printf("Vendor:%04X Product:%04X Release Number:", vendor, product);
		// printf("%hhx.%hhx.%hhx\n", release >> 8, 0x0f & release >> 4, 0x0f & release)
		// NOTE: GetLocationID might be helpful
#endif

        // need to open the device in order to change its state
        do {
            err = (*dev)->USBDeviceOpenSeize(dev);
            if (kIOReturnExclusiveAccess == err)
            {
                printf("Exclusive access err, sleeping on it\n");
                sleep(10);
            }
        } while (kIOReturnExclusiveAccess == err);

        if (kIOReturnSuccess != err)
        {
            show_io_err("unable to open device:", err);
			goto release;
        }

        err = ConfigureDevice(dev);

		// get the list of interfaces for this device
		req.bInterfaceClass = kIOUSBFindInterfaceDontCare;
		req.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
		req.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
		req.bAlternateSetting = kIOUSBFindInterfaceDontCare;
		err = (*dev)->CreateInterfaceIterator(dev, &req, &interfaceIterator);
		if (kIOReturnSuccess != err)
		{
			show_io_err("could not create interface iterator", err);
			continue;
		}
		usbInterfaceRef = IOIteratorNext(interfaceIterator);
		if (!usbInterfaceRef)
		{
			fprintf(stderr, "unable to find an interface\n");
		}
		else while (usbInterfaceRef)
		{
			kr = IOCreatePlugInInterfaceForService(usbInterfaceRef, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
			if ((kIOReturnSuccess != kr) || !plugInInterface)
			{
				show_kern_err("unable to create plugin interface for USB interface", kr);
				goto close;
			}
			kr = IOObjectRelease(usbInterfaceRef);
			usbInterfaceRef = 0;
			// have interface plugin, need interface interface
			err = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID192), (void **)(LPVOID)&intf);
			kr = IODestroyPlugInInterface(plugInInterface);
			assert(!kr);
			plugInInterface = NULL;
			if (err || !intf)
				show_io_err("could not create interface interface", err);
			else if (intf)
			{
				// Don't release the interface here. That's one too many releases and causes set alt interface to fail
				err = (*intf)->USBInterfaceOpenSeize(intf);
				if (kIOReturnSuccess != err)
				{
					show_io_err("unable to seize interface for exclusive access", err);
					goto close;
				}

				err = SelectIsochronousInterface(intf, kSoundplaneAlternateSetting);

				// add notification for device removal and other info
				if (kIOReturnSuccess == err)
				{
					mach_port_t asyncPort;
					CFRunLoopSourceRef source;

					k1->dev = dev;
					k1->intf = intf;
					k1->payloadIndex[0] = 0;
					k1->payloadIndex[1] = 0;
					kr = IOServiceAddInterestNotification(k1->notifyPort,		// notifyPort
														  usbDeviceRef,			// service
														  kIOGeneralInterest,	// interestType
														  deviceNotifyGeneral,	// callback
														  k1,					// refCon
														  &(k1->notification)	// notification
														  );
					if (kIOReturnSuccess != kr)
					{
						show_kern_err("could not add interest notification", kr);
						goto close;
					}


					err = (*intf)->CreateInterfaceAsyncPort(intf, &asyncPort);
					if (kIOReturnSuccess != err)
					{
						show_io_err("could not create asynchronous port", err);
						goto close;
					}

					// from SampleUSBMIDIDriver: confirm MIDIGetDriverIORunLoop() contains InterfaceAsyncEventSource
					source = (*intf)->GetInterfaceAsyncEventSource(intf);
					if (!source)
					{
#ifdef VERBOSE
						fprintf(stderr, "created missing async event source\n");
#endif
						err = (*intf)->CreateInterfaceAsyncEventSource(intf, &source);
						if (kIOReturnSuccess != err)
						{
							show_io_err("failure to create async event source", err);
							goto close;
						}
					}
					if (!CFRunLoopContainsSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode))
						CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);

					err = (*intf)->GetNumEndpoints(intf, &n);
					if (kIOReturnSuccess != err)
					{
						show_io_err("could not get number of endpoints in interface", err);
						goto close;
					}
					else
					{
						printf("isochronous interface opened, %d endpoints\n", n);
					}

					// for each endpoint of the isochronous interface, get pipe properties
					for (i = 1; i <= n; i++)
					{
						uint8_t		direction;
						uint8_t		number;
						uint8_t		transferType;
						uint16_t	maxPacketSize;
						uint8_t		interval;

						err = (*intf)->GetPipeProperties(intf, i, &direction, &number, &transferType, &maxPacketSize, &interval);
						if (kIOReturnSuccess != err)
						{
							fprintf(stderr, "endpoint %d - could not get endpoint properties (%08x)\n", i, err);
							goto close;
						}
					}
					// create and setup transaction data structures for each buffer in each isoch endpoint
					for (i = 0; i < kSoundplaneANumEndpoints; i++)
					{
						for (j = 0; j < kSoundplaneABuffers; j++)
						{
							k1->getTransactionData(i, j)->endpointNum = kSoundplaneAEndpointStartIdx + i;
							k1->getTransactionData(i, j)->endpointIndex = i;
							k1->getTransactionData(i, j)->bufIndex = j;
							size_t payloadSize = sizeof(SoundplaneADataPacket) * kSoundplaneANumIsochFrames;

							err = (*intf)->LowLatencyCreateBuffer(intf, (void **)&k1->getTransactionData(i, j)->payloads, payloadSize, kUSBLowLatencyReadBuffer);
							if (kIOReturnSuccess != err)
							{
								fprintf(stderr, "could not create read buffer #%d (%08x)\n", j, err);
								goto close;
							}
							bzero(k1->getTransactionData(i, j)->payloads, payloadSize);

							size_t isocFrameSize = kSoundplaneANumIsochFrames * sizeof(IOUSBLowLatencyIsocFrame);
							err = (*intf)->LowLatencyCreateBuffer(intf, (void **)&k1->getTransactionData(i, j)->isocFrames, isocFrameSize, kUSBLowLatencyFrameListBuffer);
							if (kIOReturnSuccess != err)
							{
								fprintf(stderr, "could not create frame list buffer #%d (%08x)\n", j, err);
								goto close;
							}
							bzero(k1->getTransactionData(i, j)->isocFrames, isocFrameSize);
							k1->getTransactionData(i, j)->parent = k1;
						}
					}

					// set initial state before isoch schedule
					k1->setDeviceState(kDeviceConnected);
					err = k1->setBusFrameNumber();
					assert(!err);

					// for each endpoint, schedule first transaction and
					// a few buffers into the future
					for (j = 0; j < kSoundplaneABuffersInFlight; j++)
					{
						for (i = 0; i < kSoundplaneANumEndpoints; i++)
						{
							err = k1->scheduleIsoch(k1->getTransactionData(i, j));
							if (kIOReturnSuccess != err)
							{
								show_io_err("scheduleIsoch", err);
								goto close;
							}
						}
					}
				}
				else
				{
					//	err = (*intf)->USBInterfaceClose(intf);
					res = (*intf)->Release(intf);	// REVIEW: This kills the async read!!!!!
					intf = NULL;
				}

			}
			usbInterfaceRef = IOIteratorNext(interfaceIterator);
		}
		kr = IOObjectRelease(interfaceIterator);
		assert(!kr);
		interfaceIterator = 0;
		continue;

close:
		k1->dev = NULL;
		err = (*dev)->USBDeviceClose(dev);
        if (kIOReturnSuccess != err)
			show_io_err("unable to close device", err);
		else
			printf("closed dev:%p\n", dev);
release:
		k1->dev = NULL;
		res = (*dev)->Release(dev);
        if (kIOReturnSuccess != res)
			show_io_err("unable to release device", err);
		else
			printf("released dev:%p\n", dev);
		dev = NULL;
		kr = IOObjectRelease(usbDeviceRef);
		assert(!kr);
	}
}

// if device is unplugged, remove device and go back to waiting.
//
void MacSoundplaneDriver::deviceNotifyGeneral(void *refCon, io_service_t service, natural_t messageType, void *messageArgument)
{
	MacSoundplaneDriver *k1 = static_cast<MacSoundplaneDriver *>(refCon);

	if (kIOMessageServiceIsTerminated == messageType)
	{
		k1->removeDevice();
	}
}

// -------------------------------------------------------------------------------
#pragma mark main thread routines

// This thread is responsible for finding and adding USB devices matching the Soundplane.
// Execution is controlled by a Core Foundation (Cocoa) run loop.
//
void MacSoundplaneDriver::grabThread()
{
	bool OK = true;
	kern_return_t				kr;
	CFMutableDictionaryRef		matchingDict;
	CFRunLoopSourceRef			runLoopSource;
	CFNumberRef					numberRef;
	SInt32						usbVendor = kSoundplaneUSBVendor;
	SInt32						usbProduct = kSoundplaneUSBProduct;

	matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
	if (!matchingDict)
	{
		fprintf(stderr, "Cannot create USB matching dictionary\n");
		OK = false;
	}
	if (OK)
	{
		numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbVendor);
		assert(numberRef);
		CFDictionarySetValue(matchingDict, CFSTR(kUSBVendorID), numberRef);
		CFRelease(numberRef);
		numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbProduct);
		assert(numberRef);
		CFDictionarySetValue(matchingDict, CFSTR(kUSBProductID), numberRef);
		CFRelease(numberRef);
		numberRef = NULL;

		notifyPort = IONotificationPortCreate(kIOMasterPortDefault);
		runLoopSource = IONotificationPortGetRunLoopSource(notifyPort);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

		// Set up asynchronous callback to call deviceAdded() when a Soundplane is found.
		// TODO REVIEW: Check out IOServiceMatching() and IOServiceNameMatching()
		kr = IOServiceAddMatchingNotification(  notifyPort,
												kIOFirstMatchNotification,
												matchingDict,
												deviceAdded,
												this, // refCon
												&(matchedIter) );

		// Iterate once to get already-present devices and arm the notification
		deviceAdded(this, matchedIter);

		// Start the run loop. Now we'll receive notifications and remain looping here until the
		// run loop is stopped with CFRunLoopStop or all the sources and timers are removed from
		// the default run loop mode.
		// For more information about how Core Foundation run loops behave, see ?un Loops?in
		// Apple? Threading Programming Guide.
		CFRunLoopRun();

		// clean up
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
	}
}

// This thread is responsible for collecting all data from the Soundplane. The routines isochComplete()
// and scheduleIsoch() combine to keep data running into the transfer buffers in round-robin fashion.
// This thread looks at those buffers and attempts to stay in sync with incoming data, first
// by finding the most recent transfer to start from, then watching as transfers are filled in
// with successive sequence numbers. When a matching pair of endpoints is found, reclockFrameToBuffer()
// is called to send the data to any listeners.
//
void MacSoundplaneDriver::processThread()
{
	uint16_t curSeqNum0, curSeqNum1;
	uint16_t maxSeqNum0, maxSeqNum1;
	uint16_t currentCompleteSequence = 0;
	uint16_t newestCompleteSequence = 0;
	unsigned char* pPayload0 = 0;
	unsigned char* pPayload1 = 0;
	uint16_t curBytes0, curBytes1;
	uint16_t nextBytes0, nextBytes1;
	SoundplaneOutputFrame pWorkingFrame;
	SoundplaneOutputFrame pPrevFrame;

	// transaction data buffer index, 0 to kSoundplaneABuffers-1
	int bufferIndex = 0;

	// current frame within transaction buffer, 0 to kSoundplaneANumIsochFrames-1
	int frameIndex = 0;

	int droppedTransactions = 0;
	int framesReceived = 0;
	int badXfers = 0;
	int zeros = 0;
	int gaps = 0;
	int waiting = 0;
	int gotNext = 0;
	bool printThis = false;
	bool lost = true;
	bool advance = false;

	while(getDeviceState() != kDeviceIsTerminating)
	{
		// wait for grab and initialization
		while (getDeviceState() == kNoDevice)
		{
			usleep(100*1000);
		}
		while (getDeviceState() == kDeviceSuspend)
		{
			usleep(100*1000);
		}
		while (getDeviceState() == kDeviceConnected)
		{
			// initialize: find latest scheduled transaction
			UInt64 maxTransactionStartFrame = 0;
			int maxIdx = -1;
			for(int j=0; j < kSoundplaneABuffers; ++j)
			{
				K1IsocTransaction* t = getTransactionData(0, j);
				UInt64 frame = t->busFrameNumber;
				if(frame > maxTransactionStartFrame)
				{
					maxIdx = j;
					maxTransactionStartFrame = frame;
				}
			}

			// found at least one scheduled transaction: read backwards through all transactions
			// to get most recent complete frame.
			if (maxIdx >= 0)
			{
				int seq0, seq1;
				seq0 = seq1 = 0;
				lost = true;

				for(bufferIndex=maxIdx; (bufferIndex >= 0) && lost; bufferIndex--)
				{
					for(frameIndex=kSoundplaneANumIsochFrames - 1; (frameIndex >= 0) && lost; frameIndex--)
					{
						int l0 = getSequenceNumber(0, bufferIndex, frameIndex);
						int l1 = getSequenceNumber(1, bufferIndex, frameIndex);

						if (l0)
							seq0 = l0;
						if (l1)
							seq1 = l1;
						if (seq0 && seq1)
						{
							bufferIndex++;
							frameIndex++;
							curSeqNum0 = maxSeqNum0 = seq0;
							curSeqNum1 = maxSeqNum1 = seq1;
							currentCompleteSequence = std::min(curSeqNum0, curSeqNum1);

							// finally, we have sync.
							//
							Carriers carriers;
							std::copy(kDefaultCarriers, kDefaultCarriers + sizeof(kDefaultCarriers), carriers.begin());
							setCarriers(carriers);
							setDeviceState(kDeviceHasIsochSync);
							lost = false;
						}
					}
				}

				if (!lost)
				{
					printf("connecting at sequence numbers: %d / %d\n", curSeqNum0, curSeqNum1);
				}
			}
			usleep(100*1000);
		}

		// resume: for now just like kDeviceConnected, but without default carriers
		//
		while(getDeviceState() == kDeviceResume)
		{
			// initialize: find latest scheduled transaction
			UInt64 maxTransactionStartFrame = 0;
			int maxIdx = -1;
			for(int j=0; j < kSoundplaneABuffers; ++j)
			{
				K1IsocTransaction* t = getTransactionData(0, j);
				UInt64 frame = t->busFrameNumber;
				if(frame > maxTransactionStartFrame)
				{
					maxIdx = j;
					maxTransactionStartFrame = frame;
				}
			}

			// found at least one scheduled transaction: read backwards through all transactions
			// to get most recent complete frame.
			if (maxIdx >= 0)
			{
				int seq0, seq1;
				seq0 = seq1 = 0;
				lost = true;

				for(bufferIndex=maxIdx; (bufferIndex >= 0) && lost; bufferIndex--)
				{
					for(frameIndex=kSoundplaneANumIsochFrames - 1; (frameIndex >= 0) && lost; frameIndex--)
					{
						int l0 = getSequenceNumber(0, bufferIndex, frameIndex);
						int l1 = getSequenceNumber(1, bufferIndex, frameIndex);

						if (l0)
							seq0 = l0;
						if (l1)
							seq1 = l1;
						if (seq0 && seq1)
						{
							bufferIndex++;
							frameIndex++;
							curSeqNum0 = maxSeqNum0 = seq0;
							curSeqNum1 = maxSeqNum1 = seq1;
							currentCompleteSequence = std::min(curSeqNum0, curSeqNum1);

							setDeviceState(kDeviceHasIsochSync);
							lost = false;
						}
					}
				}

				if (!lost)
				{
					printf("resuming at sequence numbers: %d / %d\n", curSeqNum0, curSeqNum1);
				}
			}
			usleep(100*1000);
		}

		while(getDeviceState() == kDeviceHasIsochSync)
		{
			// look ahead by 1 frame to see if data was received for either surface of next frame
			//
			int lookahead = 1;
			nextBytes0 = getTransferBytesReceived(0, bufferIndex, frameIndex, lookahead);
			nextBytes1 = getTransferBytesReceived(1, bufferIndex, frameIndex, lookahead);

			// does next frame contain something ?
			// if so, we know this frame is done.
			// checking curBytes for this frame is not enough,
			// because it seems current count may be filled in
			// before payload.
			advance = (nextBytes0 || nextBytes1);

			if (advance)
			{
				waiting = 0;
				gotNext++;

				// see what data was received for current frame.  The Soundplane A always returns
				// either a full packet of 386 bytes, or 0.
				//
				curBytes0 = getTransferBytesReceived(0, bufferIndex, frameIndex);
				curBytes1 = getTransferBytesReceived(1, bufferIndex, frameIndex);

				// get sequence numbers from current frame
				if (curBytes0)
				{
					curSeqNum0 = getSequenceNumber(0, bufferIndex, frameIndex);
					maxSeqNum0 = curSeqNum0;
				}
				if (curBytes1)
				{
					curSeqNum1 = getSequenceNumber(1, bufferIndex, frameIndex);
					maxSeqNum1 = curSeqNum1;
				}
				// get newest sequence number for which we have all surfaces
				if (maxSeqNum0 && maxSeqNum1) // look for next in sequence
				{
					newestCompleteSequence = std::min(maxSeqNum0, maxSeqNum1);
				}
				else // handle zero wrap
				{
					newestCompleteSequence = std::max(maxSeqNum0, maxSeqNum1);
				}

				// if this is different from the most recent sequence sent,
				// send new sequence to buffer.
				if (newestCompleteSequence != currentCompleteSequence)
				{
					// look for gaps
					int sequenceWanted = ((int)currentCompleteSequence + 1)&0x0000FFFF;
					if (newestCompleteSequence != sequenceWanted)
					{
						reportDeviceError(kDevGapInSequence, currentCompleteSequence, newestCompleteSequence, 0., 0.);
						printThis = true;
						gaps++;

						// TODO try to recover if too many gaps near each other
					}

					currentCompleteSequence = newestCompleteSequence;

					// scan backwards in buffers from current position to get payloads matching currentCompleteSequence
					// TODO save indices
					//
					pPayload0 = pPayload1 = 0;
					int b = bufferIndex;
					int f = frameIndex;
					for(int k = 0; k > -kSoundplaneABuffers * kSoundplaneANumIsochFrames; k--)
					{
						int seq0 = getSequenceNumber(0, b, f, k);
						if (seq0 == currentCompleteSequence)
						{
							pPayload0 = getPayloadPtr(0, b, f, k);
						}
						int seq1 = getSequenceNumber(1, b, f, k);
						if (seq1 == currentCompleteSequence)
						{
							pPayload1 = getPayloadPtr(1, b, f, k);
						}
						if (pPayload0 && pPayload1)
						{
							// printf("[%d] \n", currentCompleteSequence);
							framesReceived++;
							break;
						}
					}

					if (pPayload0 && pPayload1)
					{
						K1_unpack_float2(pPayload0, pPayload1, pWorkingFrame);
						K1_clear_edges(pWorkingFrame);
						if(startupCtr > kSoundplaneStartupFrames)
						{
							float df = frameDiff(pPrevFrame, pWorkingFrame);
							if (df < kMaxFrameDiff)
							{
								// we are OK, the data gets out normally
								reclockFrameToBuffer(pWorkingFrame);
							}
							else
							{
								// possible sensor glitch.  also occurs when changing carriers.
								reportDeviceError(kDevDataDiffTooLarge, startupCtr, 0, df, 0.);
								dumpDeviceData(pPrevFrame.data(), kSoundplaneWidth * kSoundplaneHeight);
								dumpDeviceData(pWorkingFrame.data(), kSoundplaneWidth * kSoundplaneHeight);
								startupCtr = 0;
							}
						}
						else
						{
							// wait for initialization
							startupCtr++;
						}

						std::copy(pWorkingFrame.begin(), pWorkingFrame.end(), pPrevFrame.begin());
					}
				}

				// increment current frame / buffer position
				if (++frameIndex >= kSoundplaneANumIsochFrames)
				{
					frameIndex = 0;
					bufferIndex++;
					bufferIndex &= kSoundplaneABuffersMask;
				}
			}
			else
			{
				waiting++;
				// NOT WORKING
				if (waiting > 1000) // delay amount? -- try reconnect
				{
					printf("RESYNCHING\n");
					waiting = 0;
					setDeviceState(kNoDevice);
				}
			}

			if (printThis)
			{
				if (dev)
				{
		//			AbsoluteTime atTime;
		//			UInt64 bf;
		//			OSErr err = (*dev)->GetBusFrameNumber(dev, &bf, &atTime);
		//			k1->dumpTransactions(bufferIndex, frameIndex);

					printf("current seq num: %d / %d\n", curSeqNum0, curSeqNum1);
		//			printf("now: %u:%u\n", (int)atTime.hi, (int)atTime.lo);
					printf("process: gaps %d, dropped %d, gotNext %d \n", gaps, droppedTransactions, gotNext);
					printf("process: good frames %d, bad xfers %d \n", framesReceived, badXfers);
					printf("process: current sequence: [%d, %d] \n", maxSeqNum0, maxSeqNum1);
					printf("process: transactions in flight: %d\n", mTransactionsInFlight);

					droppedTransactions = 0;
					framesReceived = 0;
					badXfers = 0;
					zeros = 0;
					gaps = 0;
					gotNext = 0;
				}
				printThis = false;
			}

			// wait .5 ms
			usleep(500);
		}
	}
}

// write frame to buffer, reconstructing a constant clock from the data.
// this may involve interpolating frames.
//
void MacSoundplaneDriver::reclockFrameToBuffer(const SoundplaneOutputFrame& frame)
{
	// currently, clock is ignored and we simply ship out data as quickly as possible.
	// TODO timestamps that will allow reconstituting the data with lower jitter.
	mListener->receivedFrame(*this, frame.data(), frame.size());
}

void MacSoundplaneDriver::setDeviceState(MLSoundplaneState n)
{
	mState.store(n, std::memory_order_release);
	mListener->deviceStateChanged(*this, n);
}

void MacSoundplaneDriver::reportDeviceError(int errCode, int d1, int d2, float df1, float df2)
{
	mListener->handleDeviceError(errCode, d1, d2, df1, df2);
}

void MacSoundplaneDriver::dumpDeviceData(float* pData, int size)
{
	mListener->handleDeviceDataDump(pData, size);
}

// -------------------------------------------------------------------------------
#pragma mark transfer utilities

int MacSoundplaneDriver::getStringDescriptor(IOUSBDeviceInterface187 **dev, uint8_t descIndex, char *destBuf, uint16_t maxLen, UInt16 lang)
{
    IOUSBDevRequest req;
    uint8_t 		desc[256]; // Max possible descriptor length
    uint8_t 		desc2[256]; // Max possible descriptor length
    int stringLen;
    IOReturn err;
    if (lang == 0) // set default langID
        lang=0x0409;

	bzero(&req, sizeof(req));
    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBDevice);
    req.bRequest = kUSBRqGetDescriptor;
    req.wValue = (kUSBStringDesc << 8) | descIndex;
    req.wIndex = lang;	// English
    req.wLength = 2;
    req.pData = &desc;
    (err = (*dev)->DeviceRequest(dev, &req));
    if ( (err != kIOReturnSuccess) && (err != kIOReturnOverrun) )
        return -1;

    // If the string is 0 (it happens), then just return 0 as the length
    //
    stringLen = desc[0];
    if(stringLen == 0)
    {
        return 0;
    }

    // OK, now that we have the string length, make a request for the full length
    //
	bzero(&req, sizeof(req));
    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBDevice);
    req.bRequest = kUSBRqGetDescriptor;
    req.wValue = (kUSBStringDesc << 8) | descIndex;
    req.wIndex = lang;	// English
    req.wLength = stringLen;
    req.pData = desc2;

    (err = (*dev)->DeviceRequest(dev, &req));
	if ( err ) return -1;

	// copy to output buffer
	size_t destLen = std::min((int)req.wLenDone, (int)maxLen);
	std::copy((const char *)desc2, (const char *)desc2 + destLen, destBuf);

    return destLen;
}

void MacSoundplaneDriver::dumpTransactions(int bufferIndex, int frameIndex)
{
	for (int j=0; j<kSoundplaneABuffers; ++j)
	{
		K1IsocTransaction* t0 = getTransactionData(0, j);
		K1IsocTransaction* t1 = getTransactionData(1, j);
		UInt64 b0 = t0->busFrameNumber;
		UInt64 b1 = t1->busFrameNumber;
		printf("\n%d: frame %09llu/%09llu", j, b0, b1);
		if (bufferIndex == j)
		{
			printf(" *current*");
		}
		for(int f=0; f<kSoundplaneANumIsochFrames; ++f)
		{
			IOUSBLowLatencyIsocFrame* frame0, *frame1;
			uint16_t seq0, seq1;
			frame0 = &(t0->isocFrames[f]);
			frame1 = &(t1->isocFrames[f]);

			seq0 = t0->getTransactionSequenceNumber(f);
			seq1 = t1->getTransactionSequenceNumber(f);

			if (f % 4 == 0) printf("\n");
			printf("%05d:%d:%d/", (int)seq0, (frame0->frReqCount), (frame0->frActCount));
			printf("%05d:%d:%d", (int)seq1, (frame1->frReqCount), (frame1->frActCount));
			if ((frameIndex == f) && (bufferIndex == j))
			{
				printf("*  ");
			}
			else
			{
				printf("   ");
			}
		}
		printf("\n");
	}
}
