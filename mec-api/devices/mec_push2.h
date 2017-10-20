#ifndef MEC_Push2_H
#define MEC_Push2_H

#include "../mec_api.h"
#include "../mec_device.h"
#include "../mec_msg_queue.h"

#include "mec_mididevice.h"

#include "ParameterModel.h"

#include <RtMidi.h>

#include <memory>
#include <vector>

#include <push2lib/push2lib.h>
#include "pa_ringbuffer.h"

namespace mec {

class Push2_OLED;

class Push2 : public MidiDevice {

public:
    Push2(ICallback&);
    virtual ~Push2();
    //mididevice : device
    virtual bool init(void*);
    virtual bool process();
    virtual void deinit();
    virtual RtMidiIn::RtMidiCallback getMidiCallback(); //override

    virtual bool midiCallback(double deltatime, std::vector< unsigned char > *message);

private:
    friend class Push2_OLED;

    static const unsigned int MAX_N_MIDI_MSGS = 16;

    bool processMidi(const MidiMsg&);
    void processPushNoteOn(unsigned n, unsigned v);
    void processPushNoteOff(unsigned n, unsigned v);
    void processPushCC(unsigned cc, unsigned v);

    // push display
    std::shared_ptr<Push2API::Push2> push2Api_;
    std::shared_ptr<Push2_OLED> oled_;

    // kontrol interface
    std::shared_ptr<Kontrol::ParameterModel>       param_model_;


    float       pitchbendRange_;
    PaUtilRingBuffer midiQueue_; // draw midi from P2
    char msgData_[sizeof(MidiMsg) * MAX_N_MIDI_MSGS];

    //navigation
    unsigned currentPage_ = 0;

    // scales colour
    void updatePadColours();
    int8_t   determinePadScaleColour(int8_t r, int8_t c);
    uint8_t  octave_;    // current octave
    uint8_t  scaleIdx_;
    uint16_t scale_;     // current scale
    uint8_t  numNotesInScale_;   // number of notes in current scale
    uint8_t  tonic_;     // current tonic
    uint8_t  rowOffset_; // current tonic
    bool     chromatic_; // are we in chromatic mode or not


};

}

#endif // MEC_Push2_H