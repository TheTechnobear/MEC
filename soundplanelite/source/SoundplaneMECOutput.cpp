
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "SoundplaneMECOutput.h"
#include "SoundplaneModelA.h"


SoundplaneMECOutput::SoundplaneMECOutput() : 
callback_(NULL)
,mCurrFrameStartTime(0)
,mLastFrameStartTime(0)
,mTimeToSendNewFrame(false)
,mDataFreq(500.0f)
{
}

SoundplaneMECOutput::~SoundplaneMECOutput()
{
}

void SoundplaneMECOutput::deviceInit()
{
    if (callback_)
    {
        callback_->device(serialNumber_.c_str(), kSoundplaneHeight, kSoundplaneWidth);
    }
}


void SoundplaneMECOutput::connect(MECCallback* cb)
{
    callback_ = cb;
}

void SoundplaneMECOutput::setActive(bool v)
{
    mActive = v;
}

void SoundplaneMECOutput::doInfrequentTasks()
{
}


//TODO review this!!!
#include <sys/time.h>
unsigned long getMilliseconds2()
{
    timeval time;
    gettimeofday(&time, NULL);
    unsigned long msec = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    return msec;
}

void SoundplaneMECOutput::processSoundplaneMessage(const SoundplaneDataMessage* msg)
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
    static const MLSymbol xyzSym("xyz");
    static const MLSymbol zSym("z");
    static const MLSymbol toggleSym("toggle");
    static const MLSymbol endFrameSym("end_frame");
    static const MLSymbol matrixSym("matrix");
    static const MLSymbol nullSym;

    if (!mActive || !callback_) return;

    MLSymbol type = msg->mType;
    MLSymbol subtype = msg->mSubtype;

    int voiceIdx, offset;
    float x, y, z, dz, note, vibrato;

    // virtual void device(const char* dev, DeviceType dt, int rows, int cols) {};

    mCurrFrameStartTime = getMilliseconds2();
    if(type == startFrameSym) {
        const unsigned long dataPeriodMillisecs = 1000 / mDataFreq;
        mCurrFrameStartTime = getMilliseconds2();
        if (mCurrFrameStartTime > mLastFrameStartTime + dataPeriodMillisecs) 
        {
            mTimeToSendNewFrame = true;
            mLastFrameStartTime = mCurrFrameStartTime;
        } 
        else 
        {
            mTimeToSendNewFrame = false;
        }

    }
    else if (type == touchSym)
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

        // float fNote =  note + vibrato;

        if (subtype == onSym)
        {
            callback_-> touch(serialNumber_.c_str(), mCurrFrameStartTime, true, voiceIdx, note, x, y, z);
        }
        if (subtype == continueSym)
        {
            if(mTimeToSendNewFrame) 
            {
                callback_-> touch(serialNumber_.c_str(), mCurrFrameStartTime, true, voiceIdx, note, x, y, z);
            }
        }
        if (subtype == offSym)
        {
            callback_-> touch(serialNumber_.c_str(), mCurrFrameStartTime, false, voiceIdx, note, x, y, z);
        }
    }
    else if (type == controllerSym) {
        int zoneID = msg->mData[0];
        if (subtype == xSym)
        {
            callback_->global(serialNumber_.c_str(), mCurrFrameStartTime, zoneID, x);
        }
        else if (subtype == ySym)
        {
            callback_->global(serialNumber_.c_str(), mCurrFrameStartTime, zoneID, y);
        }
        else if (subtype == zSym)
        {
//               callback_->global(serialNumber_.c_str(),mCurrFrameStartTime, zoneID, z);
        }
        else if (subtype == xyzSym)
        {
            callback_->global(serialNumber_.c_str(), mCurrFrameStartTime, zoneID, x);
//               callback_->global(serialNumber_.c_str(),mCurrFrameStartTime, zoneID, y);
//               callback_->global(serialNumber_.c_str(),mCurrFrameStartTime, zoneID, z);
        }
        else if (subtype == toggleSym)
        {
            callback_->global(serialNumber_.c_str(), mCurrFrameStartTime, zoneID, x > 0.5 ? 1 : 0);
        }
        else if(type == endFrameSym) 
        {
            //nop
        }
    }
}