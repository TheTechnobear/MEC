
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "SoundplaneModel.h"
#include "SoundplaneModelA.h"

#include "pa_memorybarrier.h"

#include "InertSoundplaneDriver.h"

#include <string>
#include <fstream>
#include <streambuf>


const int kModelDefaultCarriersSize = 40;
const unsigned char kModelDefaultCarriers[kModelDefaultCarriersSize] =
{
	// 40 default carriers.  avoiding 16, 32 (always bad)
	6, 7, 8, 9,
	10, 11, 12, 13, 14,
	15, 17, 18, 19, 20,
	21, 22, 23, 24, 25,
	26, 27, 28, 29, 30,
	31, 33, 34, 35, 36,
	37, 38, 39, 40, 41,
	42, 43, 44, 45, 46,
	47
};

// make one of the possible standard carrier sets, skipping a range of carriers out of the
// middle of the 40 defaults.
//
static const int kStandardCarrierSets = 8;
static void makeStandardCarrierSet(SoundplaneDriver::Carriers &carriers, int set)
{
	int skipStart = set*4 + 2;
	skipStart = clamp(skipStart, 0, kSoundplaneSensorWidth);
	int skipSize = 8;
	carriers[0] = carriers[1] = 0;
	for(int i=2; i<skipStart; ++i)
	{
		carriers[i] = kModelDefaultCarriers[i];
	}
	for(int i=skipStart; i<kSoundplaneSensorWidth; ++i)
	{
		carriers[i] = kModelDefaultCarriers[i + skipSize];
	}
}

// --------------------------------------------------------------------------------
//
#pragma mark SoundplaneModel

SoundplaneModel::SoundplaneModel() :
	mZoneMap(kSoundplaneAKeyWidth, kSoundplaneAKeyHeight),
	mOutputEnabled(false),
	mLastInfrequentTaskTime(0),
	mSurface(kSoundplaneWidth, kSoundplaneHeight),

	//mRawSignal(kSoundplaneWidth, kSoundplaneHeight),
	//mCalibratedSignal(kSoundplaneWidth, kSoundplaneHeight),
	//mCookedSignal(kSoundplaneWidth, kSoundplaneHeight),
	//mTestSignal(kSoundplaneWidth, kSoundplaneHeight),

	mCalibrating(false),
	mSelectingCarriers(false),
	mDynamicCarriers(true),
	mHasCalibration(false),
	mCalibrateSum(kSoundplaneWidth, kSoundplaneHeight),
	mCalibrateMean(kSoundplaneWidth, kSoundplaneHeight),
	mCalibrateMeanInv(kSoundplaneWidth, kSoundplaneHeight),
	mCalibrateStdDev(kSoundplaneWidth, kSoundplaneHeight),
	//
	mNotchFilter(kSoundplaneWidth, kSoundplaneHeight),
	mLopassFilter(kSoundplaneWidth, kSoundplaneHeight),
	mBoxFilter(kSoundplaneWidth, kSoundplaneHeight),
	//
	//

	mTracker(kSoundplaneWidth, kSoundplaneHeight),

	mHistoryCtr(0),

	mCarrierMaskDirty(false),
	mNeedsCarriersSet(true),
	mNeedsCalibrate(true),
	mCarriersMask(0xFFFFFFFF)
{
	// setup geometry
	mSurfaceWidthInv = 1.f / (float)mSurface.getWidth();
	mSurfaceHeightInv = 1.f / (float)mSurface.getHeight();
	
	// setup box filter.
	mBoxFilter.setSampleRate(kSoundplaneSampleRate);
	mBoxFilter.setN(7);

	// setup fixed notch
	mNotchFilter.setSampleRate(kSoundplaneSampleRate);
	mNotchFilter.setNotch(150., 0.707);
	
	// setup fixed lopass.
	mLopassFilter.setSampleRate(kSoundplaneSampleRate);
	mLopassFilter.setLopass(50, 0.707);
	
	for(int i=0; i<kSoundplaneMaxTouches; ++i)
	{
		mCurrentKeyX[i] = -1;
		mCurrentKeyY[i] = -1;
	}

	mTracker.setSampleRate(kSoundplaneSampleRate);

	// setup default carriers in case there are no saved carriers
	for (int car=0; car<kSoundplaneSensorWidth; ++car)
	{
		mCarriers[car] = kModelDefaultCarriers[car];
	}

    clearZones();

	setAllPropertiesToDefaults();

	mTracker.setListener(this);
}

SoundplaneModel::~SoundplaneModel()
{
	// Ensure the SoundplaneDriver is town down before anything else in this
	// object. This is important because otherwise there might be processing
	// thread callbacks that fly around too late.
	mpDriver.reset(new InertSoundplaneDriver());
}

