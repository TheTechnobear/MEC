#include "mec_electraone.h"

#include <algorithm>
#include <unordered_set>
#include <unistd.h>

#include <mec_log.h>
#include <RtMidiDevice.h>

namespace mec {


ElectraOne::ElectraOne(ICallback &cb) :
        callback_(cb),
        active_(false),
        sysExStream_(OUTPUT_MAX_SZ),
        stringStream_(OUTPUT_MAX_SZ) {
}

ElectraOne::~ElectraOne() {
    deinit();
}


class ElectraMidiCallback : public ElectraLite::MidiCallback {
public:
    ElectraMidiCallback(ElectraOne *parent) : parent_(parent) { ; }

    void noteOn(unsigned ch, unsigned n, unsigned v) override;
    void noteOff(unsigned ch, unsigned n, unsigned v) override;
    void cc(unsigned ch, unsigned cc, unsigned v) override;
    void pitchbend(unsigned ch, int v) override;
    void ch_pressure(unsigned ch, unsigned v) override;
    void process(const ElectraLite::MidiMsg &msg) override;
    void sysex(const unsigned char *data, unsigned sz);
private:
    ElectraOne *parent_;
};

void ElectraMidiCallback::process(const ElectraLite::MidiMsg &msg) {
    unsigned status = msg.byte(0);
    if (status == 0xF0) {
        sysex(msg.data(), msg.size());
    } else {
        MidiCallback::process(msg);
    }
}

void ElectraMidiCallback::sysex(const unsigned char *data, unsigned sz) {
    unsigned idx = 0;
    unsigned status = data[idx++]; // FO
    unsigned man[3];
    man[0] = data[idx++];
    man[1] = data[idx++];
    man[2] = data[idx++];

    for (auto i = 0; i < 3; i++) {
        if (man[i] != ElectraLite::E1_Manufacturer[i]) {
            std::cerr << "sysex not from electra" << std::hex << man[0] << man[1] << man[2] << std::dec << std::endl;
            return;
        }
    }

    unsigned reqres = data[idx++];

    unsigned datatype = data[idx++];
}


void ElectraMidiCallback::noteOn(unsigned ch, unsigned n, unsigned v) {
}

void ElectraMidiCallback::noteOff(unsigned ch, unsigned n, unsigned v) {
}

void ElectraMidiCallback::cc(unsigned ch, unsigned cc, unsigned v) {
}

void ElectraMidiCallback::pitchbend(unsigned ch, int v) {
}

void ElectraMidiCallback::ch_pressure(unsigned ch, unsigned v) {
}


//mec::Device

bool ElectraOne::init(void *arg) {
    if (active_) {
        LOG_2("ElectraOne::init - already active deinit");
        deinit();
    }

    Preferences prefs(arg);

    static const auto POLL_FREQ = 1;
    static const auto POLL_SLEEP = 1000;
    static const char *E1_Midi_Device_Ctrl = "Electra Controller Electra CTRL";
    std::string electramidi = prefs.getString("midi device", E1_Midi_Device_Ctrl);

    device_ = std::make_shared<ElectraLite::RtMidiDevice>();
    active_ = device_->init(electramidi.c_str(), electramidi.c_str());
    midiCallback_ = std::make_shared<ElectraMidiCallback>(this);

    pollFreq_ = prefs.getInt("poll freq", POLL_FREQ);
    pollSleep_ = prefs.getInt("poll sleep", POLL_SLEEP);
    pollCount_ = 0;

    return active_;
}

void ElectraOne::deinit() {
    device_ = nullptr;
    active_ = false;
    return;
}

bool ElectraOne::isActive() {
    return active_;
}

// Kontrol::KontrolCallback
bool ElectraOne::process() {
    unsigned count = 0;
    pollCount_++;
    if ((pollCount_ % pollFreq_) == 0) {
        if (device_ && active_) {
            device_->processIn(*midiCallback_);
            device_->processOut();
            count++;
        }
    }


    if (pollSleep_) {
        usleep(pollSleep_);
    }
    return true;
}

void ElectraOne::stop() {
    deinit();
}


bool ElectraOne::broadcastChange(Kontrol::ChangeSource src) {
    //todo, see oscbroadcaster
    return true;
}

unsigned ElectraOne::stringToken(const char *str) {
    auto tkn = strToToken_.find(str);
    if (tkn == strToToken_.end()) {
        return createStringToken(str);
    }
    return tkn->second;
}

enum SysExMsgs {
    E1_STRING_TKN,
    E1_START_PUB,
    E1_RACK_END,
    E1_RACK_MSG,
    E1_MODULE_MSG,
    E1_PAGE_MSG,
    E1_PARAM_MSG,
    E1_CHANGED_MSG,
    E1_RESOURCE_MSG,
    E1_SYSEX_MAX
};


void ElectraOne::resetTokenCache() {
    lastToken_ = 0;
    strToToken_.clear();
    tokenToString_.clear();
}

unsigned ElectraOne::createStringToken(const char *cstr) {
    std::string str = cstr;
    // cache token
    ++lastToken_;
    strToToken_[std::string(str)] = lastToken_;
    tokenToString_[lastToken_] = str;

    unsigned ntMSB = (lastToken_ >> 7) & 0b01111111;
    unsigned ntLSB = lastToken_ & 0b01111111;
    unsigned x = 0;

    // send to clients
    stringStream_.begin();
    addSysExHeader(stringStream_, E1_STRING_TKN);
    stringStream_ << ntMSB;
    stringStream_ << ntLSB;
    while (cstr[x]) {
        stringStream_ << ((unsigned) (cstr[x++]) & 0b01111111);
    }
    stringStream_.end();
    send(stringStream_);

    return lastToken_;
}

void ElectraOne::addSysExHeader(SysExStream &sysex, unsigned msgtype) {
    static const unsigned TB_SYSEX_MSG = 0x20;
    sysex << ElectraLite::E1_Manufacturer[0];
    sysex << ElectraLite::E1_Manufacturer[1];
    sysex << ElectraLite::E1_Manufacturer[2];
    sysex << TB_SYSEX_MSG;
    sysex << msgtype;
}


void ElectraOne::addSysExToken(SysExStream &sysex, const char *str) {
    auto tkn = stringToken(str);
    unsigned ntMSB = (tkn >> 7) & 0b01111111;
    unsigned ntLSB = tkn & 0b01111111;
    sysex << ntMSB;
    sysex << ntLSB;
}

void ElectraOne::addSysExString(SysExStream &sysex, const char *str) {
    const char *cstr = str;
    unsigned x = 0;
    while (cstr[x]) {
        sysex << ((unsigned) (cstr[x++]) & 0b01111111);
    }
    sysex << 0;
}


void ElectraOne::addSysExUnsigned(SysExStream &sysex, unsigned v) {
    unsigned vMSB = (v >> 7) & 0b01111111;
    unsigned vLSB = v & 0b01111111;
    sysex << vMSB;
    sysex << vLSB;
}

void ElectraOne::addSysExFloat(SysExStream &sysex, float v) {
    assert(sizeof(float) == 4);
    assert(sizeof(unsigned) == 4);
    unsigned uval = *static_cast<unsigned *>(static_cast<void *>(&v));
    unsigned tmp=uval;

    for (unsigned i = 0; i < 5; i++) {
        unsigned bit7 = uval & 0b01111111;
        sysex << bit7;
        uval = uval >> 7;
    }
//    LOG_0("addSysExFloat " << tmp << " " << v);
}

bool ElectraOne::send(SysExStream &sysex) {
    if (sysex.isValid()) {
        unsigned char *buf = new unsigned char[sysex.size()];
        memcpy(buf, sysex.buffer(), sysex.size());
        // mididevice takes ownership of these bytes!
        return device_->sendBytes(buf, sysex.size());
    }
    LOG_0("ElectraOne::send(SysExStream) - invalid sysex, buffer size?");
    return false;
}


void ElectraOne::rack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    SysExStream &sysex = sysExStream_;
    sysex.begin();

