#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <signal.h>

#include <iomanip>
#include <string.h>

#include <SoundplaneDriver.h>
#include "SoundplaneModelA.h"
//#include "MLSignal.h"

namespace
{

class HelloSoundplaneDriverListener : public SoundplaneDriverListener
{
public:
    HelloSoundplaneDriverListener()    
    {
    }

    virtual void deviceStateChanged(SoundplaneDriver& driver, MLSoundplaneState s) override
    {
        std::cout << "Device state changed: " << s << std::endl;
    }

    virtual void receivedFrame(SoundplaneDriver& driver, const float* data, int size) override
    {
        if (mHasC==false) {
            memcpy(mC,data,sizeof(float)*512);
            mHasC=true;
        }
        else if (mFrameCounter == 0) {
            float sum = 0.0;
            std::cout << "Soundplane data [" << size << "] :"; 
            for(int i=0;i<size;i++){
                if ((i % 64)==0) std::cout << std::endl;
                float v= (data[i] - mC[i]) * 1000.0;
                std::cout << std::fixed << std::setw(6) << std::setprecision(4) << v << ",";
                sum += v;
            }
            std::cout << std::endl;
            std::cout << "Sum : " << sum << std::endl;
        }
        mFrameCounter = (mFrameCounter + 1) % 5000;
    }
private:
    int mFrameCounter = 0;
    float mC[512];
    bool mHasC = false;
};

}

static volatile int keepRunning = 1;
void intHandler(int dummy) {
    std::cerr  << "int handler called";
    if(keepRunning==0) {
        sleep(1);
        exit(-1);
    }
    keepRunning = 0;
}

int main(int argc, const char * argv[])
{
    signal(SIGINT, intHandler);
    HelloSoundplaneDriverListener listener;
    auto driver = SoundplaneDriver::create(&listener);

    std::cout << "Hello, Soundplane?\n";
    std::cout << "Initial device state: " << driver->getDeviceState() << std::endl;

    while(keepRunning)
    {
        sleep(1);
    }
    delete driver.release();


    return 0;
}
