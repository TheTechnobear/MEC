#pragma once

#include "KontrolDevice.h"

#include <KontrolModel.h>

#include <ip/UdpSocket.h>

#include <string>
#include <pa_ringbuffer.h>
#include <thread>
#include <mutex>
#include <condition_variable>

class Organelle : public KontrolDevice {
public:
    Organelle();
    virtual ~Organelle();

    //KontrolDevice
    virtual bool init() override;

    void displayPopup(const std::string &text);
    void displayParamLine(unsigned line, const Kontrol::Parameter &p);
    void displayLine(unsigned line, const char *);
    void invertLine(unsigned line);
    void clearDisplay();
    void flipDisplay();

    void midiCC(unsigned num, unsigned value) override;
    void midiLearn(bool b);

    bool midiLearn() { return midiLearnActive_; }

    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;

    Kontrol::EntityId currentRack() { return currentRackId_; }

    void currentRack(const Kontrol::EntityId &p) { currentRackId_ = p; }

    Kontrol::EntityId currentModule() { return currentModuleId_; }

    void currentModule(const Kontrol::EntityId &p) { currentModuleId_ = p; }

    void rack(Kontrol::ParameterSource source, const Kontrol::Rack &rack) override;
    void module(Kontrol::ParameterSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) override;

    void writePoll();

private:
    void send(const char *data, unsigned size);
    void stop() override ;

    struct OscMsg {
        static const int MAX_N_OSC_MSGS = 64;
        static const int MAX_OSC_MESSAGE_SIZE = 128;
        int size_;
        char buffer_[MAX_OSC_MESSAGE_SIZE];
    };


    bool connect();
    Kontrol::EntityId currentRackId_;
    Kontrol::EntityId currentModuleId_;
    Kontrol::EntityId lastParamId_; // for midi learn
    bool midiLearnActive_;

    std::string asDisplayString(const Kontrol::Parameter &p, unsigned width) const;
    std::shared_ptr<UdpTransmitSocket> socket_;

    PaUtilRingBuffer messageQueue_;
    char msgData_[sizeof(OscMsg) * OscMsg::MAX_N_OSC_MSGS];
    bool running_;
    std::mutex write_lock_;
    std::condition_variable write_cond_;
    std::thread writer_thread_;
};