    addSysExHeader(sysex, E1_RACK_MSG);
    addSysExToken(sysex, rack.id().c_str());
    addSysExString(sysex, rack.host().c_str());
    addSysExUnsigned(sysex, rack.port());

    sysex.end();
    send(sysex);
    LOG_0("Published RACK : " << rack.id());
}

void ElectraOne::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &m) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    SysExStream &sysex = sysExStream_;
    sysex.begin();

    addSysExHeader(sysex, E1_MODULE_MSG);
    addSysExToken(sysex, rack.id().c_str());
    addSysExToken(sysex, m.id().c_str());
    addSysExString(sysex, m.displayName().c_str());
    addSysExString(sysex, m.type().c_str());

    sysex.end();
    send(sysex);
}

void ElectraOne::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                      const Kontrol::Module &module, const Kontrol::Page &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    SysExStream &sysex = sysExStream_;
    sysex.begin();

    addSysExHeader(sysex, E1_PAGE_MSG);
    addSysExToken(sysex, rack.id().c_str());
    addSysExToken(sysex, module.id().c_str());
    addSysExToken(sysex, p.id().c_str());
    addSysExString(sysex, p.displayName().c_str());

    for (const std::string &paramId : p.paramIds()) {
        addSysExString(sysex,  paramId.c_str());
    }

    sysex.end();
    send(sysex);
}

