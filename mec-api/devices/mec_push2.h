#ifndef MEC_Push2_H 
#define MEC_Push2_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"


#include "ParameterModel.h"

#include <RtMidi.h>

#include <memory>
#include <vector>

#include <push2lib/push2lib.h>

namespace mec {

class Push2 : public Device {

public:
    Push2(ICallback&);
    virtual ~Push2();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    virtual bool midiCallback(double deltatime, std::vector< unsigned char > *message);

private:

    bool active_;
	ICallback& callback_;
    // push display
    std::shared_ptr<Push2API::Push2> push2Api_;

    // kontrol interface
    std::shared_ptr<oKontrol::ParameterModel>       param_model_;

    // midi
    std::unique_ptr<RtMidiIn>        midiDevice_; 
    MsgQueue    queue_;
    float       pitchbendRange_;


};

}

#endif // MEC_Push2_H