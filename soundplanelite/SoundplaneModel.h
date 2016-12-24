
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __SOUNDPLANE_MODEL__
#define __SOUNDPLANE_MODEL__

#include <list>
#include <map>
#include <stdint.h>

#include "MLModel.h"
#include "SoundplaneModelA.h"
#include "SoundplaneDriver.h"
#include "SoundplaneDataListener.h"
#include "TouchTracker.h"
#include "MLSymbol.h"
#include "MLParameter.h"
#include "cJSON.h"
#include "Zone.h"

#include "SoundplaneOSCOutput.h"
#include "SoundplaneMECOutput.h"


class SoundplaneModel :
	public SoundplaneDriverListener,
	public TouchTracker::Listener,
	public MLModel
{
public:

	SoundplaneModel();
	~SoundplaneModel();

	// MLModel
    void doPropertyChangeAction(MLSymbol , const MLProperty & ) override;

	void setAllPropertiesToDefaults();

	// SoundplaneDriverListener
	virtual void deviceStateChanged(SoundplaneDriver& driver, MLSoundplaneState s) override;
	virtual void receivedFrame(SoundplaneDriver& driver, const float* data, int size) override;
	virtual void handleDeviceError(int errorType, int data1, int data2, float fd1, float fd2) override;
	virtual void handleDeviceDataDump(const float* pData, int size) override;

	// TouchTracker::Listener
	void hasNewCalibration(const MLSignal& cal, const MLSignal& norm, float avgDist) override;


	void initialize();
	void clearTouchData();
	void sendTouchDataToZones();
    void sendMessageToListeners();


	float getSampleHistory(int x, int y);

	void getHistoryStats(float& mean, float& stdDev);
	int getWidth() { return mSurface.getWidth(); }
	int getHeight() { return mSurface.getHeight(); }

	void setDefaultCarriers();
	void setCarriers(const SoundplaneDriver::Carriers& c);
	int enableCarriers(unsigned long mask);
	int getNumCarriers() { return kSoundplaneSensorWidth; }
	void dumpCarriers();

	void enableOutput(bool b);

	int getStateIndex();
	const char* getHardwareStr();
	const char* getStatusStr();

	int getSerialNumber() const {return mSerialNumber;}

	void clear();

	void setRaw(bool b);
	bool getRaw(){ return mRaw; }

	void beginCalibrate();
	bool isCalibrating() { return mCalibrating; }
	float getCalibrateProgress();
	void endCalibrate();

	void beginSelectCarriers();
	bool isSelectingCarriers() { return mSelectingCarriers; }
	float getSelectCarriersProgress();
	void nextSelectCarriersStep();
	void endSelectCarriers();

	void setFilter(bool b);

	void getMinMaxHistory(int n);
	const MLSignal& getCorrelation();
	void setTaxelsThresh(int t) { mTracker.setTaxelsThresh(t); }

	const MLSignal& getTouchFrame() { return mTouchFrame; }
	const MLSignal& getTouchHistory() { return mTouchHistory; }
	//const MLSignal& getRawSignal() { return mRawSignal; }
	//const MLSignal& getCalibratedSignal() { return mCalibratedSignal; }
	//const MLSignal& getCookedSignal() { return mCookedSignal; }
	//const MLSignal& getTestSignal() { return mTestSignal; }
	const MLSignal& getTrackerCalibrateSignal();
	Vec3 getTrackerCalibratePeak();
	bool isWithinTrackerCalibrateArea(int i, int j);
	const int getHistoryCtr() { return mHistoryCtr; }

    const std::vector<ZonePtr>& getZones(){ return mZones; }

    void setStateFromJSON(cJSON* pNode, int depth);
    bool loadZonePresetByName(const std::string& name);

	int getDeviceState(void);

	void beginNormalize();
	void cancelNormalize();
	bool trackerIsCalibrating();
	bool trackerIsCollectingMap();
	void setDefaultNormalize();
	Vec2 getTrackerCalibrateDims() { return Vec2(kCalibrateWidth, kCalibrateHeight); }
	Vec2 xyToKeyGrid(Vec2 xy);

	SoundplaneMECOutput& mecOutput() { return mMECOutput;}

private:
	void addListener(SoundplaneDataListener* pL) { mListeners.push_back(pL); }
	SoundplaneListenerList mListeners;

    void clearZones();
    void sendParametersToZones();
    void addZone(ZonePtr pz);

    std::vector<ZonePtr> mZones;
    MLSignal mZoneMap;

	bool mOutputEnabled;

	static const int miscStrSize = 256;
    void loadZonesFromString(const std::string& zoneStr);

	void doInfrequentTasks();
	int mLastInfrequentTaskTime;

	/**
	 * Please note that it is not safe to access this member from the processing
	 * thread: It is nulled out by the destructor before the SoundplaneDriver
	 * is torn down. (It would not be safe to not null it out either because
	 * then the pointer would point to an object that's being destroyed.)
	 */
	std::unique_ptr<SoundplaneDriver> mpDriver;
	int mSerialNumber;

    SoundplaneDataMessage mMessage;

	MLSignal mSurface;
	MLSignal mCalibrateData;

	int	mMaxTouches;
	MLSignal mTouchFrame;
	MLSignal mTouchHistory;

	bool mCalibrating;
	bool mSelectingCarriers;
	bool mRaw;
    bool mSendMatrixData;

	// when on, calibration tries to collect the lowest noise carriers to use.  otherwise a default set is used.
	//
	bool mDynamicCarriers;
	SoundplaneDriver::Carriers mCarriers;

	bool mHasCalibration;
	MLSignal mCalibrateSum;
	MLSignal mCalibrateMean;
	MLSignal mCalibrateMeanInv;
	MLSignal mCalibrateStdDev;

	//MLSignal mRawSignal;
	//MLSignal mCalibratedSignal;
	//MLSignal mCookedSignal;
	//MLSignal mTestSignal;

	int mCalibrateCount; // samples in one calibrate step
	int mCalibrateStep; // calibrate step from 0 - end
	int mTotalCalibrateSteps;
	int mSelectCarriersStep;

	float mSurfaceWidthInv;
	float mSurfaceHeightInv;

	Biquad2D mNotchFilter;
	Biquad2D mLopassFilter;
	BoxFilter2D mBoxFilter;

    // store current key for each touch to implement hysteresis.
	int mCurrentKeyX[kSoundplaneMaxTouches];
	int mCurrentKeyY[kSoundplaneMaxTouches];

	char mHardwareStr[miscStrSize];
	char mStatusStr[miscStrSize];

	TouchTracker mTracker;

	int mHistoryCtr;

	bool mCarrierMaskDirty;
	bool mNeedsCarriersSet;
	bool mNeedsCalibrate;
	unsigned long mCarriersMask;

	std::vector<float> mMaxNoiseByCarrierSet;
	std::vector<float> mMaxNoiseFreqByCarrierSet;

    SoundplaneOSCOutput mOSCOutput;
    SoundplaneMECOutput mMECOutput;
};

// JSON utilities (to go where?)
std::string getJSONString(cJSON* pNode, const char* name);
double getJSONDouble(cJSON* pNode, const char* name);
int getJSONInt(cJSON* pNode, const char* name);

#endif // __SOUNDPLANE_MODEL__
