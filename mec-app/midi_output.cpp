#include "midi_output.h"

#include "mec_app.h"


MidiOutput::MidiOutput() {
    try {
        output_.reset(new RtMidiOut(RtMidi::Api::UNSPECIFIED, "MEC MIDI"));
    } catch (RtMidiError &error) {
        LOG_0("Midi output ctor error:" << error.what());
    }
}

MidiOutput::~MidiOutput() {
    output_.reset();
}


bool MidiOutput::create(const std::string &portname, bool virt) {

    if (!output_) return false;

    if (output_->isPortOpen()) output_->closePort();

    virtualOpen_ = false;
    if (virt) {
        try {
            output_->openVirtualPort(portname);
            LOG_0("Midi virtual output created :" << portname);
            virtualOpen_ = true; // port is open because it belongs to client
        } catch (RtMidiError &error) {
            LOG_0("Midi virtual output create error:" << error.what());
            return false;
        }
        return true;
    }

    for (int i = 0; i < output_->getPortCount(); i++) {
        if (portname.compare(output_->getPortName(i)) == 0) {
            try {
                output_->openPort(i);
                LOG_0("Midi output opened :" << portname);
            } catch (RtMidiError &error) {
                LOG_0("Midi output create error:" << error.what());
                return false;
            }
            return true;
        }
    }
    LOG_0("Port not found : [" << portname << "]");
    LOG_0("available ports : ");
    for (int i = 0; i < output_->getPortCount(); i++) {
        LOG_0("[" << output_->getPortName(i) << "]");
    }

    return false;
}

bool MidiOutput::sendMsg(std::vector<unsigned char> &msg) {
    if (!isOpen()) return false;

    try {
        output_->sendMessage(&msg);
    } catch (RtMidiError &error) {
        LOG_0("Midi output write error:" << error.what());
        return false;
    }
    return true;
}

