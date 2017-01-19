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


class SoundplaneMECCallback {
public:
	virtual ~SoundplaneMECCallback() {};
    virtual void device(const char* dev, int rows, int cols) = 0;
    virtual void touch(const char* dev, unsigned long long t, bool a, int touch, float note, float x, float y, float z) = 0;
    virtual void control(const char* dev, unsigned long long t, int id, float val) = 0;
};

class SoundplaneMECOutput_Impl;

class SoundplaneMECOutput :
	public SoundplaneDataListener
{
public:
	SoundplaneMECOutput();
	~SoundplaneMECOutput();

	void connect(SoundplaneMECCallback* cb);
    void deviceInit();
	
    // SoundplaneDataListener
    void processSoundplaneMessage(const SoundplaneDataMessage* msg);
    void setActive(bool v);
	void setSerialNumber(int s);
    void setDataFreq(float v);
    void setMaxTouches(int);
	void notify(int connected);
	void doInfrequentTasks();

private:	
	SoundplaneMECOutput_Impl *impl_;
};


#endif // __SOUNDPLANE_MEC_OUTPUT_H_
