
// Driver for Soundplane Model A.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __LIBUSB_SOUNDPLANE_DRIVER__
#define __LIBUSB_SOUNDPLANE_DRIVER__

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <libusb-1.0/libusb.h>

#include "SoundplaneDriver.h"
#include "SoundplaneModelA.h"
#include "Unpacker.h"

class LibusbSoundplaneDriver : public SoundplaneDriver
{
public:
	LibusbSoundplaneDriver(SoundplaneDriverListener* listener);
	~LibusbSoundplaneDriver() noexcept(true);

	void init();

	virtual MLSoundplaneState getDeviceState() const override;
	virtual uint16_t getFirmwareVersion() const override;
	virtual std::string getSerialNumberString() const override;

	virtual const unsigned char *getCarriers() const override;
	virtual void setCarriers(const Carriers& carriers) override;
	virtual void enableCarriers(unsigned long mask) override;

private:
	/**
	 * A RAII helper for libusb device handles. It closes the device handle on
	 * destruction.
	 */
	class LibusbDevice
	{
	public:
		LibusbDevice() {}

		/**
		 * Handle may be nullptr.
		 */
		explicit LibusbDevice(libusb_device_handle* handle) :
			mHandle(handle) {}

		LibusbDevice(const LibusbDevice &) = delete;
		LibusbDevice& operator=(const LibusbDevice &) = delete;

		LibusbDevice(LibusbDevice &&other)
		{
			*this = std::move(other);
		}

		LibusbDevice& operator=(LibusbDevice &&other)
		{
			std::swap(mHandle, other.mHandle);
			return *this;
		}

		~LibusbDevice()
		{
			if (mHandle)
			{
				libusb_close(mHandle);
			}
		}

		libusb_device_handle* get() const
		{
			return mHandle;
		}

	private:
		libusb_device_handle* mHandle = nullptr;
	};

	/**
	 * A RAII helper for claiming libusb device interfaces.
	 */
	class LibusbClaimedDevice
	{
	public:
		LibusbClaimedDevice() {}

		/**
		 * This constructor assumes ownership of an underlying LibusbDevice
		 * and attempts to claim the specified interface number. If that fails,
		 * the LibusbDevice is released and the created LibusbClaimedDevice
		 * is an empty one.
		 *
		 * handle may be an empty handle.
		 */
		LibusbClaimedDevice(LibusbDevice &&handle, int interfaceNumber) :
			mHandle(std::move(handle)),
			mInterfaceNumber(interfaceNumber) {
			// Attempt to claim the specified interface
			if (!mHandle.get() || libusb_claim_interface(mHandle.get(), interfaceNumber) < 0) {
				// Claim failed. Reset underlying handle
				LibusbDevice empty;
				std::swap(empty, mHandle);
			}
		}

		~LibusbClaimedDevice() {
			if (mHandle.get() != nullptr) {
				libusb_release_interface(mHandle.get(), mInterfaceNumber);
			}
		}

		LibusbClaimedDevice(const LibusbDevice &) = delete;
		LibusbClaimedDevice& operator=(const LibusbDevice &) = delete;

		LibusbClaimedDevice(LibusbClaimedDevice &&other)
		{
			*this = std::move(other);
		}

		LibusbClaimedDevice& operator=(LibusbClaimedDevice &&other)
		{
			std::swap(mHandle, other.mHandle);
			std::swap(mInterfaceNumber, other.mInterfaceNumber);
			return *this;
		}

		libusb_device_handle* get() const
		{
			return mHandle.get();
		}

		explicit operator bool() const
		{
			return mHandle.get() != nullptr;
		}

	private:
		LibusbDevice mHandle;
		int mInterfaceNumber = 0;
	};

	/**
	 * LibusbSoundplaneDriver holds an integer multiple >= 2 of the buffers that
	 * are in flight. This is because buffers that have been received might not
	 * be immediately processable, since the separate Soundplane USB endpoints
	 * can be slightly out of sync. The buffers that are not in flight are kept
	 * so that when endpoint 1 is lagging behind endpoint 2, the full messages
	 * can be reconstructed when the packets for endpoint 1 arrive.
	 */
	static constexpr int kInFlightMultiplier = 2;
	static constexpr int kBuffersPerEndpoint = kInFlightMultiplier * kSoundplaneABuffersInFlight;

	using LibusbUnpacker = Unpacker<kBuffersPerEndpoint - kSoundplaneABuffersInFlight, kSoundplaneANumEndpoints>;

	/**
	 * An object that represents one USB transaction: It has a buffer and
	 * a libusb_transfer*.
	 */
	class Transfer
	{
	public:
		Transfer() :
			transfer(libusb_alloc_transfer(kSoundplaneANumIsochFrames)) {}

		Transfer(const Transfer &) = delete;
		Transfer& operator=(const Transfer &) = delete;

		~Transfer()
		{
			libusb_free_transfer(transfer);
		}

		static constexpr int numPackets()
		{
			return kSoundplaneANumIsochFrames;
		}

		/**
		 * Identifier for the endpoint, starting at 0 and going up
		 * monotonically.
		 */
		int endpointId = 0;
		/**
		 * Endpoint "address", as used by libusb.
		 */
		int endpointAddress = 0;
		libusb_device_handle* device = nullptr;
		LibusbSoundplaneDriver* parent = nullptr;
		struct libusb_transfer* const transfer;
		LibusbUnpacker* unpacker = nullptr;
		SoundplaneADataPacket packets[kSoundplaneANumIsochFrames];
		/**
		 * Only an integer fraction of the allocated buffers are ever being
		 * processed by libusb. The other ones are kept in order to be able
		 * to reconstruct messages even though the different Soundplane
		 * endpoints can be out of sync.
		 *
		 * When a transfer is done, LibusbSoundplaneDriver needs to queue one
		 * more transfer. Since the transfer that was just done has to be kept
		 * for a while before being re-submitted, some other transfer has to be
		 * submitted instead. nextTransfer points to that packet.
		 *
		 * In practice, the nextTransfer pointers are set up on initialization
		 * to point to each other in circular lists that have
		 * kInFlightMultiplier elements per cycle.
		 *
		 * @see kInFlightMultiplier
		 */
		Transfer* nextTransfer;
	};

