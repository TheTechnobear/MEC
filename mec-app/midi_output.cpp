#include "midi_output.h"

#include "mec_app.h"

#ifdef __linux__
#include <alsa/asoundlib.h>

// Imported from RtMidi library.
extern unsigned int portInfo(snd_seq_t *seq, snd_seq_port_info_t *pinfo, unsigned int type, int portNumber);

bool findRtMidiPortId(unsigned &result, const std::string &portName, bool outputPort) {
    snd_seq_t *seq;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
        return false;

    result = 0;
    bool success = false;
    const unsigned int type = outputPort ? SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE : SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ;

    snd_seq_addr_t addr;
    if (snd_seq_parse_address(seq, &addr, portName.c_str()) >= 0) {
        snd_seq_port_info_t *info;
        snd_seq_port_info_alloca(&info);
        unsigned count = portInfo(seq, info, type, -1);

        for (unsigned i = 0; i < count; ++i) {
            portInfo(seq, info, type, i);

            if (memcmp(&addr, snd_seq_port_info_get_addr(info), sizeof(addr)) == 0) {
                result = i;
                success = true;
                break;
            }
        }
    }

    snd_seq_close(seq);
    return success;
}

#else

bool findRtMidiPortId(unsigned &result, const std::string &portName, bool outputPort)
{
    RtMidiOut out;
    RtMidiIn in;
    RtMidi &rt = outputPort ? (RtMidi&)out : (RtMidi&)in;

    for (unsigned i = 0; i < rt.getPortCount(); i++) {
        if (portName.compare(rt.getPortName(i)) == 0) {
            result = i;
            return true;
        }
    }
    return false;
}

#endif // __linux__

MidiOutput::MidiOutput() {
    try {
        output_.reset(new RtMidiOut(RtMidi::Api::UNSPECIFIED, "MEC MIDI OUTPUT"));
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
            output_->openVirtualPort("MIDI OUT");
            LOG_0("Midi virtual output created :" << portname);
            virtualOpen_ = true; // port is open because it belongs to client
        } catch (RtMidiError &error) {
            LOG_0("Midi virtual output create error:" << error.what());
            return false;
        }
        return true;
    }

    unsigned port;
    if (findRtMidiPortId(port, portname.c_str(), true)) {
        try {
            output_->openPort(port, "MIDI OUT");
            LOG_0("Midi output opened :" << portname);
        } catch (RtMidiError &error) {
            LOG_0("Midi output create error:" << error.what());
            return false;
        }
        return true;
    }
    LOG_0("Port not found : [" << portname << "]");
    LOG_0("available ports : ");
    for (unsigned i = 0; i < output_->getPortCount(); i++) {
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