void ElectraOne::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                       const Kontrol::Module &module, const Kontrol::Parameter &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    SysExStream &sysex = sysExStream_;
    sysex.begin();

    addSysExHeader(sysex, E1_PARAM_MSG);
    addSysExToken(sysex, rack.id().c_str());
    addSysExToken(sysex, module.id().c_str());

    std::vector<Kontrol::ParamValue> values;
    p.createArgs(values);
    for (auto &v : values) {
        switch (v.type()) {
            case Kontrol::ParamValue::T_Float: {
                addSysExUnsigned(sysex, v.type());
                addSysExFloat(sysex, v.floatValue());
                break;
            }
            case Kontrol::ParamValue::T_String:
            default: {
                addSysExUnsigned(sysex, Kontrol::ParamValue::T_String);
                addSysExString(sysex, v.stringValue().c_str());
            }
        }
    }

    sysex.end();
    send(sysex);
}

void ElectraOne::changed(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                         const Kontrol::Module &module, const Kontrol::Parameter &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    SysExStream &sysex = sysExStream_;
    sysex.begin();

    addSysExHeader(sysex, E1_CHANGED_MSG);
    addSysExToken(sysex, rack.id().c_str());
    addSysExToken(sysex, module.id().c_str());

    addSysExToken(sysex, p.id().c_str());

    auto v = p.current();
    switch (v.type()) {
        case Kontrol::ParamValue::T_Float: {
            addSysExUnsigned(sysex, v.type());
            addSysExFloat(sysex, v.floatValue());
            break;
        }
        case Kontrol::ParamValue::T_String:
        default: {
            addSysExUnsigned(sysex, Kontrol::ParamValue::T_String);
            addSysExString(sysex, v.stringValue().c_str());
        }
    }

    sysex.end();
    send(sysex);
}

void ElectraOne::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                          const std::string &type, const std::string &res) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    SysExStream &sysex = sysExStream_;
    sysex.begin();

    addSysExHeader(sysex, E1_RESOURCE_MSG);
    addSysExToken(sysex, rack.id().c_str());
    addSysExToken(sysex, type.c_str());
    addSysExString(sysex, res.c_str());

    sysex.end();
    send(sysex);
}

void ElectraOne::deleteRack(Kontrol::ChangeSource src, const Kontrol::Rack &rack) {
}

void ElectraOne::activeModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                              const Kontrol::Module &module) {
}

void ElectraOne::loadModule(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                            const Kontrol::EntityId &modId, const std::string &modType) {
}


void ElectraOne::publishStart(Kontrol::ChangeSource src, unsigned numRacks) {
    resetTokenCache();
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    SysExStream &sysex = sysExStream_;

    sysex.begin();
    addSysExHeader(sysex, E1_START_PUB);
    addSysExUnsigned(sysex, numRacks);
    sysex.end();
    send(sysex);

}

void ElectraOne::publishRackFinished(Kontrol::ChangeSource src, const Kontrol::Rack &r) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    SysExStream &sysex = sysExStream_;

    sysex.begin();
    addSysExHeader(sysex, E1_RACK_END);
    addSysExToken(sysex, r.id().c_str());
    sysex.end();
    send(sysex);
}

void ElectraOne::midiLearn(Kontrol::ChangeSource src, bool b) {
}

void ElectraOne::modulationLearn(Kontrol::ChangeSource src, bool b) {
}

void ElectraOne::savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
}

void ElectraOne::loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
}


}