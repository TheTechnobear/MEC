
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "SoundplaneOSCOutput.h"

const char* kDefaultHostnameString = "localhost";

OSCVoice::OSCVoice() :
    startX(0),
    startY(0),
    x(0),
    y(0),
    z(0),
    note(0),
    mState(kVoiceStateInactive)
{
}

OSCVoice::~OSCVoice()
{
}

// --------------------------------------------------------------------------------
#pragma mark SoundplaneOSCOutput

SoundplaneOSCOutput::SoundplaneOSCOutput() :
	mDataFreq(250.),
	mLastFrameStartTime(0),
	mCurrentBaseUDPPort(kDefaultUDPPort),
	mFrameId(0),
	mSerialNumber(0),
	lastInfrequentTaskTime(0),
    mGotNoteChangesThisFrame(false),
    mGotMatrixThisFrame(false)
{
	// create buffers for UDP packet streams
	mUDPBuffers.resize(kNumUDPPorts);
	for(int i=0; i<kNumUDPPorts; ++i)
	{
		mUDPBuffers[i].resize(kUDPOutputBufferSize);
	}
	mUDPPacketStreams.resize(kNumUDPPorts);
	
	resetAllSockets();
	
	// create a vector of voices for each possible port offset
	mOSCVoices.resize(kNumUDPPorts);
	for(int i=0; i<kNumUDPPorts; ++i)
	{
		mOSCVoices[i].resize(kSoundplaneMaxTouches);
	}
}

SoundplaneOSCOutput::~SoundplaneOSCOutput()
{
}

void SoundplaneOSCOutput::connect(const char* name, int port)
{	
	mCurrentBaseUDPPort = port;
	resetAllSockets();
	try
	{
		UdpTransmitSocket& socket = getTransmitSocketForOffset(0);
		osc::OutboundPacketStream& p = getPacketStreamForOffset(0);
		p << osc::BeginBundleImmediate;
		p << osc::BeginMessage( "/t3d/dr" );	
		p << (osc::int32)mDataFreq;
		p << osc::EndMessage;
		p << osc::EndBundle;
		socket.Send( p.Data(), p.Size() );
		debug() << "SoundplaneOSCOutput:connected to " << name << ", port " << port << "\n";
	}
	catch(std::runtime_error err)
	{
		debug() << "SoundplaneOSCOutput::connect error: " << err.what() << "\n";
		mCurrentBaseUDPPort = kDefaultUDPPort;
	}
}

void SoundplaneOSCOutput::setActive(bool v)
{
	mActive = v;
	
	// reset frame ID
	mFrameId = 0;
}

void SoundplaneOSCOutput::doInfrequentTasks()
{
	// for each possible offset: if a socket has been created at that offset, send data rate and notifications 
	for(int portOffset = 0; portOffset < kNumUDPPorts; portOffset++)
	{
		if(mSocketInitialized[portOffset])	
		{
			osc::OutboundPacketStream& p = getPacketStreamForOffset(portOffset);
			UdpTransmitSocket& socket = getTransmitSocketForOffset(portOffset);

			// send data rate to receiver
			p << osc::BeginBundleImmediate;
			p << osc::BeginMessage( "/t3d/dr" );	
			p << (osc::int32)mDataFreq;
			p << osc::EndMessage;
			p << osc::EndBundle;
			socket.Send( p.Data(), p.Size() );
		}
	}
}

void SoundplaneOSCOutput::resetAllSockets()
{
	mUDPSockets.clear();
	mUDPSockets.resize(kNumUDPPorts);
	mSocketInitialized.resize(kNumUDPPorts);
	mSocketInitialized = {false};
}

void SoundplaneOSCOutput::initializeSocket(int portOffset)
{
	mUDPPacketStreams[portOffset] = std::unique_ptr< osc::OutboundPacketStream >
		(new osc::OutboundPacketStream( mUDPBuffers[portOffset].data(), kUDPOutputBufferSize ));
	mUDPSockets[portOffset] = std::unique_ptr< UdpTransmitSocket >
		(new UdpTransmitSocket(IpEndpointName(kDefaultHostnameString, mCurrentBaseUDPPort + portOffset)));
	mSocketInitialized[portOffset] = true;
}

osc::OutboundPacketStream& SoundplaneOSCOutput::getPacketStreamForOffset(int portOffset)
{
	if(!mSocketInitialized[portOffset])
	{
		initializeSocket(portOffset);
	}
	osc::OutboundPacketStream& p (*mUDPPacketStreams[portOffset]);
	p.Clear();
	return p;
}