void SoundplaneModel::doPropertyChangeAction(MLSymbol p, const MLProperty & newVal)
{
     //debug() << "SoundplaneModel::doPropertyChangeAction: " << p << " -> " << newVal << "\n";

	int propertyType = newVal.getType();
	switch(propertyType)
	{
		case MLProperty::kFloatProperty:
		{
			float v = newVal.getFloatValue();
			if (p.withoutFinalNumber() == MLSymbol("carrier_toggle"))
			{
				// toggles changed -- mute carriers
				unsigned long mask = 0;
				for(int i=0; i<32; ++i)
				{
					MLSymbol tSym = MLSymbol("carrier_toggle").withFinalNumber(i);
					bool on = (int)(getFloatProperty(tSym));
					mask = mask | (on << i);
				}

				mCarriersMask = mask;
				mCarrierMaskDirty = true; // trigger carriers set in a second or so
			}

			else if (p == "all_toggle")
			{
				bool on = (bool)(v);
				for(int i=0; i<32; ++i)
				{
					MLSymbol tSym = MLSymbol("carrier_toggle").withFinalNumber(i);
					setProperty(tSym, on);
				}
				mCarriersMask = on ? ~0 : 0;
				mCarrierMaskDirty = true; // trigger carriers set in a second or so
			}
			else if (p == "max_touches")
			{
				mTracker.setMaxTouches(v);
                mOSCOutput.setMaxTouches(v);
                mMECOutput.setMaxTouches(v);
			}
			else if (p == "lopass")
			{
				mTracker.setLopass(v);
			}

			else if (p == "z_thresh")
			{
				mTracker.setThresh(v);
			}
			else if (p == "z_scale")
			{
				mTracker.setZScale(v);
			}
			else if (p == "z_curve")
			{
				mTracker.setForceCurve(v);
			}
			else if (p == "snap")
			{
				sendParametersToZones();
			}
			else if (p == "vibrato")
			{
				sendParametersToZones();
			}
			else if (p == "lock")
			{
				sendParametersToZones();
			}
            else if (p == "data_freq_osc")
            {
                mOSCOutput.setDataFreq(v);
            }
            else if (p == "data_freq_mec")
            {
                mMECOutput.setDataFreq(v);
            }
            else if (p == "osc_active")
            {
                bool b = v;
                mOSCOutput.setActive(b);
            }
            else if (p == "mec_active")
            {
                bool b = v;
                mMECOutput.setActive(b);
            }
            else if (p == "osc_send_matrix")
            {
                //bool b = v;
                //mSendMatrixData = b;
            }
			else if (p == "t_thresh")
			{
				mTracker.setTemplateThresh(v);
			}
			else if (p == "bg_filter")
			{
				mTracker.setBackgroundFilter(v);
			}
			else if (p == "quantize")
			{
				bool b = v;
				mTracker.setQuantize(b);
				sendParametersToZones();
			}
			else if (p == "rotate")
			{
				bool b = v;
				mTracker.setRotate(b);
			}
			else if (p == "glissando")
			{
				sendParametersToZones();
			}
			else if (p == "hysteresis")
			{
				sendParametersToZones();
			}
			else if (p == "transpose")
			{
				sendParametersToZones();
			}
			else if (p == "bend_range")
			{
				sendParametersToZones();
			}
		}
		break;
		case MLProperty::kStringProperty:
		{
			const std::string& str = newVal.getStringValue();
            if(p == "osc_service_name")
            {
                if(str == "default")
                {
                    // connect via number directly to default port
                    mOSCOutput.connect("localhost", 3123);
                }
                else 
                {
                    //int port = newVal.getIntValue();
                    //mOSCOutput.connect("localhost", port);
                }
            }
			else if (p == "zone_JSON")
			{
				loadZonesFromString(str);
			}
            //#define ZONEPRESETS
            #ifdef ZONEPRESETS
			else if (p == "zone_preset")
			{
                std::ifstream t(str);
                if(t.good()) 
                {
                    std::string jsonStr;
                    t.seekg(0, std::ios::end);   
                    jsonStr.reserve(t.tellg());
                    t.seekg(0, std::ios::beg);

                    jsonStr.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                    setPropertyImmediate("zone_JSON", jsonStr);
                }
			}
            #endif //ZONEPRESETS
            
		}
		break;
		case MLProperty::kSignalProperty:
		{
			const MLSignal& sig = newVal.getSignalValue();
			if(p == MLSymbol("carriers"))
			{
				// get carriers from signal
				assert(sig.getSize() == kSoundplaneSensorWidth);
				for(int i=0; i<kSoundplaneSensorWidth; ++i)
				{
					mCarriers[i] = sig[i];
				}
				mNeedsCarriersSet = true;
			}
			if(p == MLSymbol("tracker_calibration"))
			{
				mTracker.setCalibration(sig);
			}
			if(p == MLSymbol("tracker_normalize"))
			{
				mTracker.setNormalizeMap(sig);
			}

		}
			break;
		default:
			break;
	}
}

