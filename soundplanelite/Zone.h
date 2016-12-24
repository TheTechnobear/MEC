//
//  Zone.h
//  Soundplane
//
//  Created by Randy Jones on 10/18/13.
//
//

#ifndef __Soundplane__SoundplaneZone__
#define __Soundplane__SoundplaneZone__

#include "MLModel.h"
#include "SoundplaneModelA.h"
#include "SoundplaneDriver.h"
#include "SoundplaneDataListener.h"
#include "TouchTracker.h"
#include "MLSymbol.h"
#include "MLParameter.h"
#include <list>
#include <map>
#include "cJSON.h"

enum ZoneType
{
    kNoteRow,
    kControllerX,
    kControllerY,
    kControllerXY,
    kControllerXYZ,
    kControllerZ,
    kToggle,
    kZoneTypes
};

const int kZoneValArraySize = 8;

class ZoneTouch
{
public:
	ZoneTouch() { clear(); }
	ZoneTouch(float px, float py, int ix, int iy, float pz, float pdz) : pos(px, py, pz, pdz), kx(ix), ky(iy) {}
    void clear(){ pos.set(0.); }
    bool isActive() const { return pos.z() > 0.f; }
    
    Vec4 pos;
    
    // store the current key the touch is in, which due to hysteresis may not be
    // the one directly under the position.
    int kx;
    int ky;
};

class Zone
{
    friend class SoundplaneModel;
public:
    Zone(const SoundplaneListenerList& l);
    ~Zone();

    static int symbolToZoneType(MLSymbol s);

    void clearTouches();
    void addTouchToFrame(int i, float x, float y, int kx, int ky, float z, float dz);
    void processTouches(const std::vector<bool>& freedTouches);
    
    const ZoneTouch touchToKeyPos(const ZoneTouch& t) const
    {
        return ZoneTouch(mXRange(t.pos.x()), mYRange(t.pos.y()), t.kx, t.ky, t.pos.z(), t.pos.w());
    }
    float getQuantizeAmt(const ZoneTouch& t) const;
    
    // getters
    int setZoneID() const { return mZoneID; }
    const ZoneTouch getTouch(int i) const { return mTouches1[i]; }
    bool needsRedraw() const { return mNeedsRedraw; }
    const std::string& getName() const { return mName; }
    MLRect getBounds() const { return mBounds; }
    int getType() const { return mType; }
    bool isQuantized() const { return mQuantize; }
	int getOffset() const { return mOffset; }
    // return values on [0..1]
    float getValue(int i) const { return mValue[clamp(i, 0, kZoneValArraySize - 1)]; }
    float getXValue() const { return getValue(0); }
    float getYValue() const { return getValue(1); }

    // return values scaled to key grid
    float getXKeyPos() const { return mXRange(getValue(0)); }
    float getYKeyPos() const { return mYRange(getValue(1)); }

    bool getToggleValue() const { return (getValue(0) > 0.5f); }

    // setters
    void setZoneID(int z) { mZoneID = z; }
    void setSnapFreq(float f);
    void setBounds(MLRect b);
    void setNeedsRedraw(bool b) { mNeedsRedraw = b; }
    
    // TODO look at usage wrt. x/y/z display and make these un-public again
    MLRect mBounds;
    MLRange mXRange;
    MLRange mYRange;
    MLRange mXRangeInv;
    MLRange mYRangeInv;
    
protected:
    int mZoneID;
    int mType;
    int mStartNote;
    
    float mVibrato;
    float mHysteresis;
    bool mQuantize;
    bool mNoteLock;
    int mTranspose;
    
    // start note falls on this degree of scale-- for diatonic and other non-chromatic scales
    int mScaleNoteOffset;
    
    MLSignal mScaleMap;
    int mControllerNum1;
    int mControllerNum2;
    int mControllerNum3;
    int mOffset;
    std::string mName;
    const SoundplaneListenerList& mListeners;
    SoundplaneDataMessage mMessage;
    
private:
    void processTouchesNoteRow(const std::vector<bool>& freedTouches);
	void processTouchesNoteOffs(std::vector<bool>& freedTouches);
    int getNumberOfActiveTouches() const;
    int getNumberOfNewTouches() const;
    Vec3 getAveragePositionOfActiveTouches() const;
    float getMaxZOfActiveTouches() const;
    void processTouchesControllerX();
    void processTouchesControllerY();
    void processTouchesControllerXY();
    void processTouchesControllerXYZ();
    void processTouchesControllerToggle();
    void processTouchesControllerPressure();
    void sendMessage(MLSymbol type, MLSymbol subType, float a, float b=0, float c=0, float d=0, float e=0, float f=0, float g=0, float h=0);
    void sendMessageToListeners();
    
    bool mNeedsRedraw;
    float mValue[kZoneValArraySize];

    // touch locations are stored scaled to [0..1] over the Zone boundary.
    // incoming touches 
    ZoneTouch mTouches0[kSoundplaneMaxTouches];
    // touch positions this frame
    ZoneTouch mTouches1[kSoundplaneMaxTouches];
    // touch positions saved at touch onsets
    ZoneTouch mStartTouches[kSoundplaneMaxTouches];
    
	float mSnapFreq;
	std::vector<MLBiquad> mNoteFilters;
	std::vector<MLBiquad> mVibratoFilters;

};
typedef std::shared_ptr<Zone> ZonePtr;

#endif /* defined(__Soundplane__SoundplaneZone__) */
