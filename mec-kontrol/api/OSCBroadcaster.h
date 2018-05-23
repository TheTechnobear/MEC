#pragma once

#include "KontrolModel.h"
#include "ChangeSource.h"

#include <memory>
#include <ip/UdpSocket.h>
#include <pa_ringbuffer.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Kontrol {


class OSCBroadcaster : public KontrolCallback {
public:
    static const unsigned int OUTPUT_BUFFER_SIZE = 1024;

    OSCBroadcaster(Kontrol::ChangeSource src, unsigned keepAlive, bool master);
    ~OSCBroadcaster();
    bool connect(const std::string &host, unsigned port);
    void stop() override;

    void sendPing(unsigned port);

    // KontrolCallback
    void rack(ChangeSource, const Rack &) override;
    void module(ChangeSource, const Rack &rack, const Module &) override;
    void page(ChangeSource src, const Rack &rack, const Module &module, const Page &p) override;
    void param(ChangeSource src, const Rack &rack, const Module &module, const Parameter &) override;
    void changed(ChangeSource src, const Rack &rack, const Module &module, const Parameter &p) override;
    void resource(ChangeSource, const Rack &, const std::string &, const std::string &) override;


    void ping(ChangeSource src, const std::string &host, unsigned port, unsigned keepAlive) override;
    void assignMidiCC(ChangeSource, const Rack &, const Module &, const Parameter &, unsigned midiCC) override;
    void unassignMidiCC(ChangeSource, const Rack &, const Module &, const Parameter &, unsigned midiCC) override;
    void updatePreset(ChangeSource, const Rack &, std::string preset) override;
    void applyPreset(ChangeSource, const Rack &, std::string preset) override;
    void saveSettings(ChangeSource, const Rack &) override;
    void loadModule(ChangeSource, const Rack &, const EntityId &, const std::string &) override;

    bool isThisHost(const std::string &host, unsigned port) { return host == host_ && port == port_; }

    bool isActive();
    void writePoll();

    std::string host() { return host_; }

    unsigned port() { return port_; }


protected:
    void send(const char *data, unsigned size);
    bool broadcastChange(ChangeSource src);

private:
    struct OscMsg {
        static const int MAX_N_OSC_MSGS = 128;
        static const int MAX_OSC_MESSAGE_SIZE = 512;
        int size_;
        char buffer_[MAX_OSC_MESSAGE_SIZE];
    };

    std::string host_;
    unsigned int port_;
    std::shared_ptr<UdpTransmitSocket> socket_;
    char buffer_[OUTPUT_BUFFER_SIZE];
    std::chrono::steady_clock::time_point lastPing_;
    unsigned keepAliveTime_;

    PaUtilRingBuffer messageQueue_;
    char msgData_[sizeof(OscMsg) * OscMsg::MAX_N_OSC_MSGS];
    bool master_;

    bool running_;
    std::mutex write_lock_;
    std::condition_variable write_cond_;
    std::thread writer_thread_;
    ChangeSource changeSource_;
};

} //namespace
