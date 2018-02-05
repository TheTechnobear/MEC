#pragma once

#include "KontrolModel.h"


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

    OSCBroadcaster(bool master);
    ~OSCBroadcaster();
    bool connect(const std::string &host = ADDRESS, unsigned port = 9001);
    void stop();

    void sendPing(unsigned port);

    // KontrolCallback
    virtual void ping(const std::string &host, unsigned port);
    virtual void rack(ParameterSource, const Rack &);
    virtual void module(ParameterSource, const Rack &rack, const Module &);
    virtual void page(ParameterSource src, const Rack &rack, const Module &module, const Page &p);
    virtual void param(ParameterSource src, const Rack &rack, const Module &module, const Parameter &);
    virtual void changed(ParameterSource src, const Rack &rack, const Module &module, const Parameter &p);

    virtual bool isThisHost(const std::string &host, unsigned port) { return host==host_ && port==port_;}
    bool isActive();
    void writePoll();

protected:
    void send(const char* data, unsigned size);
    bool broadcastChange(ParameterSource src);

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
};

} //namespace