UdpTransmitSocket& SoundplaneOSCOutput::getTransmitSocketForOffset(int portOffset)
{
	if(!mSocketInitialized[portOffset])
	{
		initializeSocket(portOffset);
	}
	return *mUDPSockets[portOffset];
}


#include <sys/time.h>
unsigned long getMilliseconds() 
{
    timeval time;
    gettimeofday(&time, NULL);
    unsigned long msec = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    return msec;
}

void SoundplaneOSCOutput::processSoundplaneMessage(const SoundplaneDataMessage* msg)
{
    static const MLSymbol startFrameSym("start_frame");
    static const MLSymbol touchSym("touch");
    static const MLSymbol onSym("on");
    static const MLSymbol continueSym("continue");
    static const MLSymbol offSym("off");
    static const MLSymbol controllerSym("controller");
    static const MLSymbol xSym("x");
    static const MLSymbol ySym("y");
    static const MLSymbol xySym("xy");
    static const MLSymbol zSym("z");
    static const MLSymbol toggleSym("toggle");
    static const MLSymbol endFrameSym("end_frame");
    static const MLSymbol matrixSym("matrix");
    static const MLSymbol nullSym;
    
	if (!mActive) return;
    MLSymbol type = msg->mType;
    MLSymbol subtype = msg->mSubtype;
    
    int voiceIdx, offset;
	float x, y, z, dz, note, vibrato;
    
    if(type == startFrameSym)
    {
        const unsigned long dataPeriodMillisecs = 1000 / mDataFreq;
        mCurrFrameStartTime = getMilliseconds();
        if (mCurrFrameStartTime > mLastFrameStartTime + dataPeriodMillisecs)
        {
            mLastFrameStartTime = mCurrFrameStartTime;
            mTimeToSendNewFrame = true;
        }
        else
        {
            mTimeToSendNewFrame = false;
        }        
        mGotNoteChangesThisFrame = false;
        mGotMatrixThisFrame = false;
		
		for(int i=0; i < kSoundplaneMaxTouches; ++i)
		{
			mPrevPortOffsetsByTouch[i] = mPortOffsetsByTouch[i];
		}
		
		// update all voice states
		for(int offset=0; offset < kNumUDPPorts; ++offset)
		{
			for(int voiceIdx=0; voiceIdx < kSoundplaneMaxTouches; ++voiceIdx)
			{
				OSCVoice& v = mOSCVoices[offset][voiceIdx];        
				if (v.mState == kVoiceStateOff)
				{
					v.mState = kVoiceStateInactive;
				}
			}
		}
		
    }
    else if(type == touchSym)
    {
        // get incoming touch data from message
        voiceIdx = msg->mData[0];
        x = msg->mData[1];
        y = msg->mData[2];
        z = msg->mData[3];
        dz = msg->mData[4];
		note = msg->mData[5];
		vibrato = msg->mData[6];
		offset = msg->mOffset;
		
		mPortOffsetsByTouch[voiceIdx] = offset;
		
		// update new voice state for incoming touch
        OSCVoice& v = mOSCVoices[offset][voiceIdx];        
        v.x = x;
        v.y = y;
        v.z = z;
        v.note = note + vibrato;
		
        if(subtype == onSym)
        {
            v.startX = x;
            v.startY = y;
			
			// send dz (velocity) as first z value
			v.z = dz;
			
            v.mState = kVoiceStateOn;
            mGotNoteChangesThisFrame = true;
        }
        if(subtype == continueSym)
        {
            v.mState = kVoiceStateActive;
        }
        if(subtype == offSym)
        {
            if((v.mState == kVoiceStateActive) || (v.mState == kVoiceStateOn))
            {
                v.mState = kVoiceStateOff;
                v.z = 0;
                mGotNoteChangesThisFrame = true;
            }
        }
    }
    else if(type == controllerSym)
    {
        // when a controller message comes in, make a local copy of the message and store by zone ID.
        int zoneID = msg->mData[0];
        mMessagesByZone[zoneID] = *msg;
    }
    else if(type == matrixSym)
    {
        // store matrix to send with bundle
        mGotMatrixThisFrame = true;
        mMatrixMessage = *msg;
    }
    else if(type == endFrameSym)
    {
        if(mGotNoteChangesThisFrame || mTimeToSendNewFrame)
        {
			sendFrame();
		}
		
		// format and send matrix in OSC blob if we got one.
		// matrix is always sent to the default port. 
		if(mGotMatrixThisFrame)
		{
			osc::OutboundPacketStream& p = getPacketStreamForOffset(0);					 
			UdpTransmitSocket& socket = getTransmitSocketForOffset(0);
			p << osc::BeginMessage( "/t3d/matrix" );
			p << osc::Blob( &(msg->mMatrix), sizeof(msg->mMatrix) );
			p << osc::EndMessage;
			mGotMatrixThisFrame = false;
			socket.Send( p.Data(), p.Size() );
		}
    }
}

