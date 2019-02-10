
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "SoundplaneModel.h"
#include "ThreadUtility.h"
#include "SensorFrame.h"
#include "MLProjectInfo.h"

const int kModelDefaultCarriersSize = 40;
const unsigned char kModelDefaultCarriers[kModelDefaultCarriersSize] =
{
	// 40 default carriers.  avoiding 32 (gets aliasing from 16)
	3, 4, 5, 6, 7,
	8, 9, 10, 11, 12,
	13, 14, 15, 16, 17,
	18, 19, 20, 21, 22,
	23, 24, 25, 26, 27,
	28, 29, 30, 31, 33,
	34, 35, 36, 37, 38,
	39, 40, 41, 42, 43
};


std::string makeDefaultServiceName()
{
	std::stringstream nameStream;
	nameStream << "default" << " (" << kDefaultUDPPort << ")";
	return nameStream.str();
}

// make one of the possible standard carrier sets, skipping a range of carriers out of the
// middle of the 40 defaults.
//
static const int kStandardCarrierSets = 16;
static void makeStandardCarrierSet(SoundplaneDriver::Carriers &carriers, int set)
{
	int startOffset = 2;
	int skipSize = 2;
	int gapSize = 4;
	int gapStart = set*skipSize + startOffset;
	carriers[0] = carriers[1] = 0;
	for(int i=startOffset; i<gapStart; ++i)
	{
		carriers[i] = kModelDefaultCarriers[i];
	}
	for(int i=gapStart; i<kSoundplaneNumCarriers; ++i)
	{
		carriers[i] = kModelDefaultCarriers[i + gapSize];
	}
}

void touchArrayToFrame(const TouchArray* pArray, MLSignal* pFrame)
{
	// get references for syntax
	const TouchArray& array = *pArray;
	MLSignal& frame = *pFrame;
	
	for(int i = 0; i < kMaxTouches; ++i)
	{
		Touch t = array[i];
		frame(xColumn, i) = t.x;
		frame(yColumn, i) = t.y;
		frame(zColumn, i) = t.z;
		frame(dzColumn, i) = t.dz;
		frame(ageColumn, i) = t.age;
	}
}

MLSignal sensorFrameToSignal(const SensorFrame &f)
{
	MLSignal out(SensorGeometry::width, SensorGeometry::height);
	for(int j = 0; j < SensorGeometry::height; ++j)
	{
		const float* srcStart = f.data() + SensorGeometry::width*j;
		const float* srcEnd = srcStart + SensorGeometry::width;
		std::copy(srcStart, srcEnd, out.getBuffer() + out.row(j));
	}
	return out;
}

SensorFrame signalToSensorFrame(const MLSignal& in)
{
	SensorFrame out;
	for(int j = 0; j < SensorGeometry::height; ++j)
	{
		const float* srcStart = in.getConstBuffer() + (j << in.getWidthBits());
		const float* srcEnd = srcStart + in.getWidth();
		std::copy(srcStart, srcEnd, out.data() + SensorGeometry::width*j);
	}
	return out;
}

// --------------------------------------------------------------------------------
//
#pragma mark SoundplaneModel

SoundplaneModel::SoundplaneModel() :
mOutputEnabled(false),
mSurface(SensorGeometry::width, SensorGeometry::height),
mRawSignal(SensorGeometry::width, SensorGeometry::height),
mCalibratedSignal(SensorGeometry::width, SensorGeometry::height),
mSmoothedSignal(SensorGeometry::width, SensorGeometry::height),
mCalibrating(false),
mTestTouchesOn(false),
mTestTouchesWasOn(false),
mSelectingCarriers(false),
mHasCalibration(false),
mZoneIndexMap(kSoundplaneAKeyWidth, kSoundplaneAKeyHeight),
mHistoryCtr(0),
mCarrierMaskDirty(false),
mNeedsCarriersSet(false),
mNeedsCalibrate(false),
mLastInfrequentTaskTime(0),
mCarriersMask(0xFFFFFFFF),
mDoOverrideCarriers(false),
mKymaIsConnected(0),
mKymaMode(false)
{
	mpDriver = SoundplaneDriver::create(*this);
	
	for(int i=0; i<kMaxTouches; ++i)
	{
		mCurrentKeyX[i] = -1;
		mCurrentKeyY[i] = -1;
	}
	
	// setup default carriers in case there are no saved carriers
	for (int car=0; car<kSoundplaneNumCarriers; ++car)
	{
		mCarriers[car] = kModelDefaultCarriers[car];
	}
	
	clearZones();
	setAllPropertiesToDefaults();
	
	MLConsole() << "SoundplaneModel: listening for OSC on port " << kDefaultUDPReceivePort << "...\n";
	listenToOSC(kDefaultUDPReceivePort);
	
	// mMIDIOutput.initialize();
	mMECOutput.initialize();
	
	mTouchFrame.setDims(kSoundplaneTouchWidth, kMaxTouches);
	mTouchHistory.setDims(kSoundplaneTouchWidth, kMaxTouches, kSoundplaneHistorySize);
	
	// make zone presets collection
	File zoneDir = getDefaultFileLocation(kPresetFiles, MLProjectInfo::makerName, MLProjectInfo::projectName).getChildFile("ZonePresets");
	debug() << "LOOKING for zones in " << zoneDir.getFileName() << "\n";
	mZonePresets = std::unique_ptr<MLFileCollection>(new MLFileCollection("zone_preset", zoneDir, "json"));
	mZonePresets->processFilesImmediate();
	//mZonePresets->dump();
	
	// now that the driver is active, start polling for changes in properties
	mTerminating = false;
	
	startModelTimer();
	
	mSensorFrameQueue = std::unique_ptr< Queue<SensorFrame> >(new Queue<SensorFrame>(kSensorFrameQueueSize));
	
	mProcessThread = std::thread(&SoundplaneModel::processThread, this);
	SetPriorityRealtimeAudio(mProcessThread.native_handle());
	
	mpDriver->start();
}

