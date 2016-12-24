#ifndef __SOUNDPLANE_MEC_OUTPUT_H_
#define __SOUNDPLANE_MEC_OUTPUT_H_

#include <vector>
#include <list>
#include <memory>
#include <stdint.h>

#include "MLDebug.h"
#include "SoundplaneDataListener.h"
#include "SoundplaneModelA.h"
#include "TouchTracker.h"


class MECCallback {
public:
	MECCallback() {};
	virtual ~MECCallback() {};
    virtual void device(const char* dev, int rows, int cols) {};
    virtual void touch(const char* dev, unsigned long long t, bool a, int touch, float note, float x, float y, float z) {};
    virtual void global(const char* dev, unsigned long long t, int id, float val) {};
};


class SoundplaneMECOutput :
	public SoundplaneDataListener
{
public:
	SoundplaneMECOutput();
	~SoundplaneMECOutput();

	void connect(MECCallback* cb);
    void deviceInit();
	
    // SoundplaneDataListener
    void processSoundplaneMessage(const SoundplaneDataMessage* msg);
    
    
    void setActive(bool v);
	void setSerialNumber(int s) { serialNumber_ = std::to_string(s); deviceInit();}

    void setDataFreq(float v) {mDataFreq = v;}
    void setMaxTouches(int) {};

	void notify(int connected);
	
	void doInfrequentTasks();

private:	
	MECCallback	*callback_;
	std::string serialNumber_;
	float	 mDataFreq;
	uint64_t mCurrFrameStartTime;
	uint64_t mLastFrameStartTime;
    bool 	 mTimeToSendNewFrame;
};


#endif // __SOUNDPLANE_MEC_OUTPUT_H_
