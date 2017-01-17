/**
** This class handles the soundplane by using the low level touch tracking classes
** This is the really just a test
**/


#include <iostream>
#include <unistd.h>
#include <string.h>

#include <pthread.h>

#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>


#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>

#include <iomanip>
#include <string.h>

#include <SoundplaneDriver.h>
#include <SoundplaneModelA.h>
#include <MLSignal.h>
#include <MLVector.h>
#include <TouchTracker.h>
#include <Filters2D.h>

#include "mec.h"

#define OUT_ADDRESS "127.0.0.1"
#define OUT_PORT 9002
#define OUTPUT_BUFFER_SIZE 1024

#define TOUCH_THROTTLE 4

const int kMaxTouch = 8;

class TouchProcesssor : public SoundplaneDriverListener
{
public:
    TouchProcesssor() :

        mTracker(kSoundplaneWidth, kSoundplaneHeight),
        mSurface(kSoundplaneWidth, kSoundplaneHeight),
        mCalibration(kSoundplaneWidth, kSoundplaneHeight),
        mCalibrateMean(kSoundplaneWidth, kSoundplaneHeight),
        mCalibrateSum(kSoundplaneWidth, kSoundplaneHeight),

        mNotchFilter(kSoundplaneWidth, kSoundplaneHeight),
        mLopassFilter(kSoundplaneWidth, kSoundplaneHeight),
        mBoxFilter(kSoundplaneWidth, kSoundplaneHeight),

        transmitSocket( IpEndpointName( OUT_ADDRESS, OUT_PORT ) )
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


        // from SoundplaneModel
        // setup box filter.
        mBoxFilter.setSampleRate(kSoundplaneSampleRate);
        mBoxFilter.setN(7);

        // setup fixed notch
        mNotchFilter.setSampleRate(kSoundplaneSampleRate);
        mNotchFilter.setNotch(150., 0.707);