SoundplaneModel::~SoundplaneModel()
{
	// signal threads to shut down
	mTerminating = true;
	
	if (mProcessThread.joinable())
	{
		mProcessThread.join();
		printf("SoundplaneModel: mProcessThread terminated.\n");
	}
	
	listenToOSC(0);
	
	mpDriver = nullptr;
}



void SoundplaneModel::doPropertyChangeAction(ml::Symbol p, const MLProperty & newVal)
{
	//  debug() << "SoundplaneModel::doPropertyChangeAction: " << p << " -> " << newVal << "\n";
	
	int propertyType = newVal.getType();
	switch(propertyType)
	{
		case MLProperty::kFloatProperty:
		{
			float v = newVal.getFloatValue();
			if (ml::textUtils::stripFinalNumber(p) == ml::Symbol("carrier_toggle"))
			{
				// toggles changed -- mute carriers
				unsigned long mask = 0;
				for(int i=0; i<32; ++i)
				{
					ml::Symbol tSym = ml::textUtils::addFinalNumber(ml::Symbol("carrier_toggle"), i);
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
					ml::Symbol tSym = ml::textUtils::addFinalNumber(ml::Symbol("carrier_toggle"), i);
					setProperty(tSym, on);
				}
				mCarriersMask = on ? ~0 : 0;
				mCarrierMaskDirty = true; // trigger carriers set in a second or so
			}
			else if (p == "max_touches")
			{
				mMaxTouches = v;
				// mMIDIOutput.setMaxTouches(v);
				// mOSCOutput.setMaxTouches(v);
			}
			else if (p == "lopass_z")
			{
				mTracker.setLopassZ(v);
			}
			else if (p == "z_thresh")
			{
				mTracker.setThresh(v);
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
			else if (p == "data_rate")
			{
				mDataRate = v;
				// mOSCOutput.setDataRate(v);
				// mMIDIOutput.setDataRate(v);
			}
			else if (p == "midi_active")
			{
				// mMIDIOutput.setActive(bool(v));
			}
			else if (p == "midi_mpe")
			{
				// mMIDIOutput.setMPE(bool(v));
			}
			else if (p == "midi_mpe_extended")
			{
				// mMIDIOutput.setMPEExtended(bool(v));
			}
			else if (p == "midi_channel")
			{
				// mMIDIOutput.setStartChannel(int(v));
			}
			else if (p == "midi_pressure_active")
			{
				// mMIDIOutput.setPressureActive(bool(v));
			}
			else if (p == "osc_active")
			{
				bool b = v;
				// mOSCOutput.setActive(b);
			}
			else if (p == "osc_send_matrix")
			{
				bool b = v;
				mSendMatrixData = b;
			}
			else if (p == "quantize")
			{
				sendParametersToZones();
			}
			else if (p == "rotate")
			{
				bool b = v;
				mTracker.setRotate(b);
			}
			else if (p == "glissando")
			{
				// mMIDIOutput.setGlissando(bool(v));
				sendParametersToZones();
			}
			else if (p == "hysteresis")
			{
				mMIDIOutput.setHysteresis(v);
				sendParametersToZones();
			}
			else if (p == "transpose")
			{
				sendParametersToZones();
			}
			else if (p == "bend_range")
			{
				mMIDIOutput.setBendRange(v);
				sendParametersToZones();
			}
			else if (p == "verbose")
			{
				bool b = v;
				mVerbose = b;
			}
			else if (p == "override_carriers")
			{
				bool b = v;
				mDoOverrideCarriers = b;
				mNeedsCarriersSet = true;
			}
			else if (p == "override_carrier_set")
			{
				makeStandardCarrierSet(mOverrideCarriers, v);
				mNeedsCarriersSet = true;
			}
			else if (p == "test_touches")
			{
				bool b = v;
				MLConsole() << "test touches: " << b << "\n";
				mTestTouchesOn = b;
				mSensorFrameQueue->clear();
				mRequireSendNextFrame = true;
			}
		}
		break;
		case MLProperty::kTextProperty:
		{
			// TODO clean up, use text for everything
			ml::Text strText = newVal.getTextValue();
			std::string str (strText.getText()); 
			
			if(p == "osc_service_name")
			{
				// mOSCOutput.clear();
				
				// // we only save the formatted service name.
				// std::string serviceName = unformatServiceName(str);
				// std::string hostName = MLNetServiceHub::getHostName(serviceName);

				// if(hostName == "")
				// {
				// 	// wait a bit for service to be resolved on startup
				// 	time_point<system_clock> waitStartTime = system_clock::now();
				// 	bool stopWaiting = false;
				// 	while(hostName == "" && !stopWaiting)
				// 	{
				// 		std::this_thread::sleep_for(std::chrono::milliseconds(500));
				// 		time_point<system_clock> now = system_clock::now();
				// 		int waitDuration = duration_cast<milliseconds>(now - waitStartTime).count();
				// 		if(waitDuration > 2000) stopWaiting = true;
				// 		hostName = MLNetServiceHub::getHostName(serviceName);
				// 	}
				// }
				
				// if(hostName == "")
				// {
				// 	std::cout << "OSC service not resolved: falling back to default\n";
				// 	hostName = "localhost";
				// 	serviceName = "default";
					
				// 	std::string defaultService = makeDefaultServiceName();
				// 	setProperty("osc_service_name", defaultService.c_str());
				// }
				
				// int port = MLNetServiceHub::getPort(serviceName);
				// mOSCOutput.setHostName(hostName);
				// mOSCOutput.setPort(port);
				// mOSCOutput.reconnect();
			}
			if (p == "viewmode")
			{
				// nothing to do for Model
			}
			else if (p == "midi_device")
			{
				mMIDIOutput.setDevice(str);
			}
			else if (p == "zone_JSON")
			{
				loadZonesFromString(str);
			}
			else if (p == "zone_preset")
			{
				// look for built in zone map names.
				if(str == "chromatic")
				{
					setProperty("zone_JSON", (SoundplaneBinaryData::chromatic_json));
				}
				else if(str == "rows in fourths")
				{
					setProperty("zone_JSON", (SoundplaneBinaryData::rows_in_fourths_json));
				}
				else if(str == "rows in octaves")
				{
					setProperty("zone_JSON", (SoundplaneBinaryData::rows_in_octaves_json));
				}
				// if not built in, load a zone map file.
				else
				{
					const MLFile& f = mZonePresets->getFileByPath(str);
					if(f.exists())
					{
						File zoneFile = f.getJuceFile();
						String stateStr(zoneFile.loadFileAsString());
						setPropertyImmediate("zone_JSON", (stateStr.toUTF8()));
					}
				}
			}
		}
			break;
		case MLProperty::kSignalProperty:
		{
			const MLSignal& sig = newVal.getSignalValue();
			if(p == ml::Symbol("carriers"))
			{
				// get carriers from signal
				assert(sig.getSize() == kSoundplaneNumCarriers);
				for(int i=0; i<kSoundplaneNumCarriers; ++i)
				{
					if(mCarriers[i] != sig[i])
					{
						mCarriers[i] = sig[i];
						mNeedsCarriersSet = true;
					}
				}
			}
		}
			break;
		default:
			break;
	}
}


void SoundplaneModel::onStartup()
{
	// get serial number and auto calibrate noise on sync detect
	const unsigned long instrumentModel = 1; // Soundplane A
	// mOSCOutput.setSerialNumber((instrumentModel << 16) | mpDriver->getSerialNumber());
	
	// connected but not calibrated -- disable output.
	enableOutput(false);
	// output will be enabled at end of calibration.
	mNeedsCalibrate = true;
}

// we need to return as quickly as possible from driver callback.
// just put the new frame in the queue.
void SoundplaneModel::onFrame(const SensorFrame& frame)
{
	if(!mTestTouchesOn)
	{
		mSensorFrameQueue->push(frame);
	}
}

void SoundplaneModel::onError(int error, const char* errStr)
{
	switch(error)
	{
		case kDevDataDiffTooLarge:
			MLConsole() << "error: frame difference too large: " << errStr << "\n";
			beginCalibrate();
			break;
		case kDevGapInSequence:
			if(mVerbose)
			{
				MLConsole() << "note: gap in sequence " << errStr << "\n";
			}
			break;
		case kDevReset:
			if(mVerbose)
			{
				MLConsole() << "isoch stalled, resetting " << errStr << "\n";
			}
			break;
		case kDevPayloadFailed:
			if(mVerbose)
			{
				MLConsole() << "payload failed at sequence " << errStr << "\n";
			}
			break;
	}
}

void SoundplaneModel::onClose()
{
	enableOutput(false);
}

void SoundplaneModel::processThread()
{
	time_point<system_clock> previous, now;
	previous = now = system_clock::now();
	mPrevProcessTouchesTime = now; // TODO interval timer object
	
	while(!mTerminating)
	{
		now = system_clock::now();
		process(now);
		mProcessCounter++;
		
		size_t queueSize = mSensorFrameQueue->elementsAvailable();
		if(queueSize > mMaxRecentQueueSize)
		{
			mMaxRecentQueueSize = queueSize;
		}
		
		if(mProcessCounter >= 1000)
		{
			if(mVerbose)
			{
				if(mMaxRecentQueueSize >= kSensorFrameQueueSize)
				{
					MLConsole() << "warning: input queue full \n";
				}
			}
			
			mProcessCounter = 0;
			mMaxRecentQueueSize = 0;
		}
		
		// sleep, less than one frame interval
		std::this_thread::sleep_for(std::chrono::microseconds(500));
		
		// do infrequent tasks every second
		int secondsInterval = duration_cast<seconds>(now - previous).count();
		if (secondsInterval >= 1)
		{
			previous = now;
			doInfrequentTasks();
		}
	}
}


void SoundplaneModel::process(time_point<system_clock> now)
{
	static int tc = 0;
	tc++;
	
	TouchArray touches{};
	if(mTestTouchesOn || mTestTouchesWasOn)
	{
		touches = getTestTouchesFromTracker(now);
		mTestTouchesWasOn = mTestTouchesOn;
		outputTouches(touches, now);
	}
	else
	{
		if(mSensorFrameQueue->pop(mSensorFrame))
		{
			mSurface = sensorFrameToSignal(mSensorFrame);
			
			// store surface for raw output
			{
				std::lock_guard<std::mutex> lock(mRawSignalMutex);
				mRawSignal.copy(mSurface);
			}
			
			if(mCalibrating)
			{
				mStats.accumulate(mSensorFrame);
				if (mStats.getCount() >= kSoundplaneCalibrateSize)
				{
					endCalibrate();
				}
			}
			else if (mSelectingCarriers)
			{
				mStats.accumulate(mSensorFrame);
				
				if (mStats.getCount() >= kSoundplaneCalibrateSize)
				{
					nextSelectCarriersStep();
				}
			}
			else if(mOutputEnabled)
			{
				if (mHasCalibration)
				{
					mCalibratedFrame = subtract(multiply(mSensorFrame, mCalibrateMeanInv), 1.0f);
					
					TouchArray touches = trackTouches(mCalibratedFrame);
					outputTouches(touches, now);
				}
			}
		}
	}
}

void SoundplaneModel::outputTouches(TouchArray touches, time_point<system_clock> now)
{
	saveTouchHistory(touches);
	
	// let Zones process touches. This is always done at the controller's frame rate.
	sendTouchesToZones(touches);
	
	// determine if incoming frame could start or end a touch
	bool notesChangedThisFrame = findNoteChanges(touches, mTouchArray1);
	mTouchArray1 = touches;
	
	const int dataPeriodMicrosecs = 1000*1000 / mDataRate;
	int microsSinceSend = duration_cast<microseconds>(now - mPrevProcessTouchesTime).count();
	bool timeForNewFrame = (microsSinceSend >= dataPeriodMicrosecs);
	if(notesChangedThisFrame || timeForNewFrame || mRequireSendNextFrame)
	{
		mRequireSendNextFrame = false;
		mPrevProcessTouchesTime = now;
		sendFrameToOutputs(now);
	}
}

// send raw touches to zones in order to generate touch and controller states within the Zones.
//
void SoundplaneModel::sendTouchesToZones(TouchArray touches)
{
	// const int maxTouches = getFloatProperty("max_touches");
	const float hysteresis = getFloatProperty("hysteresis");
	
	// clear incoming touches and push touch history in each zone
	for(auto& zone : mZones)
	{
		zone.newFrame();
	}
	
	// add any active touches to the Zones they are over
	// MLTEST for(int i=0; i<maxTouches; ++i)
	
	// iterate on all possible touches so touches will turn off when max_touches is lowered
	for(int i=0; i<kMaxTouches; ++i)
	{
		float x = touches[i].x;
		float y = touches[i].y;
		
		if(touchIsActive(touches[i]))
		{
			//std::cout << i << ":" << age << "\n";
			// get fractional key grid position (Soundplane A)
			Vec2 keyXY (x, y);
			
			// get integer key
			int ix = (int)x;
			int iy = (int)y;
			
			// apply hysteresis to raw position to get current key
			// hysteresis: make it harder to move out of current key
			if(touches[i].state == kTouchStateOn)
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
			
			// send index, xyz, dz to zone
			int zoneIdx = mZoneIndexMap(mCurrentKeyX[i], mCurrentKeyY[i]);
			if(zoneIdx >= 0)
			{
				Touch t = touches[i];
				t.kx = mCurrentKeyX[i];
				t.ky = mCurrentKeyY[i];
				mZones[zoneIdx].addTouchToFrame(i, t);
			}
		}
	}
	
	for(auto& zone : mZones)
	{
		zone.storeAnyNewTouches();
	}
	
	std::bitset<kMaxTouches> freedTouches;
	
	// process note offs for each zone
	// this happens before processTouches() to allow touches to be freed for reuse in this frame
	for(auto& zone : mZones)
	{
		zone.processTouchesNoteOffs(freedTouches);
	}
	
	// process touches for each zone
	for(auto& zone : mZones)
	{
		zone.processTouches(freedTouches);
	}
}

void SoundplaneModel::dumpOutputsByZone()
{
	// count touches in zones
	int activeTouches = 0;
	for(auto& zone : mZones)
	{
		// touches
		for(int i=0; i<kMaxTouches; ++i)
		{
			Touch t = zone.mOutputTouches[i];
			if(touchIsActive(t))
			{
				activeTouches++;
			}
		}
	}
	
	if(activeTouches)
	{
		int zc = 0;
		
		// send messages to outputs about each zone
		for(auto& zone : mZones)
		{
			
			std::cout << "[zone " << zc++ << ": ";
			
			// touches
			for(int i=0; i<kMaxTouches; ++i)
			{
				Touch t = zone.mOutputTouches[i];
				if(touchIsActive(t))
				{
					std::cout << i << ":" << t.state << ":" << t.z << " ";
				}
			}

			std::cout << "]";
		}
		std::cout << "\n";
	}
	
}

void SoundplaneModel::sendFrameToOutputs(time_point<system_clock> now)
{
	beginOutputFrame(now);
	
	// send messages to outputs about each zone
	for(auto& zone : mZones)
	{
		// touches
		for(int i=0; i<kMaxTouches; ++i)
		{
			Touch t = zone.mOutputTouches[i];
			if(touchIsActive(t))
			{
				sendTouchToOutputs(i, zone.mOffset, t);
			}
		}
		
		// controller
		if(zone.mOutputController.active)
		{
			sendControllerToOutputs(zone.mZoneID, zone.mOffset, zone.mOutputController);
		}
	}
	
	// send optional calibrated matrix to OSC output
	if(mSendMatrixData)
	{
		MLSignal calibratedPressure = getCalibratedSignal();
		if(calibratedPressure.getHeight() == SensorGeometry::height)
		{
			// send to OSC output only
			// mOSCOutput.processMatrix(calibratedPressure);
		}
	}
	
	endOutputFrame();
}

void SoundplaneModel::beginOutputFrame(time_point<system_clock> now)
{
	mMECOutput.beginOutputFrame(now);
	// if(mMIDIOutput.isActive())
	// {
	// 	mMIDIOutput.beginOutputFrame(now);
	// }
	// if(mOSCOutput.isActive())
	// {
	// 	mOSCOutput.beginOutputFrame(now);
	// }
}

void SoundplaneModel::sendTouchToOutputs(int i, int offset, const Touch& t)
{
	mMECOutput.processTouch(i, offset, t);
	// if(mMIDIOutput.isActive())
	// {
	// 	mMIDIOutput.processTouch(i, offset, t);
	// }
	// if(mOSCOutput.isActive())
	// {
	// 	mOSCOutput.processTouch(i, offset, t);
	// }
}

void SoundplaneModel::sendControllerToOutputs(int zoneID, int offset, const Controller& m)
{
	mMECOutput.processController(zoneID, offset, m);
	// if(mMIDIOutput.isActive())
	// {
	// 	mMIDIOutput.processController(zoneID, offset, m);
	// }
	// if(mOSCOutput.isActive())
	// {
	// 	mOSCOutput.processController(zoneID, offset, m);
	// }
}

void SoundplaneModel::endOutputFrame()
{
	mMECOutput.endOutputFrame();
	// if(mMIDIOutput.isActive())
	// {
	// 	mMIDIOutput.endOutputFrame();
	// }
	// if(mOSCOutput.isActive())
	// {
	// 	mOSCOutput.endOutputFrame();
	// }
}

void SoundplaneModel::setAllPropertiesToDefaults()
{
	// parameter defaults and creation
	setProperty("max_touches", 4);
	setProperty("lopass_z", 100.);
	
	setProperty("z_thresh", 0.05);
	setProperty("z_scale", 1.);
	setProperty("z_curve", 0.5);
	setProperty("display_scale", 1.);
	
	setProperty("pairs", 0.);
	setProperty("quantize", 1.);
	setProperty("lock", 0.);
	setProperty("abs_rel", 0.);
	setProperty("snap", 250.);
	setProperty("vibrato", 0.5);
	
	setProperty("midi_active", 0);
	setProperty("midi_mpe", 1);
	setProperty("midi_mpe_extended", 0);
	setProperty("midi_channel", 1);
	
	setProperty("data_rate", 250.);
	
	setProperty("kyma_poll", 0);
	
	{
		std::string defaultService = makeDefaultServiceName();
		setProperty("osc_service_name", defaultService.c_str());
	}

	setProperty("osc_active", 1);
	setProperty("osc_raw", 0);
	
	setProperty("bend_range", 48);
	setProperty("transpose", 0);
	setProperty("bg_filter", 0.05);
	
	setProperty("hysteresis", 0.5);
	setProperty("lo_thresh", 0.1);
	
	// menu param defaults
	setProperty("viewmode", "calibrated");
	
	// preset menu defaults (TODO use first choices?)
	setProperty("zone_preset", "rows in fourths");
	setProperty("touch_preset", "touch default");
	
	setProperty("view_page", 0);
	
	for(int i=0; i<32; ++i)
	{
		setProperty(ml::textUtils::addFinalNumber("carrier_toggle", i), 1);
	}
}

// Process incoming OSC.  Used for Kyma communication.
//
// void SoundplaneModel::ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint)
// {
// 	// MLTEST - kyma debugging
// 	char endpointStr[256];
// 	remoteEndpoint.AddressAndPortAsString(endpointStr);
// 	MLConsole() << "OSC: " << m.AddressPattern() << " from " << endpointStr << "\n";
	
// 	osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
// 	osc::int32 a1;
// 	try
// 	{
// 		if( std::strcmp( m.AddressPattern(), "/osc/response_from" ) == 0 )
// 		{
// 			args >> a1 >> osc::EndMessage;
			
// 			// MLTEST
// 			MLConsole() << " arg = " << a1 << "\n";
			
// 			mKymaIsConnected = true;
// 		}
// 		else if (std::strcmp( m.AddressPattern(), "/osc/notify/midi/Soundplane" ) == 0 )
// 		{
// 			args >> a1 >> osc::EndMessage;
			
// 			// MLTEST
// 			MLConsole() << " arg = " << a1 << "\n";
			
// 			// set voice count to a1
// 			int newTouches = ml::clamp((int)a1, 0, kMaxTouches);
			
// 			// Kyma is sending 0 sometimes, which there is probably
// 			// no reason to respond to
// 			if(newTouches > 0)
// 			{
// 				setProperty("max_touches", newTouches);
// 			}
// 		}
// 	}
// 	catch( osc::Exception& e )
// 	{
// 		MLConsole() << "oscpack error while parsing message: "
// 		<< m.AddressPattern() << ": " << e.what() << "\n";
// 	}
// }

// void SoundplaneModel::ProcessBundle(const osc::ReceivedBundle &b, const IpEndpointName& remoteEndpoint)
// {
	
// }

// const std::vector<std::string>& SoundplaneModel::getServicesList()
// {
// 	return MLNetServiceHub::getFormattedServiceNames();
// }

void SoundplaneModel::initialize()
{
}

int SoundplaneModel::getClientState(void)
{
	return mKymaIsConnected;
}

int SoundplaneModel::getDeviceState(void)
{
	if(!mpDriver.get())
	{
		return kNoDevice;
	}
	
	return mpDriver->getDeviceState();
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
		case kDeviceUnplugged:
			snprintf(mHardwareStr, kMiscStringSize, "no device");
			break;
		case kDeviceConnected:
		case kDeviceHasIsochSync:
			serial_number = mpDriver->getSerialNumberString();
			v = mpDriver->getFirmwareVersion();
			a = v >> 8 & 0x0F;
			b = v >> 4 & 0x0F,
			c = v & 0x0F;
			snprintf(mHardwareStr, kMiscStringSize, "%s #%s, firmware %d.%d.%d", kSoundplaneAName, serial_number.c_str(), a, b, c);
			break;
		default:
			snprintf(mHardwareStr, kMiscStringSize, "?");
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
			snprintf(mStatusStr, kMiscStringSize, "waiting for Soundplane...");
			break;
			
		case kDeviceConnected:
			snprintf(mStatusStr, kMiscStringSize, "waiting for isochronous data...");
			break;
			
		case kDeviceHasIsochSync:
			snprintf(mStatusStr, kMiscStringSize, "synchronized");
			break;
			
		case kDeviceUnplugged:
			snprintf(mStatusStr, kMiscStringSize, "unplugged");
			break;
			
		default:
			snprintf(mStatusStr, kMiscStringSize, "unknown status");
			break;
	}
	return mStatusStr;
}

