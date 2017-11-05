#ifndef MEC_MIDI_OUTPUT_H
#define MEC_MIDI_OUTPUT_H

#include <memory>
#include <RtMidi.h>


class MidiOutput {
public:
    MidiOutput();
    virtual ~MidiOutput();

    bool create(const std::string &portname, bool virt = false);

    bool isOpen() { return (output_ && (virtualOpen_ || output_->isPortOpen())); }

    bool sendMsg(std::vector<unsigned char> &msg);
private:
    std::unique_ptr<RtMidiOut> output_;
    bool virtualOpen_;
};

#endif //MEC_MIDI_OUTPUT_H