void SoundplaneModel::setAllPropertiesToDefaults()
{
	// parameter defaults and creation
	setProperty("max_touches", 4);
	setProperty("lopass", 100.);

	setProperty("z_thresh", 0.01);
	setProperty("z_scale", 1.);
	setProperty("z_curve", 0.25);
	setProperty("display_scale", 1.);

	setProperty("quantize", 1.);
	setProperty("lock", 0.);
	setProperty("abs_rel", 0.);
	setProperty("snap", 250.);
	setProperty("vibrato", 0.5);

	setProperty("t_thresh", 0.2);

	setProperty("bend_range", 48);
	setProperty("transpose", 0);
	setProperty("bg_filter", 0.05);

	setProperty("hysteresis", 0.5);

	// menu param defaults
	setProperty("viewmode", "calibrated");

    // preset menu defaults (TODO use first choices?)
	setProperty("zone_preset", "rows in fourths");
	setProperty("touch_preset", "touch default");

	setProperty("view_page", 0);

	for(int i=0; i<32; ++i)
	{
		setProperty(MLSymbol("carrier_toggle").withFinalNumber(i), 1);
	}
}


void SoundplaneModel::initialize()
{
    addListener(&mOSCOutput);
    addListener(&mMECOutput);
	mpDriver = SoundplaneDriver::create(this);

	// TODO mem err handling
	if (!mCalibrateData.setDims(kSoundplaneWidth, kSoundplaneHeight, kSoundplaneCalibrateSize))
	{
		MLConsole() << "SoundplaneModel: out of memory!\n";
	}

	mTouchFrame.setDims(kTouchWidth, kSoundplaneMaxTouches);
	mTouchHistory.setDims(kTouchWidth, kSoundplaneMaxTouches, kSoundplaneHistorySize);
}

int SoundplaneModel::getDeviceState(void)
{
	return mpDriver->getDeviceState();
}

void SoundplaneModel::deviceStateChanged(SoundplaneDriver& driver, MLSoundplaneState s)
{
    unsigned long instrumentModel = 1; // Soundplane A

	PaUtil_WriteMemoryBarrier();

	switch(s)
	{
		case kNoDevice:
		break;
		case kDeviceConnected:
			// connected but not calibrated -- disable output.
			enableOutput(false);
		break;
		case kDeviceHasIsochSync:
            mOSCOutput.setSerialNumber((instrumentModel << 16) | driver.getSerialNumber());
            mMECOutput.setSerialNumber((instrumentModel << 16) | driver.getSerialNumber());
			mNeedsCarriersSet = true;
			// output will be enabled at end of calibration.
			mNeedsCalibrate = true;
		break;
		case kDeviceIsTerminating:
		break;
		case kDeviceSuspend:
		break;
		case kDeviceResume:
		break;
	}
}


void SoundplaneModel::receivedFrame(SoundplaneDriver& driver, const float* data, int size)
{
    // do once every so many frames
	if(mLastInfrequentTaskTime > 1000)
	{
		doInfrequentTasks();
		mLastInfrequentTaskTime = 0;
	}
    else 
    {
        mLastInfrequentTaskTime++;
    }

	// read from driver's ring buffer to incoming surface
	MLSample* pSurfaceData = mSurface.getBuffer();
	memcpy(pSurfaceData, data, size * sizeof(float));

	// store surface for raw output
	//mRawSignal.copy(mSurface);

	
	if (mCalibrating)
	{
		// copy surface to a frame of 3D calibration buffer
		mCalibrateData.setFrame(mCalibrateCount++, mSurface);
		if (mCalibrateCount >= kSoundplaneCalibrateSize)
		{
			endCalibrate();
		}
	}
	else if (mSelectingCarriers)
	{
		// copy surface to a frame of 3D calibration buffer
		mCalibrateData.setFrame(mCalibrateCount++, mSurface);
		if (mCalibrateCount >= kSoundplaneCalibrateSize)
		{
			nextSelectCarriersStep();
		}
	}
	else if(mOutputEnabled)
	{
		// scale incoming data
		float in, cmean, cout;
		float epsilon = 0.000001;
		if (mHasCalibration)
		{
			for(int j=0; j<mSurface.getHeight(); ++j)
			{
				for(int i=0; i<mSurface.getWidth(); ++i)
				{
					// scale to 1/z curve
					in = mSurface(i, j);
					cmean = mCalibrateMean(i, j);
					cout = (1.f - ((cmean + epsilon) / (in + epsilon)));
					mSurface(i, j) = cout;
				}
			}
		}
		
		// filter data in time
		mBoxFilter.setInputSignal(&mSurface);
		mBoxFilter.setOutputSignal(&mSurface);
		mBoxFilter.process(1);	
		mNotchFilter.setInputSignal(&mSurface);
		mNotchFilter.setOutputSignal(&mSurface);
		mNotchFilter.process(1);
		mLopassFilter.setInputSignal(&mSurface);
		mLopassFilter.setOutputSignal(&mSurface);
		mLopassFilter.process(1);

		// send filtered data to touch tracker.
		mTracker.setInputSignal(&mSurface);
		mTracker.setOutputSignal(&mTouchFrame);
		mTracker.process(1);

		// get calibrated and cooked signals for viewing
		//mCalibratedSignal = mTracker.getCalibratedSignal();
		//mCookedSignal = mTracker.getCookedSignal();
		//mTestSignal = mTracker.getTestSignal();

 		sendTouchDataToZones();

		mHistoryCtr++;
		if (mHistoryCtr >= kSoundplaneHistorySize) mHistoryCtr = 0;
		mTouchHistory.setFrame(mHistoryCtr, mTouchFrame);
	}
}

