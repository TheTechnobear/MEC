#include "mec_electraone.h"

#include <algorithm>
#include <unistd.h>

#include <mec_log.h>
#include <RtMidiDevice.h>

namespace mec {


ElectraOne::ElectraOne(ICallback &cb) :
        callback_(cb),
        active_(false),
        sysExOutStream_(OUTPUT_MAX_SZ),
        stringOutStream_(OUTPUT_MAX_SZ) {
}

ElectraOne::~ElectraOne() {
    deinit();
}


class ElectraMidiCallback : public ElectraLite::MidiCallback {
public:
    ElectraMidiCallback(ElectraOne *parent) :
            parent_(parent),
            model_(Kontrol::KontrolModel::model()) {
        ;
    }

    void noteOn(unsigned ch, unsigned n, unsigned v) override;
    void noteOff(unsigned ch, unsigned n, unsigned v) override;
    void cc(unsigned ch, unsigned cc, unsigned v) override;
    void pitchbend(unsigned ch, int v) override;
    void ch_pressure(unsigned ch, unsigned v) override;
    void process(const ElectraLite::MidiMsg &msg) override;
    void sysex(const unsigned char *data, unsigned sz);
private:
    ElectraOne *parent_;
    std::shared_ptr<Kontrol::KontrolModel> model_;
};

void ElectraMidiCallback::process(const ElectraLite::MidiMsg &msg) {
    unsigned b = 0;
    unsigned status = msg.byte(b++);
    if (status == 0xF0) {
        bool notmine = false;
        notmine |= (msg.byte(b++) != E1_Manufacturer[0]);
        notmine |= (msg.byte(b++) != E1_Manufacturer[1]);
        notmine |= (msg.byte(b++) != E1_Manufacturer[2]);
        if (!notmine) {
            unsigned msgid = msg.byte(b++);
            if (msgid == TB_SYSEX_MSG) {
                sysex(msg.data(), msg.size());
            } else if (msgid == 0x7f) {
                unsigned type = msg.byte(b++);
                // report E1 logs
                if (type == 0x00) { // log
                    std::stringbuf buf;
                    unsigned char c;
                    while ((c = msg.byte(b++)) != 0xF7) {
                        buf.sputc(c);
                    }
                    LOG_0("E1 Log >> " << buf.str());
                }
            }
        }
    } else {
        MidiCallback::process(msg);
    }
}

void ElectraMidiCallback::sysex(const unsigned char *data, unsigned sz) {
    Kontrol::ChangeSource src = Kontrol::ChangeSource(Kontrol::ChangeSource::SrcType::REMOTE, "E1");
    SysExInputStream sysex(data, sz);
    bool ok = parent_->handleE1SysEx(src, sysex, model_);
    if (!ok) {
        LOG_0("ElectraMidiCallback::sysex - invalid sysex");
    }

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
    //TODO, see oscbroadcaster
    Kontrol::ChangeSource e1src = Kontrol::ChangeSource(Kontrol::ChangeSource::SrcType::REMOTE, "E1");
    return !(src == e1src);
    return true;
}

unsigned ElectraOne::stringToken(const char *str) {
    auto tkn = strToToken_.find(str);
    if (tkn == strToToken_.end()) {
        return createStringToken(str);
    }
    return tkn->second;
}

const std::string &ElectraOne::tokenString(unsigned tkn) {
    static const std::string invalid("INVALID");
    auto str = tokenToString_.find(tkn);
    if (str == tokenToString_.end()) {
        return invalid;
    }
    return str->second;
}


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

    unsigned x = 0;

    // send to clients
    stringOutStream_.begin();
    stringOutStream_.addHeader(E1_STRING_TKN);
    stringOutStream_.addUnsigned(lastToken_);
    stringOutStream_.addString(cstr);
    stringOutStream_.end();
    send(stringOutStream_);

    return lastToken_;
}

bool ElectraOne::send(SysExOutputStream &sysex) {
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
    auto &sysex = sysExOutStream_;
    sysex.begin();

    sysex.addHeader(E1_RACK_MSG);
    sysex.addUnsigned(stringToken(rack.id().c_str()));
    sysex.addString(rack.host().c_str());
    sysex.addUnsigned(rack.port());

    sysex.end();
    send(sysex);
    LOG_0("Published RACK : " << rack.id());
}