	using Transfers = std::array<std::array<Transfer, kBuffersPerEndpoint>, kSoundplaneANumEndpoints>;

	static void processThreadControlTransferCallback(struct libusb_transfer *xfr);
	libusb_error processThreadSendControl(
		libusb_device_handle *device,
		uint8_t request,
		uint16_t value,
		uint16_t index,
		const unsigned char *data,
		size_t dataSize);

	/**
	 * Returns false if the process thread should quit.
	 *
	 * May spuriously wait for a shorter than the specified time.
	 */
	bool processThreadWait(int ms) const;
	/**
	 * Returns false if the process thread should quit.
	 */
	bool processThreadOpenDevice(LibusbClaimedDevice &outDevice) const;
	/**
	 * Sets mFirmwareVersion and mSerialNumber as a side effect, but only
	 * if the whole operation succeeds.
	 *
	 * Returns false if getting the device info failed.
	 */
	bool processThreadGetDeviceInfo(libusb_device_handle *device);
	/**
	 * Get the endpoint addresses and fill them in into the Transfer objects
	 * for later use. Also set the parent field of the Transfer objects.
	 *
	 * Returns false if getting the endpoint addresses failed.
	 */
	bool processThreadFillTransferInformation(
		Transfers &transfers,
		LibusbUnpacker *unpacker,
		libusb_device_handle *device);
	/**
	 * Sets mState to a new value and notifies the listener.
	 *
	 * Returns false if the process thread should quit.
	 */
	bool processThreadSetDeviceState(MLSoundplaneState newState);
	/**
	 * Returns false if selecting the isochronous failed.
	 */
	bool processThreadSelectIsochronousInterface(libusb_device_handle *device) const;
	bool processThreadShouldStopTransfers() const;
	/**
	 * Returns false if scheduling the transfer failed.
	 */
	bool processThreadScheduleTransfer(Transfer &transfer);
	/**
	 * Returns false if scheduling of any of the initial transfers failed.
	 */
	bool processThreadScheduleInitialTransfers(Transfers &transfers);
	static void processThreadTransferCallbackStatic(struct libusb_transfer *xfr);
	void processThreadTransferCallback(Transfer& transfer);
	libusb_error processThreadSetCarriers(
		libusb_device_handle *device, const unsigned char *carriers, size_t carriersSize);
	/**
	 * Returns true if a control request was sent.
	 */
	bool processThreadHandleRequests(libusb_device_handle *device);
	void processThread();

	/**
	 * mState is set only by the processThread. Because the processThread never
	 * decides to quit, the outward facing state of the driver is
	 * kDeviceIsTerminating if mQuitting is true.
	 */
	std::atomic<MLSoundplaneState> mState;
	/**
	 * mQuitting is set to true by the destructor, and is read by the processing
	 * thread and getDeviceState in order to know if the driver is quitting.
	 */
	std::atomic<bool> mQuitting;

	/**
	 * Written to by the processing thread before mState is set from kNoDevice,
	 * read by any thread.
	 */
	std::atomic<uint16_t> mFirmwareVersion;
	/**
	 * Written to by the processing thread before mState is set from kNoDevice,
	 * read by any thread.
	 */
	std::atomic<std::array<unsigned char, 64>> mSerialNumber;

	mutable std::mutex mMutex;  // Used with mCondition
	/**
	 * Used to wake up the process thread when it's waiting for a device
	 */
	mutable std::condition_variable mCondition;

	/**
	 * Written on object initialization and then never modified. Can be read
	 * from any thread.
	 */
	libusb_context				*mLibusbContext = nullptr;
	/**
	 * Written on object initialization and then never modified. Can be read
	 * from any thread.
	 */
	SoundplaneDriverListener	* const mListener;

	std::thread					mProcessThread;

	/**
	 * The usb transfer callback sets this to true if reading failed and the
	 * device connection should be treated as lost.
	 *
	 * Accessed only from the processing thread.
	 */
	bool						mUsbFailed;

	/**
	 * The number of outstanding transfers. This is used during shutdown to
	 * ensure that libusb isn't torn down before transfers have finished.
	 * Failure to do so results in crashes (do_close in core.c of libusb NULLs
	 * out the dev_handle of all transfers, and darwin_async_io_callback
	 * attempts to read it). Accessed only by the processing thread.
	 *
	 * I believe this should not be needed. See
	 * https://github.com/libusb/libusb/issues/84
	 */
	size_t						mOutstandingTransfers;

	/**
	 * Set to a value (allocated with new) by setCarriers. Read (and deleted)
	 * by the processing thread.
	 */
	std::atomic<const Carriers*> mSetCarriersRequest;

	/**
	 * Neither read nor written to by the processing thread. This is only a
	 * copy of the values for use by the clients of LibusbSoundplaneDriver.
	 */
	Carriers					mCurrentCarriers;

	/**
	 * Set to a value (allocated with new) by enableCarriers. Read (and deleted)
	 * by the processing thread.
	 */
	std::atomic<const unsigned long*> mEnableCarriersRequest;
};

#endif // __LIBUSB_SOUNDPLANE_DRIVER__