void SoundplaneModel::handleDeviceError(int errorType, int data1, int data2, float fd1, float fd2)
{
	switch(errorType)
	{
		case kDevDataDiffTooLarge:
			if(!mSelectingCarriers)
			{
				MLConsole() << "note: diff too large (" << fd1 << ")\n";
				MLConsole() << "startup count = " << data1 << "\n";
			}
			break;
		case kDevGapInSequence:
			MLConsole() << "note: gap in sequence (" << data1 << " -> " << data2 << ")\n";
			break;
		case kDevNoErr:
		default:
			MLConsole() << "SoundplaneModel::handleDeviceError: unknown error!\n";
			break;
	}
}

void SoundplaneModel::handleDeviceDataDump(const float* pData, int size)
{
	if(mSelectingCarriers) return;

	debug() << "----------------------------------------------------------------\n ";
	int c = 0;
	int w = getWidth();
	int row = 0;
	debug() << std::setprecision(2);

	debug() << "[0] ";
	for(int i=0; i<size; ++i)
	{
		debug() << pData[i] << " ";
		c++;
		if(c == w/2) 
        {
			debug() << " / ";
        }
		if(c >= w)
		{
			debug() << "\n";
			c = 0;
			if (i < (size - 1))
			{
				debug() << "[" << ++row << "] ";
			}
		}
	}
	debug() << "\n";
}

// when calibration is done, set params to save entire calibration signal and
// set template threshold based on average distance
void SoundplaneModel::hasNewCalibration(const MLSignal& cal, const MLSignal& norm, float avgDistance)
{
	if(avgDistance > 0.f)
	{
		setProperty("tracker_calibration", cal);
		setProperty("tracker_normalize", norm);
		float thresh = avgDistance * 1.75f;
		MLConsole() << "SoundplaneModel::hasNewCalibration: calculated template threshold: " << thresh << "\n";
		setProperty("t_thresh", thresh);
	}
	else
	{
		// set default calibration
		setProperty("tracker_calibration", cal);
		setProperty("tracker_normalize", norm);
		float thresh = 0.2f;
		MLConsole() << "SoundplaneModel::hasNewCalibration: default template threshold: " << thresh << "\n";
		setProperty("t_thresh", thresh);
	}
}

// get a string that explains what Soundplane hardware and firmware and client versions are running.
const char* SoundplaneModel::getHardwareStr()
{
	long v;
	unsigned char a, b, c;
	std::string serial_number;
	switch(getDeviceState())
	{
		case kNoDevice:
			snprintf(mHardwareStr, miscStrSize, "no device");
			break;
		case kDeviceConnected:
		case kDeviceHasIsochSync:

			serial_number = mpDriver->getSerialNumberString();

			v = mpDriver->getFirmwareVersion();
			a = v >> 8 & 0x0F;
			b = v >> 4 & 0x0F,
			c = v & 0x0F;

			snprintf(mHardwareStr, miscStrSize, "%s #%s, firmware %d.%d.%d", kSoundplaneAName, serial_number.c_str(), a, b, c);
			break;

		default:
			snprintf(mHardwareStr, miscStrSize, "?");
			break;
	}
	return mHardwareStr;
}

// get the string to report general connection status.
const char* SoundplaneModel::getStatusStr()
{
	switch(getDeviceState())
	{
		case kNoDevice:
			snprintf(mStatusStr, miscStrSize, "waiting for Soundplane...");
			break;

		case kDeviceConnected:
			snprintf(mStatusStr, miscStrSize, "waiting for isochronous data...");
			break;

		case kDeviceHasIsochSync:
			snprintf(mStatusStr, miscStrSize, "synchronized");
			break;

		default:
			snprintf(mStatusStr, miscStrSize, "unknown status.");
			break;
	}
	return mStatusStr;
}

// remove all zones from the zone list.
void SoundplaneModel::clearZones()
{
    mZones.clear();
    mZoneMap.fill(-1);
}

// add a zone to the zone list and color in its boundary on the map.
void SoundplaneModel::addZone(ZonePtr pz)
{
    // TODO prevent overlapping zones
    int zoneIdx = mZones.size();
    if(zoneIdx < kSoundplaneAMaxZones)
    {
        pz->setZoneID(zoneIdx);
        mZones.push_back(pz);
        MLRect b(pz->getBounds());
        int x = b.x();
        int y = b.y();
        int w = b.width();
        int h = b.height();

        for(int j=y; j < y + h; ++j)
        {
            for(int i=x; i < x + w; ++i)
            {
                mZoneMap(i, j) = zoneIdx;
            }
        }
    }
    else
    {
        MLConsole() << "SoundplaneModel::addZone: out of zones!\n";
    }
}

