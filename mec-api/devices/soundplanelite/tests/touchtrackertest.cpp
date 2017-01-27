#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <signal.h>

#include <iomanip>
#include <string.h>

#include <SoundplaneDriver.h>
#include "SoundplaneModelA.h"
#include "MLSignal.h"
#include "TouchTracker.h"

namespace {

const int kMaxTouch = 8;
class TouchTrackerTest : public SoundplaneDriverListener
{
public:
    TouchTrackerTest() : 
        mTracker(kSoundplaneWidth,kSoundplaneHeight),
        mSurface(kSoundplaneWidth, kSoundplaneHeight),
        mCalibration(kSoundplaneWidth, kSoundplaneHeight),
        mTest(kSoundplaneWidth, kSoundplaneHeight)
    {
        mTracker.setSampleRate(kSoundplaneSampleRate);
        mTouchFrame.setDims(kTouchWidth, kSoundplaneMaxTouches);
        mTracker.setMaxTouches(kMaxTouch);
        mTracker.setLopass(100);
        //mTracker.setThresh(0.005000);
        mTracker.setThresh(0.005000);
        mTracker.setZScale(0.700000);
        mTracker.setForceCurve(0.250000);
        mTracker.setTemplateThresh(0.279104);
        mTracker.setBackgroundFilter(0.050000);
        mTracker.setQuantize(1);
        mTracker.setRotate(0);
        //mTracker.setNormalizeMap(sig)
    }

    virtual void deviceStateChanged(SoundplaneDriver& driver, MLSoundplaneState s) override {
        std::cout << "Device state changed: " << s << std::endl;
    }
    void dumpTouches() {
        std::cout << std::fixed << std::setw(6) << std::setprecision(4);
        for(int i=0; i<kMaxTouch; ++i)
        {
            std::cout << " x:" << mTouchFrame(xColumn, i);
            std::cout << " y:" << mTouchFrame(yColumn, i);
            std::cout << " z:" << mTouchFrame(zColumn, i);
            //cout << mTouchFrame(dzColumn, i);
            //cout << mTouchFrame(ageColumn, i;
            //cout << mTouchFrame(dtColumn, i);
            std::cout << " n:" << mTouchFrame(noteColumn, i);
            //cout << mTouchFrame(reservedColumn, i);
            std::cout << std::endl;
        }
    }

    virtual void receivedFrame(SoundplaneDriver& driver, const float* data, int size) override {
        if (!mHasCalibration)
        {
            memcpy(mCalibration.getBuffer(), data, sizeof(float) * size);
            mHasCalibration = true;
            mTracker.setCalibration(mCalibration);
            std::cout << "calibration\n";
            mCalibration.dumpASCII(std::cout);
            std::cout << "============\n";

//            mTracker.setDefaultNormalizeMap(); // ?
        }
        else 
        {
            memcpy(mSurface.getBuffer(), data, sizeof(float) * size);


            mTest = mSurface;
            mTest.subtract(mCalibration);
            mTracker.setInputSignal(&mTest);
            mTracker.setOutputSignal(&mTouchFrame);
            mTracker.process(1);

            if (mFrameCounter == 0) {
                mTest.scale(100.f);
                mTest.flipVertical();

                std::cout << "\n";
                mTest.dumpASCII(std::cout);
                mTest.dump(std::cout);
                dumpTouches();
            }

        }

        mFrameCounter = (mFrameCounter + 1) % 1000;
    }

private:
    int mFrameCounter = 0;
    bool mHasCalibration = false;
    MLSignal mTest;
    MLSignal mTouchFrame;
    MLSignal mSurface;
    MLSignal mCalibration;
    TouchTracker mTracker;
};

} // namespace

static volatile int keepRunning = 1;
void intHandler(int dummy) {
    std::cerr  << "int handler called";
    if(keepRunning==0) {
        sleep(1);
        exit(-1);
    }
    keepRunning = 0;
}
        
int main(int argc, const char * argv[]) {
    signal(SIGINT, intHandler);

    TouchTrackerTest listener;
    auto driver = SoundplaneDriver::create(&listener);

    std::cout << "TouchTrackerTest\n";
    std::cout << "Initial device state: " << driver->getDeviceState() << std::endl;

    while(keepRunning) {
        sleep(1);
    }
    delete driver.release();


    return 0;
}
