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
#include "pa_ringbuffer.h"

namespace mec {

class Push2_OLED;

class Push2 : public Device {

public:
    Push2(ICallback&);
    virtual ~Push2();
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual bool isActive();

    virtual bool midiInCallback(double deltatime, std::vector< unsigned char > *message);

private:
    struct MidiMsg{
        MidiMsg() : status_(0),data1_(0),data2_(0) {;}
        unsigned status_;
        unsigned data1_;
        unsigned data2_;
    };
    static const unsigned int MAX_N_MIDI_MSGS = 16;

    bool processMidi(const MidiMsg&);
    void processPushNoteOn(unsigned n, unsigned v);
    void processPushNoteOff(unsigned n, unsigned v);
    void processPushCC(unsigned cc, unsigned v);

    bool active_;
	ICallback& callback_;
    // push display
    std::shared_ptr<Push2API::Push2> push2Api_;
    std::shared_ptr<Push2_OLED> oled_;

    // kontrol interface
    std::shared_ptr<oKontrol::ParameterModel>       param_model_;

    // midi
    std::unique_ptr<RtMidiIn>        midiDevice_; 

    MsgQueue    touchQueue_; //touches
    float       pitchbendRange_;
    PaUtilRingBuffer midiQueue_; // draw midi from P2
    char msgData_[sizeof(MidiMsg) * MAX_N_MIDI_MSGS];

    //navigation
    unsigned currentPage_ = 0;
};

}

#endif // MEC_Push2_H