void SoundplaneModel::loadZonesFromString(const std::string& zoneStr)
{
    clearZones();
    cJSON* root = cJSON_Parse(zoneStr.c_str());
    if(!root)
    {
        MLConsole() << "zone file parse failed!\n";
        const char* errStr = cJSON_GetErrorPtr();
        MLConsole() << "    error at: " << errStr << "\n";
        return;
    }
    cJSON* pNode = root->child;
    while(pNode)
    {
        if(!strcmp(pNode->string, "zone"))
        {
            Zone* pz = new Zone(mListeners);
            cJSON* pZoneType = cJSON_GetObjectItem(pNode, "type");
            if(pZoneType)
            {
                // get zone type and type specific attributes
                MLSymbol typeSym(pZoneType->valuestring);
                int zoneTypeNum = Zone::symbolToZoneType(typeSym);
                if(zoneTypeNum >= 0)
                {
                    pz->mType = zoneTypeNum;
                }
                else
                {
                    MLConsole() << "Unknown type " << typeSym << " for zone!\n";
                }
            }
            else
            {
                MLConsole() << "No type for zone!\n";
            }

            // get zone rect
            cJSON* pZoneRect = cJSON_GetObjectItem(pNode, "rect");
            if(pZoneRect)
            {
                int size = cJSON_GetArraySize(pZoneRect);
                if(size == 4)
                {
                    int x = cJSON_GetArrayItem(pZoneRect, 0)->valueint;
                    int y = cJSON_GetArrayItem(pZoneRect, 1)->valueint;
                    int w = cJSON_GetArrayItem(pZoneRect, 2)->valueint;
                    int h = cJSON_GetArrayItem(pZoneRect, 3)->valueint;
                    pz->setBounds(MLRect(x, y, w, h));
                }
                else
                {
                    MLConsole() << "Bad rect for zone!\n";
                }
            }
            else
            {
                MLConsole() << "No rect for zone\n";
            }

            pz->mName = getJSONString(pNode, "name");
            pz->mStartNote = getJSONInt(pNode, "note");
            pz->mOffset = getJSONInt(pNode, "offset");
            pz->mControllerNum1 = getJSONInt(pNode, "ctrl1");
            pz->mControllerNum2 = getJSONInt(pNode, "ctrl2");
            pz->mControllerNum3 = getJSONInt(pNode, "ctrl3");

            addZone(ZonePtr(pz));
           //  mZoneMap.dump(mZoneMap.getBoundsRect());
        }
		pNode = pNode->next;
    }
    sendParametersToZones();
}

// turn (x, y) position into a continuous 2D key position.
// Soundplane A only.
Vec2 SoundplaneModel::xyToKeyGrid(Vec2 xy)
{
	MLRange xRange(4.5f, 60.5f);
	xRange.convertTo(MLRange(1.5f, 29.5f));
	float kx = clamp(xRange(xy.x()), 0.f, (float)kSoundplaneAKeyWidth);

	MLRange yRange(1., 6.);  // Soundplane A as measured with kNormalizeThresh = .125
	yRange.convertTo(MLRange(1.f, 4.f));
    float scaledY = yRange(xy.y());
	float ky = clamp(scaledY, 0.f, (float)kSoundplaneAKeyHeight);

	return Vec2(kx, ky);
}

void SoundplaneModel::clearTouchData()
{
	const int maxTouches = getFloatProperty("max_touches");
	for(int i=0; i<maxTouches; ++i)
	{
		mTouchFrame(xColumn, i) = 0;
		mTouchFrame(yColumn, i) = 0;
		mTouchFrame(zColumn, i) = 0;
		mTouchFrame(dzColumn, i) = 0;
		mTouchFrame(ageColumn, i) = 0;
		mTouchFrame(dtColumn, i) = 1.;
		mTouchFrame(noteColumn, i) = -1;
		mTouchFrame(reservedColumn, i) = 0;
	}
}

// copy relevant parameters from Model to zones
void SoundplaneModel::sendParametersToZones()
{
    // TODO zones should have parameters (really attributes) too, so they can be inspected.
    int zones = mZones.size();
	const float v = getFloatProperty("vibrato");
    const float h = getFloatProperty("hysteresis");
    bool q = getFloatProperty("quantize");
    bool nl = getFloatProperty("lock");
    int t = getFloatProperty("transpose");
    float sf = getFloatProperty("snap");

    for(int i=0; i<zones; ++i)
	{
        mZones[i]->mVibrato = v;
        mZones[i]->mHysteresis = h;
        mZones[i]->mQuantize = q;
        mZones[i]->mNoteLock = nl;
        mZones[i]->mTranspose = t;
        mZones[i]->setSnapFreq(sf);
    }
}