        // setup fixed lopass.
        mLopassFilter.setSampleRate(kSoundplaneSampleRate);
        mLopassFilter.setLopass(50, 0.707);


    }

    virtual void deviceStateChanged(SoundplaneDriver& driver, MLSoundplaneState s) override {
        LOG_0(std::cout << "Device state changed: " << s << std::endl;)
    }

    //start: SoundplaneModel
    const float kTouchScaleToModel = 20.f;
    Vec2 xyToKeyGrid(Vec2 xy)
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
    //end: SoundplaneModel

    void sendTouches(int frameCount) {
        const float zscale = 1.0;
        const float zcurve = 0.25;
        int count = 0;
        osc::OutboundPacketStream op( buffer, OUTPUT_BUFFER_SIZE );
        op << osc::BeginBundleImmediate;
        for (int i = 0; i < kMaxTouch; ++i)
        {
            bool active = mTouchFrame(ageColumn, i) > 0 ;

            if (     active != touchActive[i]
                     ||  (active && (frameCount % TOUCH_THROTTLE) == 0 )) {

                touchActive[i] = active;
                count++;

                float x = mTouchFrame(xColumn, i);
                float y = mTouchFrame(yColumn, i);
                float z = mTouchFrame(zColumn, i);
                Vec2 xy = xyToKeyGrid(Vec2(x, y));

                z *= zscale * kTouchScaleToModel;
                z = (1.f - zcurve) * z + zcurve * z * z * z;
                z = clamp(z, 0.f, 1.f);

                op << osc::BeginMessage( "/tb/touch" )
                   << i
                   << xy.x()
                   << xy.y()
                   << z
                   << osc::EndMessage;

                //<< mTouchFrame(noteColumn, i)
                //<< mTouchFrame(dzColumn, i);
                //<< mTouchFrame(ageColumn, i;
                //<< mTouchFrame(dtColumn, i);
                //<< mTouchFrame(reservedColumn, i);
            }
        }
        op << osc::EndBundle;
        if (count > 0)
        {
            transmitSocket.Send( op.Data(), op.Size() );
        }
    }


    void beginCalibrate()
    {
        for (int i = 0; i < kMaxTouch; i++) touchActive[i] = false;
        mTracker.clear();
        mCalibrateSum.clear();
        mCalibrateMean.clear();
        mCalibrateCount = 0;
        mCalibrating = true;
    }

    void calibrate(const MLSignal& frame)
    {
        mCalibrateSum.add(frame);
        mCalibrateCount++;
        if (mCalibrateCount == kSoundplaneCalibrateSize)
        {
            endCalibrate();
        }
    }

    void endCalibrate()
    {

        MLSignal calibrateSum(kSoundplaneWidth, kSoundplaneHeight);
        MLSignal calibrateStdDev(kSoundplaneWidth, kSoundplaneHeight);
        MLSignal dSum(kSoundplaneWidth, kSoundplaneHeight);
        MLSignal dMean(kSoundplaneWidth, kSoundplaneHeight);
        MLSignal mean(kSoundplaneWidth, kSoundplaneHeight);

        // get mean
        mean = mCalibrateSum;
        mean.scale(1.f / mCalibrateCount);
        mCalibrateMean = mean;
        mCalibrateMean.sigClamp(0.0001f, 2.f);

        mCalibrating = false;
        mHasCalibration = true;

        mCalibrateSum.clear();

        mBoxFilter.clear();
        mNotchFilter.clear();
        mLopassFilter.clear();
    }


    virtual void receivedFrame(SoundplaneDriver& driver, const float* data, int size) override {

        // adapted from SoundplaneModel::receiveFrame

        // read from driver's ring buffer to incoming surface

        if (!mHasCalibration)
        {
            if (!mCalibrating) {
                beginCalibrate();
            }

            memcpy(mCalibration.getBuffer(), data, size * sizeof(float));
            calibrate(mCalibration);

            // OLD : mTracker.setCalibration(mCalibration);
        }
        else {

            memcpy(mSurface.getBuffer(), data, size * sizeof(float));

            // scale incoming data
            float in, cmean, cout;
            float epsilon = 0.000001;
            if (mHasCalibration)
            {
                for (int j = 0; j < mSurface.getHeight(); ++j)
                {
                    for (int i = 0; i < mSurface.getWidth(); ++i)
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
            sendTouches(mFrameCounter);
            mFrameCounter = (mFrameCounter + 1) % 1000;
        }
    }

private:
    int mFrameCounter = 0;
    bool mHasCalibration = false;
    bool mCalibrating = false;
    int  mCalibrateCount = 0;

    MLSignal mTouchFrame;
    TouchTracker mTracker;
    MLSignal mSurface;
    MLSignal mCalibration;
    MLSignal mCalibrateMean;
    MLSignal mCalibrateSum;

    Biquad2D mNotchFilter;
    Biquad2D mLopassFilter;
    BoxFilter2D mBoxFilter;

    UdpTransmitSocket transmitSocket;
    char buffer[OUTPUT_BUFFER_SIZE];
    bool touchActive[kMaxTouch];
};

void *soundplane_tt_proc(void *)
{
    pthread_mutex_lock(&waitMtx);
    LOG_0(std::cout  << "soundplane_tt_proc start" << std::endl;)
    TouchProcesssor listener;
    auto pDriver = SoundplaneDriver::create(&listener);
    int searchCount = 5;

    while (pDriver->getDeviceState() == 0 && searchCount > 0)
    {
        sleep(1);
        searchCount--;
    }

    if (searchCount > 0) {
        while (keepRunning) {
            pthread_cond_wait(&waitCond, &waitMtx);
        }
    }
    delete pDriver.release();

    LOG_0(std::cout  << "soundplane_tt_proc stop" << std::endl;)
    pthread_mutex_unlock(&waitMtx);
    pthread_exit(NULL);
}