void ElectraOne::module(Kontrol::ChangeSource src, const Kontrol::Rack &rack, const Kontrol::Module &m) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    auto &sysex = sysExOutStream_;
    sysex.begin();

    sysex.addHeader(E1_MODULE_MSG);
    sysex.addUnsigned(stringToken(rack.id().c_str()));
    sysex.addUnsigned(stringToken(m.id().c_str()));
    sysex.addString(m.displayName().c_str());
    sysex.addString(m.type().c_str());

    sysex.end();
    send(sysex);
}

void ElectraOne::page(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                      const Kontrol::Module &module, const Kontrol::Page &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    auto &sysex = sysExOutStream_;
    sysex.begin();

    sysex.addHeader(E1_PAGE_MSG);
    sysex.addUnsigned(stringToken(rack.id().c_str()));
    sysex.addUnsigned(stringToken(module.id().c_str()));
    sysex.addUnsigned(stringToken(p.id().c_str()));
    sysex.addString(p.displayName().c_str());

    for (const std::string &paramId : p.paramIds()) {
        sysex.addString(paramId.c_str());
    }

    sysex.end();
    send(sysex);
}

void ElectraOne::param(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                       const Kontrol::Module &module, const Kontrol::Parameter &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    auto &sysex = sysExOutStream_;
    sysex.begin();

    sysex.addHeader(E1_PARAM_MSG);
    sysex.addUnsigned(stringToken(rack.id().c_str()));
    sysex.addUnsigned(stringToken(module.id().c_str()));

    std::vector<Kontrol::ParamValue> values;
    p.createArgs(values);
    for (auto &v : values) {
        switch (v.type()) {
            case Kontrol::ParamValue::T_Float: {
                sysex.addUnsigned(v.type());
                sysex.addFloat(v.floatValue());
                break;
            }
            case Kontrol::ParamValue::T_String:
            default: {
                sysex.addUnsigned(Kontrol::ParamValue::T_String);
                sysex.addString(v.stringValue().c_str());
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
    auto &sysex = sysExOutStream_;
    sysex.begin();

    sysex.addHeader(E1_CHANGED_MSG);
    sysex.addUnsigned(stringToken(rack.id().c_str()));
    sysex.addUnsigned(stringToken(module.id().c_str()));
    sysex.addString(p.id().c_str());

    auto v = p.current();
    switch (v.type()) {
        case Kontrol::ParamValue::T_Float: {
            sysex.addUnsigned(v.type());
            sysex.addFloat(v.floatValue());
            break;
        }
        case Kontrol::ParamValue::T_String:
        default: {
            sysex.addUnsigned(Kontrol::ParamValue::T_String);
            sysex.addString(v.stringValue().c_str());
        }
    }

    sysex.end();
    send(sysex);
}

void ElectraOne::resource(Kontrol::ChangeSource src, const Kontrol::Rack &rack,
                          const std::string &type, const std::string &res) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    auto &sysex = sysExOutStream_;
    sysex.begin();

    sysex.addHeader(E1_RESOURCE_MSG);
    sysex.addUnsigned(stringToken(rack.id().c_str()));
    sysex.addUnsigned(stringToken(type.c_str()));
    sysex.addString(res.c_str());

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

    auto &sysex = sysExOutStream_;

    sysex.begin();
    sysex.addHeader(E1_START_PUB);
    sysex.addUnsigned(numRacks);
    sysex.end();
    send(sysex);

}

void ElectraOne::publishRackFinished(Kontrol::ChangeSource src, const Kontrol::Rack &r) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    auto &sysex = sysExOutStream_;

    sysex.begin();
    sysex.addHeader(E1_RACK_END);
    sysex.addUnsigned(stringToken(r.id().c_str()));
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


bool ElectraOne::handleE1SysEx(Kontrol::ChangeSource src, SysExInputStream &sysex,
                               std::shared_ptr<Kontrol::KontrolModel> model) {
    bool valid = sysex.readHeader();
    if (!valid) {
        return false;
    };
    char msgid = sysex.read();

    switch (msgid) {
        case E1_STRING_TKN: {
            unsigned tkn = sysex.readUnsigned();
            std::string str = sysex.readString();
            tokenToString_[tkn] = str;
            // logMessage("New String tkn=%d str=%s", tkn, str.c_str());
            break;
        }
        case E1_START_PUB: {
            tokenToString_.clear();
            unsigned numRacks = sysex.readUnsigned();
            // logMessage("E1_START_PUB %d", numRacks);
            model->publishStart(src, numRacks);
            break;
        }
        case E1_RACK_END: {
            Kontrol::EntityId rackId = tokenString(sysex.readUnsigned());
            // logMessage("E1_RACK_END %s", rackId.c_str());
            model->publishRackFinished(src, rackId);
            break;
        }
        case E1_RACK_MSG: {
            Kontrol::EntityId rackId = tokenString(sysex.readUnsigned());
            std::string host = sysex.readString();
            unsigned port = sysex.readUnsigned();

            // logMessage("E1_RACK_MSG %s %s %u", rackId.c_str(), host.c_str(),port);
            model->createRack(src, rackId, host, port);
            break;
        }
        case E1_MODULE_MSG: {
            Kontrol::EntityId rackId = tokenString(sysex.readUnsigned());
            Kontrol::EntityId moduleId = tokenString(sysex.readUnsigned());
            std::string displayName = sysex.readString();
            std::string type = sysex.readString();

            // logMessage("E1_MODULE_MSG %s %s %s %s",
            // rackId.c_str(),moduleId.c_str(),displayName.c_str(),type.c_str());
            model->createModule(src, rackId, moduleId, displayName, type);
            break;
        }
        case E1_PAGE_MSG: {
            Kontrol::EntityId rackId = tokenString(sysex.readUnsigned());
            Kontrol::EntityId moduleId = tokenString(sysex.readUnsigned());
            Kontrol::EntityId pageId = tokenString(sysex.readUnsigned());
            std::string displayName = sysex.readString();

            std::vector<Kontrol::EntityId> paramIds;
            while (!sysex.atEnd()) {
                std::string val = sysex.readString();
                paramIds.push_back(val);
            }

            // logMessage("E1_PAGE_MSG %s %s %s %s", rackId.c_str(),
            // moduleId.c_str(),pageId.c_str(), displayName.c_str());
            model->createPage(src, rackId, moduleId, pageId, displayName, paramIds);

            break;
        }
        case E1_PARAM_MSG: {
            std::vector<Kontrol::ParamValue> params;
            Kontrol::EntityId rackId = tokenString(sysex.readUnsigned());
            Kontrol::EntityId moduleId = tokenString(sysex.readUnsigned());
            while (!sysex.atEnd()) {
                auto type = (Kontrol::ParamValue::Type) sysex.readUnsigned();
                switch (type) {
                    case Kontrol::ParamValue::T_Float: {
                        float val = sysex.readFloat();
                        params.push_back(Kontrol::ParamValue(val));
                        break;
                    }
                    case Kontrol::ParamValue::T_String:
                    default: {
                        std::string val = sysex.readString();
                        params.push_back(Kontrol::ParamValue(val));
                        break;
                    }
                }
            }

            // logMessage("E1_PARAM_MSG %s %s", rackId.c_str(), moduleId.c_str());
            model->createParam(src, rackId, moduleId, params);
            break;
        }
        case E1_CHANGED_MSG: {
            Kontrol::EntityId rackId = tokenString(sysex.readUnsigned());
            Kontrol::EntityId moduleId = tokenString(sysex.readUnsigned());
            Kontrol::EntityId paramId = sysex.readString();

            if (!sysex.atEnd()) {
                auto type = (Kontrol::ParamValue::Type) sysex.readUnsigned();
                switch (type) {
                    case Kontrol::ParamValue::T_Float: {
                        float val = sysex.readFloat();
                        // logMessage("E1_CHANGED_MSG %s %s %s %f" , rackId.c_str(),
                        // moduleId.c_str(),paramId.c_str(), val);
                        model->changeParam(src, rackId, moduleId, paramId,
                                           Kontrol::ParamValue(val));
                        break;
                    }

                    case Kontrol::ParamValue::T_String:
                    default: {
                        std::string val = sysex.readString();
                        // logMessage("E1_CHANGED_MSG %s %s %s %s", rackId.c_str(),
                        // moduleId.c_str(),paramId.c_str(), val.c_str());
                        model->changeParam(src, rackId, moduleId, paramId,
                                           Kontrol::ParamValue(val));
                        break;
                    }
                }
            }
            break;
        }
        case E1_RESOURCE_MSG: {
            Kontrol::EntityId rackId = tokenString(sysex.readUnsigned());
            Kontrol::EntityId resType = tokenString(sysex.readUnsigned());
            std::string resValue = sysex.readString();
            // logMessage("E1_RESOURCE_MSG %s %s %s", rackId.c_str(),
            // resType.c_str(),resValue.c_str());
            model->createResource(src, rackId, resType, resValue);
            break;
        }

        case E1_SYSEX_MAX:
        default: {
            return false;
        }
    }
    return true;
}

} //namespace


