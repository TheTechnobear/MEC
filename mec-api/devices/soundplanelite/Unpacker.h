// Driver for Soundplane Model A.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __UNPACKER__
#define __UNPACKER__

#include <array>

#include "SoundplaneModelA.h"

/**
 * The Soundplane model A USB protocol exposes two separate endpoints with
 * separate streams, each for one half of the board. A soundplane driver has to
 * take these two streams and unify them into one stream of control values.
 *
 * Unpacker objects do this, so that the SoundplaneDriver objects can focus on
 * the actual USB stuff.
 */
template<int StoredTransfersPerEndpoint, int Endpoints>
class Unpacker
{
	static_assert(Endpoints == 2, "Unpacker only supports 2 endpoints at the moment");

	/**
	 * A basic ring buffer
	 */
	template<typename T, size_t Capacity>
	class RingBuffer
	{
	public:
		RingBuffer() = default;

		RingBuffer(const RingBuffer &) = delete;
		RingBuffer &operator=(const RingBuffer &) = delete;

		/**
		 * If the buffer overflows, the oldest value is silently discarded.
		 */
		void push_back(T value)
		{
			mData[mIdx] = value;
			mSize = std::min(Capacity, mSize + 1);
			mIdx = (mIdx + 1) % Capacity;
		}

		void pop_front()
		{
			mSize--;
		}

		T &front()
		{
			// Add Capacity to ensure that the number that's %Capacity'd
			// isn't negative.
			return mData[(mIdx + Capacity - mSize) % Capacity];
		}

		bool empty() const
		{
			return mSize == 0;
		}
	private:
		size_t mSize = 0;
		size_t mIdx = 0;
		T mData[Capacity];
	};

	static bool lessThanHandleOverflow(uint16_t a, uint16_t b)
	{
		// This expression is equivalent to a < b, except that it gracefully
		// handles serial number overflows.
		return static_cast<uint16_t>(b - a) < (1 << (sizeof(a) * 8 - 1));
	}

	/**
	 * This method is called once the Unpacker has identified a matching set
	 * of packets. It unpacks the data, performs a sanity check on it and
	 * passes it to the delegate.
	 */
	void matchedPackets(SoundplaneADataPacket& p0, SoundplaneADataPacket& p1)
	{
		std::array<float, kSoundplaneOutputFrameLength> workingFrame;
		K1_unpack_float2(p0.packedData, p1.packedData, workingFrame);
		K1_clear_edges(workingFrame);
		mGotFrame(workingFrame);
	}

public:
	using GotFrameCallback = std::function<void (const SoundplaneOutputFrame& frame)>;

	Unpacker(GotFrameCallback gotFrame) :
		mGotFrame(std::move(gotFrame)) {}

	/**
	 * Feed the Unpacker with a number of packets. The Unpacker tolerates packet
	 * losses, but it does not tolerate packet reordering. If a packet arrives
	 * too early, all subsequent packets with a lower sequence number are
	 * dropped.
	 *
	 * Caution: The Unpacker saves the packets pointer. It is expected to stay
	 * valid for as long as the object is alive, or until
	 * StoredTransfersPerEndpoint subsequent calls to gotTransfer have been
	 * made (by that time, Unpacker will forget it).
	 *
	 * The expected way to deal with this constraint is for the driver to
	 * allocate StoredTransfersPerEndpoint extra transfer buffers, so that
	 * the transfer buffers that the Unpacker works with are never used by the
	 * USB stack.
	 */
	void gotTransfer(int endpoint, SoundplaneADataPacket* packets, int numPackets)
	{
		mTransfers[endpoint].push_back(Transfer(endpoint, packets, numPackets));

		Transfer* ts[2] = { getOldestTransfer(0), getOldestTransfer(1) };
		while (ts[0] && ts[1])
		{
			SoundplaneADataPacket& p0 = ts[0]->currentPacket();
			SoundplaneADataPacket& p1 = ts[1]->currentPacket();
            if(p0.seqNum == 0) {
				popPacket(&ts[0]);
            }
            else if(p1.seqNum == 0) {
				popPacket(&ts[1]);
            }
			else if (p0.seqNum == p1.seqNum)
			{
				// The sequence numbers line up
				matchedPackets(p0, p1);
				popPacket(&ts[0]);
				popPacket(&ts[1]);
			}
			else
			{
				// The oldest packet we have for one endpoint is older than
				// the oldest for the other. In this case we discard the older
				// packet.
				int olderTransferEndpoint = lessThanHandleOverflow(p0.seqNum, p1.seqNum) ? 0 : 1;
				popPacket(&ts[olderTransferEndpoint]);
			}
		}
	}

private:
	struct Transfer
	{
		Transfer() = default;

		Transfer(int endpoint, SoundplaneADataPacket* packets, int numPackets) :
			mEndpoint(endpoint),
			mPackets(packets),
			mNumPackets(numPackets) {}

		SoundplaneADataPacket& currentPacket()
		{
			return mPackets[mCurrentPacketIndex];
		}

		/**
		 * Returns true if there are no packets left after the pop.
		 */
		bool popCurrentPacket()
		{
			mCurrentPacketIndex++;
			return mCurrentPacketIndex == mNumPackets;
		}

		int endpoint() const
		{
			return mEndpoint;
		}
	private:
		int mEndpoint;
		/**
		 * Index to the first packet that has not yet been processed.
		 */
		int mCurrentPacketIndex = 0;
		SoundplaneADataPacket* mPackets = nullptr;
		int mNumPackets = 0;
	};

	/**
	 * May return nullptr
	 */
	Transfer* getOldestTransfer(int endpoint)
	{
		if (mTransfers[endpoint].empty())
		{
			return nullptr;
		}
		return &mTransfers[endpoint].front();
	}

	/**
	 * If popping the packet pops the last packet in the Transfer, it will
	 * be updated to the next one (or nullptr).
	 */
	void popPacket(Transfer **transfer)
	{
        int endpoint = (*transfer)->endpoint();
		if ((*transfer)->popCurrentPacket())
		{
			mTransfers[endpoint].pop_front();
			*transfer = getOldestTransfer(endpoint);
		}
	}

	RingBuffer<Transfer, StoredTransfersPerEndpoint> mTransfers[Endpoints];
	const GotFrameCallback mGotFrame;
};

#endif // __UNPACKER__
