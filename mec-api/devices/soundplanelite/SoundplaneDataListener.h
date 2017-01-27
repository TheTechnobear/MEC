
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __SOUNDPLANE_DATA_LISTENER__
#define __SOUNDPLANE_DATA_LISTENER__

#include "MLSignal.h"
#include "MLModel.h"
#include "SoundplaneDriver.h"
#include "SoundplaneModelA.h"

const int kSoundplaneMaxControllerNumber = 127;

enum VoiceState
{
    kVoiceStateInactive = 0,
    kVoiceStateOn,
    kVoiceStateActive,
    kVoiceStateOff
};

struct SoundplaneDataMessage
{
    MLSymbol mType;
    MLSymbol mSubtype;
	int mOffset;				// offset for OSC port or MIDI channel
    std::string mZoneName;
    float mData[8];
    float mMatrix[kSoundplaneWidth*kSoundplaneHeight];
};

class SoundplaneDataListener
{
public:
	SoundplaneDataListener() : mActive(false) {}
	virtual ~SoundplaneDataListener() {}
    virtual void processSoundplaneMessage(const SoundplaneDataMessage* message) = 0;
    bool isActive() { return mActive; }

protected:
	bool mActive;
};

typedef std::list<SoundplaneDataListener*> SoundplaneListenerList;

#endif // __SOUNDPLANE_DATA_LISTENER__

