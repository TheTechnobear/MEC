#pragma once

#include "../mec_device.h"
#include "mec_mididevice.h"

#include <KontrolModel.h>

// #include <ip/UdpSocket.h>
// #include <string>
// #include <readerwriterqueue.h>
// #include <thread>

#include <iostream>

#include <ElectraMidi.h>
#include <MidiDevice.h>

namespace mec {

class SysExStream {
public:
    SysExStream(unsigned max_sz) : max_sz_(max_sz), size_(0), buf_(new unsigned char[max_sz]) { ; }

    ~SysExStream() {
        delete[] buf_;
        buf_ = nullptr;
    }

    SysExStream &operator<<(unsigned b) {
        if (size_ < max_sz_ - 1) {
            buf_[size_++] = b;
        }
        return *this;
    }

    SysExStream(SysExStream &) = delete;
    SysExStream &operator=(SysExStream &) = delete;

    void begin() {
        size_ = 0;
        *this << 0xF0;
    }

    void end() {
        *this << 0xF7;
    }

    bool isValid() {
        return buf_[size_ - 1] == 0xF7;
    }

    unsigned size() { return size_; }

    unsigned char *buffer() { return buf_; }

private:
    unsigned max_sz_ = 0;
    unsigned char *buf_;
    unsigned size_ = 0;
};


class ElectraOne : public Device, public Kontrol::KontrolCallback {
public:
    ElectraOne(ICallback &cb);
    virtual ~ElectraOne();

    //mec::Device
    bool init(void *) override;
    bool process() override;
    void deinit() override;
    bool isActive() override;

    //Kontrol::KontrolCallback

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override;
    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Page &) override;
    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override;
    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override;
    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &, const std::string &) override;
    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::EntityId &,
                    const std::string &) override;
    void savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;

    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;

    void publishStart(Kontrol::ChangeSource, unsigned numRacks) override;
    void publishRackFinished(Kontrol::ChangeSource, const Kontrol::Rack &) override;

    std::shared_ptr<Kontrol::KontrolModel> model() { return Kontrol::KontrolModel::model(); }


private:
    bool send(SysExStream &sysex);

    void stop() override;

    bool broadcastChange(Kontrol::ChangeSource src);
    unsigned stringToken(const char *);

    void addSysExHeader(SysExStream &sysex, unsigned msgtype);
    void addSysExString(SysExStream &sysex, const char* str);
    void addSysExUnsigned(SysExStream &sysex, unsigned v);

    void resetTokenCache();
    unsigned createStringToken(const char *tkn);

    std::shared_ptr<ElectraLite::MidiDevice> device_;
    std::shared_ptr<ElectraLite::MidiCallback> midiCallback_;
    bool active_;


    static constexpr int OUTPUT_MAX_SZ = 100;
    SysExStream sysExStream_;

    SysExStream stringStream_;


    unsigned pollCount_;
    unsigned pollFreq_;
    unsigned pollSleep_;

    unsigned lastToken_ = 0;
    std::map<std::string, unsigned> strToToken_;
    std::map<unsigned, std::string> tokenToString_;

    ICallback &callback_;
};;


}
