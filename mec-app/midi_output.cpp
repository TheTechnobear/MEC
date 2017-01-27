#include "midi_output.h"

#include "mec_app.h"


MidiOutput::MidiOutput(int maxVoices, float pbr) : pitchbendRange_(pbr) {
    for (int i = 0; i < 127; i++) {
        global_[i] = 0;
    }
    try {
        output_.reset(new RtMidiOut( RtMidi::Api::UNSPECIFIED, "MEC MIDI"));
    } catch (RtMidiError  &error) {
        LOG_0("Midi output ctor error:" << error.what());
    }
}

MidiOutput::~MidiOutput() {
    output_.reset();
}


bool MidiOutput::create(const std::string& portname,bool virt) {

    if (!output_) return false;

    if (output_->isPortOpen()) output_->closePort();

	if(virt) {
		try {
			output_->openVirtualPort(portname);
			LOG_0( "Midi virtual output created :" << portname );
		} catch (RtMidiError  &error) {
			error.printMessage();
			return false;
		}
		return true;
	}

    for (int i = 0; i < output_->getPortCount(); i++) {
        if (portname.compare(output_->getPortName(i)) == 0) {
            try {
                output_->openPort(i);
                LOG_0( "Midi output opened :" << portname );
            } catch (RtMidiError  &error) {
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

bool MidiOutput::sendMsg(std::vector<unsigned char>& msg) {
    if (!isOpen()) return false;

    try {
        output_->sendMessage( &msg );
    } catch (RtMidiError  &error) {
        LOG_0("Midi output write error:" << error.what());
        return false;
    }
    return true;
}

bool MidiOutput::noteOn(unsigned ch, unsigned note, unsigned vel) {
    std::vector<unsigned char> msg;

    // LOG_2( "midi output note on ch " << ch << " note " << note  << " vel " << vel )
    msg.push_back(0x90 + ch);
    msg.push_back(note);
    msg.push_back(vel);
    output_->sendMessage( &msg );

    return true;
}


bool MidiOutput::noteOff(unsigned ch, unsigned note, unsigned vel) {
    std::vector<unsigned char> msg;

    // LOG_2( "midi output note off ch " << ch << " note " << note  << " vel " << vel )
    msg.push_back(0x80 + ch);
    msg.push_back(note);
    msg.push_back(vel);
    output_->sendMessage( &msg );

    return true;
}

bool MidiOutput::cc(unsigned ch, unsigned cc, unsigned v) {
    std::vector<unsigned char> msg;

    msg.push_back(0xB0 + ch);
    msg.push_back(cc);
    msg.push_back(v);
    output_->sendMessage( &msg );

    return true;
}

bool MidiOutput::pressure(unsigned ch, unsigned v) {
    std::vector<unsigned char> msg;

    msg.push_back( 0xD0 + ch ); // Ch Pres
    msg.push_back( v );
    output_->sendMessage( &msg );

    return true;
}

bool MidiOutput::pitchbend(unsigned ch, unsigned v) {
    std::vector<unsigned char> msg;

    // LOG_2( "midi output pb" << ch << " pb " << v <<  std::endl;)
    msg.push_back( 0xE0 + ch );
    msg.push_back( v & 0x7f);
    msg.push_back( (v & 0x3F80) >> 7);
    output_->sendMessage( &msg );

    return true;
}


bool MidiOutput::control(int id, int attr, float v, bool isBipolar) {
    if (!isOpen()) return false;

    if (global_[id] != v ) {
        global_[id] = v;
        unsigned ch = id;
        cc(ch, attr, isBipolar ? bipolar7bit(v) : unipolar7bit(v));
    }

    return true;
}

bool MidiOutput::touchOn(int id, float note, float x, float y, float z) {
    if (!isOpen()) return false;

    MidiVoiceData& voice = voices_[id];

    unsigned ch = id;
    voice.startNote_ = note; //int

    float semis = note - float(voice.startNote_); 
    int pb = bipolar14bit(semis / pitchbendRange_);

    // unsigned mx = bipolar14bit(x);
    int my = bipolar7bit(y);
    int mz = unipolar7bit(z);

    // LOG_2( "midi output on note: " << note)
    // LOG_2(          << " x :" << x << " mx: " << mx)
    // LOG_2(          << " y :" << y << " my: " << my)
    // LOG_2(          << " z :" << z << " mz: " << mz)
    // LOG_2(          )

    pitchbend(ch, pb);
    cc(ch, 74,  my);
    noteOn(ch, note, mz);

    voice.note_ = note;
    voice.pitchbend_ = pb;
    voice.timbre_ = my;

    // start with zero z, as we use intial z of velocity
    pressure(ch, 0);
    voice.pressure_ = 0;

    return true;
}

bool MidiOutput::touchContinue(int id, float note, float x, float y, float z) {
    if (!isOpen()) return false;

    MidiVoiceData& voice = voices_[id];
    unsigned ch = id;
    // unsigned mx = bipolar14bit(x);
    int my = bipolar7bit(y);
    unsigned mz = unipolar7bit(z);

    float semis = note - float(voice.startNote_); 
    int pb = bipolar14bit(semis / pitchbendRange_);

    // LOG_2(std::cout  << "midi output c")
    // LOG_2(           << " note :" << note << " pb: " << pb << " semis: " << semis)
    // LOG_2(           << " y :" << y << " my: " << my)
    // LOG_2(           << " z :" << z << " mz: " << mz)
    // LOG_2(           << " startnote :" << voice.startNote_ << " pbr: " << pitchbendRange_)
    // LOG_2(           )

    voice.note_ = note;

    if (voice.pitchbend_ != pb) {
        voice.pitchbend_ = pb;
        pitchbend(ch, pb) ;
    }
    if (voice.timbre_ != my) {
        voice.timbre_ = my;
        cc(ch, 74, my);
    }
    if (voice.pressure_ != mz) {
        voice.pressure_ = mz;
        pressure(ch, mz);
    }

    return true;
}

bool MidiOutput::touchOff(int id) {
    if (!isOpen()) return false;

    MidiVoiceData& voice = voices_[id];

    unsigned ch = id;
    unsigned note = voice.startNote_;
    unsigned vel = 0; // last vel = release velocity
    noteOff(ch, note, vel);
    pressure(ch, 0);

    voice.startNote_ = 0;
    voice.note_ = 0;
    voice.pitchbend_ = 0;
    voice.timbre_ = 0;
    voice.pressure_ = 0;//

    return true;
}