// get the string to report a specific client connection above and beyond the usual
// OSC / MIDI communication.
const char* SoundplaneModel::getClientStr()
{
	switch(mKymaIsConnected)
	{
		case 0:
			snprintf(mClientStr, kMiscStringSize, "");
			break;
			
		case 1:
			snprintf(mClientStr, kMiscStringSize, "connected to Kyma");
			break;
			
		default:
			snprintf(mClientStr, kMiscStringSize, "?");
			break;
	}
	return mClientStr;
}

// remove all zones from the zone list.
void SoundplaneModel::clearZones()
{
	mZones.clear();
	mZoneIndexMap.fill(-1);
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
			mZones.emplace_back(Zone());
			Zone* pz = &mZones.back();
			
			cJSON* pZoneType = cJSON_GetObjectItem(pNode, "type");
			if(pZoneType)
			{
				// get zone type and type specific attributes
				ml::Symbol typeSym(pZoneType->valuestring);
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
			
			// get zone rect in keys
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
			
			pz->mName = TextFragment(getJSONString(pNode, "name"));
			pz->mStartNote = getJSONInt(pNode, "note");
			pz->mOffset = getJSONInt(pNode, "offset");
			pz->mControllerNum1 = getJSONInt(pNode, "ctrl1");
			pz->mControllerNum2 = getJSONInt(pNode, "ctrl2");
			pz->mControllerNum3 = getJSONInt(pNode, "ctrl3");
			
			int zoneIdx = mZones.size() - 1;
			if(zoneIdx < kSoundplaneAMaxZones)
			{
				pz->setZoneID(zoneIdx);
				
				MLRect b(pz->getBounds());
				int x = b.x();
				int y = b.y();
				int w = b.width();
				int h = b.height();
				
				for(int j=y; j < y + h; ++j)
				{
					for(int i=x; i < x + w; ++i)
					{
						mZoneIndexMap(i, j) = zoneIdx;
					}
				}
			}
			else
			{
				MLConsole() << "SoundplaneModel::loadZonesFromString: out of zones!\n";
			}
		}
		pNode = pNode->next;
	}
	sendParametersToZones();
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
		mZones[i].mVibrato = v;
		mZones[i].mHysteresis = h;
		mZones[i].mQuantize = q;
		mZones[i].mNoteLock = nl;
		mZones[i].mTranspose = t;
		mZones[i].setSnapFreq(sf);
	}
}