// send raw touches to zones in order to generate note and controller events.
void SoundplaneModel::sendTouchDataToZones()
{
	const float kTouchScaleToModel = 20.f;
	float x, y, z, dz;
	int age;

	const float zscale = getFloatProperty("z_scale");
	const float zcurve = getFloatProperty("z_curve");
	const int maxTouches = getFloatProperty("max_touches");
	const float hysteresis = getFloatProperty("hysteresis");

	MLRange yRange(0.05, 0.8);
	yRange.convertTo(MLRange(0., 1.));

    for(int i=0; i<maxTouches; ++i)
	{
		age = mTouchFrame(ageColumn, i);
        x = mTouchFrame(xColumn, i);
        y = mTouchFrame(yColumn, i);
        z = mTouchFrame(zColumn, i);
        dz = mTouchFrame(dzColumn, i);
		if(age > 0)
		{
 			// apply adjustable force curve for z and clamp
			z *= zscale * kTouchScaleToModel;
			z = (1.f - zcurve)*z + zcurve*z*z*z;
			z = clamp(z, 0.f, 1.f);
			mTouchFrame(zColumn, i) = z;

			// get fractional key grid position (Soundplane A)
			Vec2 keyXY = xyToKeyGrid(Vec2(x, y));
            float kgx = keyXY.x();
            float kgy = keyXY.y();

            // get integer key
            int ix = (int)(keyXY.x());
            int iy = (int)(keyXY.y());

            // apply hysteresis to raw position to get current key
            // hysteresis: make it harder to move out of current key
            if(age == 1)
            {
                mCurrentKeyX[i] = ix;
                mCurrentKeyY[i] = iy;
            }
            else
            {
                float hystWidth = hysteresis*0.25f;
                MLRect currentKeyRect(mCurrentKeyX[i], mCurrentKeyY[i], 1, 1);
                currentKeyRect.expand(hystWidth);
                if(!currentKeyRect.contains(keyXY))
                {
                    mCurrentKeyX[i] = ix;
                    mCurrentKeyY[i] = iy;
                }
            }

            // send index, xyz to zone
            int zoneIdx = mZoneMap(mCurrentKeyX[i], mCurrentKeyY[i]);
            if(zoneIdx >= 0)
            {
                ZonePtr zone = mZones[zoneIdx];
                zone->addTouchToFrame(i, kgx, kgy, mCurrentKeyX[i], mCurrentKeyY[i], z, dz);
            }
        }
	}

    // tell listeners we are starting this frame.
    mMessage.mType = MLSymbol("start_frame");
	sendMessageToListeners();

    // process note offs for each zone
	// this happens before processTouches() to allow voices to be freed
    int zones = mZones.size();
	std::vector<bool> freedTouches;
	freedTouches.resize(kSoundplaneMaxTouches);

    for(int i=0; i<zones; ++i)
	{
        mZones[i]->processTouchesNoteOffs(freedTouches);
    }

    // process touches for each zone
	for(int i=0; i<zones; ++i)
	{
        mZones[i]->processTouches(freedTouches);
    }

    // TODO: not sure we want this...
#ifdef MATRIX_DATA
    // send optional calibrated matrix
    if(mSendMatrixData)
    {
        mMessage.mType = MLSymbol("matrix");
        for(int j = 0; j < kSoundplaneHeight; ++j)
        {
            for(int i = 0; i < kSoundplaneWidth; ++i)
            {
                mMessage.mMatrix[j*kSoundplaneWidth + i] = mCalibratedSignal(i, j);
            }
        }
        sendMessageToListeners();
    }
#endif

    // tell listeners we are done with this frame.
    mMessage.mType = MLSymbol("end_frame");
	sendMessageToListeners();
}

void SoundplaneModel::sendMessageToListeners()
{
 	for(SoundplaneListenerList::iterator it = mListeners.begin(); it != mListeners.end(); it++)
    if((*it)->isActive())
    {
        (*it)->processSoundplaneMessage(&mMessage);
    }
}

// --------------------------------------------------------------------------------
//
#pragma mark -

void SoundplaneModel::doInfrequentTasks()
{
    mOSCOutput.doInfrequentTasks();
    mMECOutput.doInfrequentTasks();

	if (mCarrierMaskDirty)
	{
		enableCarriers(mCarriersMask);
	}
	else if (mNeedsCarriersSet)
	{
		mNeedsCarriersSet = false;
		setCarriers(mCarriers);
		mNeedsCalibrate = true;
	}
	else if (mNeedsCalibrate)
	{
		mNeedsCalibrate = false;
		beginCalibrate();
	}
}

void SoundplaneModel::setDefaultCarriers()
{
	MLSignal cSig(kSoundplaneSensorWidth);
	for (int car=0; car<kSoundplaneSensorWidth; ++car)
	{
		cSig[car] = kModelDefaultCarriers[car];
	}
	setProperty("carriers", cSig);
}

void SoundplaneModel::setCarriers(const SoundplaneDriver::Carriers& c)
{
	enableOutput(false);
	mpDriver->setCarriers(c);
}

int SoundplaneModel::enableCarriers(unsigned long mask)
{
	mpDriver->enableCarriers(~mask);
	if (mask != mCarriersMask)
	{
		mCarriersMask = mask;
	}
	mCarrierMaskDirty = false;
	return 0;
}

void SoundplaneModel::dumpCarriers()
{
	debug() << "\n------------------\n";
	debug() << "carriers: \n";
	for(int i=0; i<kSoundplaneSensorWidth; ++i)
	{
		int c = mCarriers[i];
		debug() << i << ": " << c << " ["<< SoundplaneDriver::carrierToFrequency(c) << "Hz] \n";
	}
}

void SoundplaneModel::enableOutput(bool b)
{
	mOutputEnabled = b;
}