void SoundplaneOSCOutput::sendFrame()
{
	static const MLSymbol controllerSym("controller");
	static const MLSymbol xSym("x");
	static const MLSymbol ySym("y");
	static const MLSymbol xySym("xy");
    static const MLSymbol xyzSym("xyz");
	static const MLSymbol zSym("z");
	static const MLSymbol toggleSym("toggle");
	static const MLSymbol nullSym;	

	float x, y, z;
	
	// for each zone, send and clear any controller messages received since last frame
	// to the output port for that zone. controller messages are not sent in bundles.
	for(int i=0; i<kSoundplaneAMaxZones; ++i)
	{
		SoundplaneDataMessage* pMsg = &(mMessagesByZone[i]);
		int portOffset = pMsg->mOffset;
		
		if(pMsg->mType == controllerSym)
		{
			// send controller message: /t3d/[zoneName] val1 (val2) on port (3123 + offset).
			osc::OutboundPacketStream& p = getPacketStreamForOffset(portOffset);
			UdpTransmitSocket& socket = getTransmitSocketForOffset(portOffset);
			
			// int channel = pMsg->mData[1];
			// int ctrlNum1 = pMsg->mData[2];
			// int ctrlNum2 = pMsg->mData[3];
			// int ctrlNum3 = pMsg->mData[4];
			x = pMsg->mData[5];
			y = pMsg->mData[6];
			z = pMsg->mData[7];
			std::string ctrlStr("/");
			ctrlStr += (pMsg->mZoneName);
			
			p << osc::BeginMessage( ctrlStr.c_str() );
			
			// get control data by type and add to message
			if(pMsg->mSubtype == xSym)
			{
				p << x;
			}
			else if(pMsg->mSubtype == ySym)
			{
				p << y;
			}
			else if (pMsg->mSubtype == xySym)
			{
				p << x << y;
			}
			else if (pMsg->mSubtype == zSym)
			{
				p << z;
			}
            else if (pMsg->mSubtype == xyzSym)
            {
                p << x << y << z;
            }
			else if (pMsg->mSubtype == toggleSym)
			{
				int t = (x > 0.5f);
				p << t;
			}
			p << osc::EndMessage;
			
			// clear
			mMessagesByZone[i].mType = nullSym;
			
			socket.Send( p.Data(), p.Size() );
		}
	}
	
	// for each port, send an OSC bundle containing any touches.
	for(int portOffset=0; portOffset<kNumUDPPorts; ++portOffset)
	{
        if(!mSocketInitialized[portOffset]) continue;
        
		// begin OSC bundle for this frame
		// timestamp is now stored in the bundle, synchronizing all info for this frame.
		osc::OutboundPacketStream& p = getPacketStreamForOffset(portOffset);					 
		UdpTransmitSocket& socket = getTransmitSocketForOffset(portOffset);
		
		// begin bundle
		p << osc::BeginBundle(mCurrFrameStartTime);
		
		// send frame start message
		p << osc::BeginMessage( "/t3d/frm" );
		p << mFrameId++ << mSerialNumber;
		p << osc::EndMessage;
		
		for(int voiceIdx=0; voiceIdx < kSoundplaneMaxTouches; ++voiceIdx)
		{					
			OSCVoice& v = mOSCVoices[portOffset][voiceIdx];
            if(v.mState != kVoiceStateInactive)
            {
                osc::int32 touchID = voiceIdx + 1; // 1-based for OSC
                
                std::string address("/t3d/tch" + std::to_string(touchID));
                p << osc::BeginMessage( address.c_str() );
                p << v.x << v.y << v.z << v.note;
                p << osc::EndMessage;
            }
		}
		
		// end bundle
		p << osc::EndBundle;
		
		// send it
		socket.Send( p.Data(), p.Size() );
		
	}
}