// c over [0 - 1] fades response from sqrt(x) -> x -> x^2
//
float responseCurve(float x, float c)
{
	float y;
	if(c < 0.5f)
	{
		y = lerp(x*x, x, c*2.f);
	}
	else
	{
		y = lerp(x, sqrtf(x), c*2.f - 1.f);
	}
	return y;
}

bool SoundplaneModel::findNoteChanges(TouchArray t0, TouchArray t1)
{
	bool anyChanges = false;
	
	for(int i=0; i<kMaxTouches; ++i)
	{
		if((t0[i].state) != (t1[i].state))
		{
			anyChanges = true;
			break;
		}
	}
	
	return anyChanges;
}

TouchArray SoundplaneModel::scaleTouchPressureData(TouchArray in)
{
	TouchArray out = in;
	
	const float zscale = getFloatProperty("z_scale");
	const float zcurve = getFloatProperty("z_curve");
	const float dzScale = 0.125f;
	
	for(int i=0; i<kMaxTouches; ++i)
	{
		float z = in[i].z;
		z *= zscale;
		z = ml::clamp(z, 0.f, 4.f);
		z = responseCurve(z, zcurve);
		out[i].z = z;
		
		// for note-ons, use same z scale controls as pressure
		float dz = in[i].dz*dzScale;
		dz *= zscale;
		dz = ml::clamp(dz, 0.f, 1.f);
		dz = responseCurve(dz, zcurve);
		out[i].dz = dz;
	}
	return out;
}