void SoundplaneModel::clear()
{
	mTracker.clear();
}

// --------------------------------------------------------------------------------
#pragma mark surface calibration

// using the current carriers, calibrate the surface by collecting data and
// calculating the mean and std. deviation for each taxel.
//
void SoundplaneModel::beginCalibrate()
{
	if(getDeviceState() == kDeviceHasIsochSync)
	{
		clear();

		clearTouchData();
		sendTouchDataToZones();

		mCalibrateCount = 0;
		mCalibrating = true;
	}
}

// called by process routine when enough samples have been collected.
//
void SoundplaneModel::endCalibrate()
{
	// skip frames after commands to allow noise to settle.
	int skipFrames = 100;
	int startFrame = skipFrames;
	int endFrame = kSoundplaneCalibrateSize - skipFrames;
	float calibrateFrames = endFrame - startFrame + 1;

	MLSignal calibrateSum(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal calibrateStdDev(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal dSum(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal dMean(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal mean(kSoundplaneWidth, kSoundplaneHeight);

	// get mean
	for(int i=startFrame; i<=endFrame; ++i)
	{
		// read frame from calibrate data.
		calibrateSum.add(mCalibrateData.getFrame(i));
	}
	mean = calibrateSum;
	mean.scale(1.f / calibrateFrames);
	mCalibrateMean = mean;
	mCalibrateMean.sigClamp(0.0001f, 2.f);

	// get std deviation
	for(int i=startFrame; i<endFrame; ++i)
	{
		dMean = mCalibrateData.getFrame(i);
		dMean.subtract(mean);
		dMean.square();
		dSum.add(dMean);
	}
	dSum.scale(1.f / calibrateFrames);
	calibrateStdDev = dSum;
	calibrateStdDev.sqrt();
	mCalibrateStdDev = calibrateStdDev;

	mCalibrating = false;
	mHasCalibration = true;

	mBoxFilter.clear();
	mNotchFilter.clear();
	mLopassFilter.clear();

	enableOutput(true);
}

float SoundplaneModel::getCalibrateProgress()
{
	return mCalibrateCount / (float)kSoundplaneCalibrateSize;
}

// --------------------------------------------------------------------------------
#pragma mark carrier selection

void SoundplaneModel::beginSelectCarriers()
{
	// each possible group of carrier frequencies is tested to see which
	// has the lowest overall noise.
	// each step collects kSoundplaneCalibrateSize frames of data.
	//
	if(getDeviceState() == kDeviceHasIsochSync)
	{
		mSelectCarriersStep = 0;
		mCalibrateCount = 0;
		mSelectingCarriers = true;
		mTracker.clear();
		mMaxNoiseByCarrierSet.resize(kStandardCarrierSets);
		mMaxNoiseByCarrierSet.clear();
		mMaxNoiseFreqByCarrierSet.resize(kStandardCarrierSets);
		mMaxNoiseFreqByCarrierSet.clear();

		// setup first set of carrier frequencies
		MLConsole() << "testing carriers set " << mSelectCarriersStep << "...\n";
		makeStandardCarrierSet(mCarriers, mSelectCarriersStep);
		setCarriers(mCarriers);
	}
}

float SoundplaneModel::getSelectCarriersProgress()
{
	float p;
	if(mSelectingCarriers)
	{
		p = (float)mSelectCarriersStep / (float)kStandardCarrierSets;
	}
	else
	{
		p = 0.f;
	}
	return p;
}

void SoundplaneModel::nextSelectCarriersStep()
{
	// clear data
	mCalibrateSum.clear();
	mCalibrateCount = 0;

	// analyze calibration data just collected.
	// it's necessary to skip around 100 frames at start and end to get good data, not sure why yet.
	int skipFrames = 100;
	int startFrame = skipFrames;
	int endFrame = kSoundplaneCalibrateSize - skipFrames;
	float calibrateFrames = endFrame - startFrame + 1;
	MLSignal calibrateSum(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal calibrateStdDev(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal dSum(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal dMean(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal mean(kSoundplaneWidth, kSoundplaneHeight);
	MLSignal noise(kSoundplaneWidth, kSoundplaneHeight);

	// get mean
	for(int i=startFrame; i<=endFrame; ++i)
	{
		// read frame from calibrate data.
		calibrateSum.add(mCalibrateData.getFrame(i));
	}
	mean = calibrateSum;
	mean.scale(1.f / calibrateFrames);
	mCalibrateMean = mean;
	mCalibrateMean.sigClamp(0.0001f, 2.f);

	// get std deviation
	for(int i=startFrame; i<endFrame; ++i)
	{
		dMean = mCalibrateData.getFrame(i);
		dMean.subtract(mean);
		dMean.square();
		dSum.add(dMean);
	}
	dSum.scale(1.f / calibrateFrames);
	calibrateStdDev = dSum;
	calibrateStdDev.sqrt();

	noise = calibrateStdDev;

	// find maximum noise in any column for this set.  This is the "badness" value
	// we use to compare carrier sets.
	float maxNoise = 0;
	float maxNoiseFreq = 0;
	float noiseSum;
	int startSkip = 2;
	for(int col = startSkip; col<kSoundplaneSensorWidth; ++col)
	{
		noiseSum = 0;
		int carrier = mCarriers[col];
		float cFreq = SoundplaneDriver::carrierToFrequency(carrier);

		for(int row=0; row<kSoundplaneHeight; ++row)
		{
			noiseSum += noise(col, row);
		}
//		debug() << "noise sum col " << col << " = " << noiseSum << " carrier " << carrier << " freq " << cFreq << "\n";

		if(noiseSum > maxNoise)
		{
			maxNoise = noiseSum;
			maxNoiseFreq = cFreq;
		}
	}

	mMaxNoiseByCarrierSet[mSelectCarriersStep] = maxNoise;
	mMaxNoiseFreqByCarrierSet[mSelectCarriersStep] = maxNoiseFreq;

	MLConsole() << "max noise for set " << mSelectCarriersStep << ": " << maxNoise << "(" << maxNoiseFreq << " Hz) \n";

	// set up next step.
	mSelectCarriersStep++;
	if (mSelectCarriersStep < kStandardCarrierSets)
	{
		// set next carrier frequencies to calibrate.
		MLConsole() << "testing carriers set " << mSelectCarriersStep << "...\n";
		makeStandardCarrierSet(mCarriers, mSelectCarriersStep);
		setCarriers(mCarriers);
	}
	else
	{
		endSelectCarriers();
	}
}

void SoundplaneModel::endSelectCarriers()
{
	// get minimum of collected noise sums
	float minNoise = 99999.f;
	int minIdx = -1;
	MLConsole() << "------------------------------------------------\n";
	MLConsole() << "carrier select noise results:\n";
	for(int i=0; i<kStandardCarrierSets; ++i)
	{
		float n = mMaxNoiseByCarrierSet[i];
		float h = mMaxNoiseFreqByCarrierSet[i];
		MLConsole() << "set " << i << ": max noise " << n << "(" << h << " Hz)\n";
		if(n < minNoise)
		{
			minNoise = n;
			minIdx = i;
		}
	}

	// set that carrier group
	MLConsole() << "setting carriers set " << minIdx << "...\n";
	makeStandardCarrierSet(mCarriers, minIdx);

	// set chosen carriers as model parameter so they will be saved
	// this will trigger a recalibrate
	MLSignal cSig(kSoundplaneSensorWidth);
	for (int car=0; car<kSoundplaneSensorWidth; ++car)
	{
		cSig[car] = mCarriers[car];
	}
	setProperty("carriers", cSig);
	MLConsole() << "carrier select done.\n";

	mSelectingCarriers = false;

	enableOutput(true);
}


const MLSignal& SoundplaneModel::getTrackerCalibrateSignal()
{
	return mTracker.getCalibrateSignal();
}

Vec3 SoundplaneModel::getTrackerCalibratePeak()
{
	return mTracker.getCalibratePeak();
}

bool SoundplaneModel::isWithinTrackerCalibrateArea(int i, int j)
{
	return mTracker.isWithinCalibrateArea(i, j);
}

// --------------------------------------------------------------------------------
#pragma mark tracker calibration

void SoundplaneModel::beginNormalize()
{
	if(getDeviceState() == kDeviceHasIsochSync)
	{
		mTracker.beginCalibrate();
	}
}

void SoundplaneModel::cancelNormalize()
{
	if(getDeviceState() == kDeviceHasIsochSync)
	{
		mTracker.cancelCalibrate();
	}
}

bool SoundplaneModel::trackerIsCalibrating()
{
	int r = 0;
	if(getDeviceState() == kDeviceHasIsochSync)
	{
		r = mTracker.isCalibrating();
	}
	return r;
}

bool SoundplaneModel::trackerIsCollectingMap()
{
	int r = 0;
	if(getDeviceState() == kDeviceHasIsochSync)
	{
		r = mTracker.isCollectingNormalizeMap();
	}
	return r;
}

void SoundplaneModel::setDefaultNormalize()
{
	if(getDeviceState() == kDeviceHasIsochSync)
	{
		mTracker.setDefaultNormalizeMap();
	}
}


SoundplaneMECOutput& SoundplaneModel::mecOutput() { return mMECOutput;}


// JSON utilities

std::string getJSONString(cJSON* pNode, const char* name)
{
    cJSON* pItem = cJSON_GetObjectItem(pNode, name);
    if(pItem)
    {
        if(pItem->type == cJSON_String)
        {
            return std::string(pItem->valuestring);
        }
    }
    return std::string("");
}

double getJSONDouble(cJSON* pNode, const char* name)
{
    cJSON* pItem = cJSON_GetObjectItem(pNode, name);
    if(pItem)
    {
        if(pItem->type == cJSON_Number)
        {
            return pItem->valuedouble;
        }
    }
    return 0.;
}

int getJSONInt(cJSON* pNode, const char* name)
{
    cJSON* pItem = cJSON_GetObjectItem(pNode, name);
    if(pItem)
    {
        if(pItem->type == cJSON_Number)
        {
            return pItem->valueint;
        }
    }
    return 0;
}
