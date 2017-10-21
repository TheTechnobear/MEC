#pragma once

#include "ParameterModel.h"

#include <memory>
#include <ip/UdpSocket.h>

namespace Kontrol {

class OSCBroadcaster : public ParameterCallback {
public:
    static const unsigned int OUTPUT_BUFFER_SIZE = 1024;
    static const std::string ADDRESS;

    OSCBroadcaster();
    ~OSCBroadcaster();
    bool connect(const std::string& host = ADDRESS, unsigned port = 9001);
    void stop();

    void requestMetaData();
    void requestConnect(unsigned port);

    // ParameterCallback
    virtual void device(ParameterSource, const Device&);
    virtual void patch(ParameterSource, const Device& device, const Patch&);
    virtual void page(ParameterSource src, const Device& device, const Patch& patch, const Page& p);
    virtual void param(ParameterSource src, const Device& device, const Patch& patch, const Parameter&);
    virtual void changed(ParameterSource src, const Device& device, const Patch& patch, const Parameter& p);
private:
    unsigned int port_;
    std::shared_ptr<UdpTransmitSocket> socket_;
    char buffer_[OUTPUT_BUFFER_SIZE];
};

} //namespace