TouchArray SoundplaneModel::trackTouches(const SensorFrame& frame)
{
	SensorFrame curvature = mTracker.preprocess(frame);
	TouchArray t = mTracker.process(curvature, mMaxTouches);
	mSmoothedSignal = sensorFrameToSignal(curvature);
	t = scaleTouchPressureData(t);
	return t;
}

TouchArray SoundplaneModel::getTestTouchesFromTracker(time_point<system_clock> now)
{
	TouchArray t = mTracker.getTestTouches(now, mMaxTouches);
	t = scaleTouchPressureData(t);
	return t;
}

void SoundplaneModel::saveTouchHistory(const TouchArray& t)
{
	// convert array of touches to Signal for display, history
	{
		std::lock_guard<std::mutex> lock(mTouchFrameMutex);
		touchArrayToFrame(&t, &mTouchFrame);
	}
	
	mHistoryCtr++;
	if (mHistoryCtr >= kSoundplaneHistorySize) mHistoryCtr = 0;
	mTouchHistory.setFrame(mHistoryCtr, mTouchFrame);
}

void SoundplaneModel::doInfrequentTasks()
{
	// MLNetServiceHub::PollNetServices();
	// mOSCOutput.doInfrequentTasks();
	// mMIDIOutput.doInfrequentTasks();
	mMECOutput.doInfrequentTasks();

	if(getDeviceState() == kDeviceHasIsochSync)
	{
		
		if(mCarrierMaskDirty)
		{
			enableCarriers(mCarriersMask);
		}
		else if (mNeedsCarriersSet)
		{
			mNeedsCarriersSet = false;
			if(mDoOverrideCarriers)
			{
				setCarriers(mOverrideCarriers);
			}
			else
			{
				setCarriers(mCarriers);
			}
			
			mNeedsCalibrate = true;
		}
		else if (mNeedsCalibrate && (!mSelectingCarriers))
		{
			mNeedsCalibrate = false;
			beginCalibrate();
		}
	}
}

