#pragma once

#include "KontrolModel.h"


#include <memory>
#include <ip/UdpSocket.h>
#include <chrono>

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
    void sendMetaData();

    // KontrolCallback
    virtual void ping(const std::string &host, unsigned port);
    virtual void rack(ParameterSource, const Rack &);
    virtual void module(ParameterSource, const Rack &rack, const Module &);
    virtual void page(ParameterSource src, const Rack &rack, const Module &module, const Page &p);
    virtual void param(ParameterSource src, const Rack &rack, const Module &module, const Parameter &);
    virtual void changed(ParameterSource src, const Rack &rack, const Module &module, const Parameter &p);
private:
    bool isActive();

    unsigned int port_;
    std::shared_ptr<UdpTransmitSocket> socket_;
    char buffer_[OUTPUT_BUFFER_SIZE];
    std::chrono::steady_clock::time_point lastPing_;
    bool master_;
};

} //namespace