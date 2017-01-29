#ifndef MecPush2_H 
#define MecPush2_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

#include <RtMidi.h>

#include <memory>
#include <vector>

#include <push2lib/push2lib.h>

class MecPush2 : public MecDevice {

public:
    MecPush2(IMecCallback&);
    virtual ~MecPush2();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    virtual bool midiCallback(double deltatime, std::vector< unsigned char > *message);

private:

    bool active_;
	IMecCallback& callback_;
    std::unique_ptr<Push2API::Push2> push2Api_;
    std::unique_ptr<RtMidiIn>        midiDevice_; 
    MecMsgQueue queue_;

    float       pitchbendRange_;
};

#endif // MecPush2_H