void SoundplaneModel::setDefaultCarriers()
{
	MLSignal cSig(kSoundplaneNumCarriers);
	for (int car=0; car<kSoundplaneNumCarriers; ++car)
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

void SoundplaneModel::dumpCarriers(const SoundplaneDriver::Carriers& carriers)
{
	debug() << "\n------------------\n";
	debug() << "carriers: \n";
	for(int i=0; i<kSoundplaneNumCarriers; ++i)
	{
		int c = carriers[i];
		debug() << i << ": " << c << " ["<< carrierToFrequency(c) << "Hz] \n";
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
// calculating the mean rest value for each taxel.
//
void SoundplaneModel::beginCalibrate()
{
	if(getDeviceState() == kDeviceHasIsochSync)
	{
		mStats.clear();
		mCalibrating = true;
	}
}

// called by process routine when enough samples have been collected.
//
void SoundplaneModel::endCalibrate()
{
	SensorFrame mean = clamp(mStats.mean(), 0.0001f, 1.f);
	mCalibrateMeanInv = divide(fill(1.f), mean);
	mCalibrating = false;
	mHasCalibration = true;
	enableOutput(true);
}

float SoundplaneModel::getCalibrateProgress()
{
	return mStats.getCount() / (float)kSoundplaneCalibrateSize;
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
		mStats.clear();
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
	// analyze calibration data just collected.
	SensorFrame mean = clamp(mStats.mean(), 0.0001f, 1.f);
	SensorFrame stdDev = mStats.standardDeviation();
	SensorFrame variation = divide(stdDev, mean);
	
	// find maximum noise in any column for this set.  This is the "badness" value
	// we use to compare carrier sets.
	float maxVar = 0;
	float maxVarFreq = 0;
	float variationSum;
	int startSkip = 2;
	
	for(int col = startSkip; col<kSoundplaneNumCarriers; ++col)
	{
		variationSum = 0;
		int carrier = mCarriers[col];
		float cFreq = carrierToFrequency(carrier);
		
		variationSum = getColumnSum(variation, col);
		if(variationSum > maxVar)
		{
			maxVar = variationSum;
			maxVarFreq = cFreq;
		}
	}
	
	mMaxNoiseByCarrierSet[mSelectCarriersStep] = maxVar;
	mMaxNoiseFreqByCarrierSet[mSelectCarriersStep] = maxVarFreq;
	
	MLConsole() << "max noise for set " << mSelectCarriersStep << ": " << maxVar << "(" << maxVarFreq << " Hz) \n";
	
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
	
	// clear data
	mStats.clear();
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
	setCarriers(mCarriers);
	
	// set chosen carriers as model parameter so they will be saved
	// this will trigger a recalibrate
	MLSignal cSig(kSoundplaneNumCarriers);
	for (int car=0; car<kSoundplaneNumCarriers; ++car)
	{
		cSig[car] = mCarriers[car];
	}
	setProperty("carriers", cSig);
	MLConsole() << "carrier select done.\n";
	
	mSelectingCarriers = false;
	mNeedsCalibrate = true;
}

