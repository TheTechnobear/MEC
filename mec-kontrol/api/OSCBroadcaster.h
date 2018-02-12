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
    static const std::string ADDRESS;

    OSCBroadcaster(Kontrol::ChangeSource src, bool master);
    ~OSCBroadcaster();
    bool connect(const std::string &host = ADDRESS, unsigned port = 9001);
    void stop();

    void sendPing(unsigned port);

    // KontrolCallback
    virtual void ping(ChangeSource src, unsigned port, const std::string &host);
    virtual void rack(ChangeSource, const Rack &);
    virtual void module(ChangeSource, const Rack &rack, const Module &);
    virtual void page(ChangeSource src, const Rack &rack, const Module &module, const Page &p);
    virtual void param(ChangeSource src, const Rack &rack, const Module &module, const Parameter &);
    virtual void changed(ChangeSource src, const Rack &rack, const Module &module, const Parameter &p);

    virtual bool isThisHost(const std::string &host, unsigned port) { return host==host_ && port==port_;}
    bool isActive();
    void writePoll();

protected:
    void send(const char* data, unsigned size);
    bool broadcastChange(ChangeSource src);

private:
    struct OscMsg {
        static const int MAX_N_OSC_MSGS = 64;
        static const int MAX_OSC_MESSAGE_SIZE = 512;
        int size_;
        char buffer_[MAX_OSC_MESSAGE_SIZE];
    };

    std::string host_;
    unsigned int port_;
    std::shared_ptr<UdpTransmitSocket> socket_;
    char buffer_[OUTPUT_BUFFER_SIZE];
    std::chrono::steady_clock::time_point lastPing_;

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
