#pragma once

#include "ParameterModel.h"
#include <memory>

#include "ip/UdpSocket.h"

namespace oKontrol {

class OSCBroadcaster : public ParameterCallback {
public:
    static const unsigned int OUTPUT_BUFFER_SIZE = 1024;
    static const std::string ADDRESS;

    OSCBroadcaster();
    ~OSCBroadcaster();
    bool connect(const std::string& host=ADDRESS, unsigned port = 9001);
    void stop();

    void requestMetaData();

    // ParameterCallback
    virtual void addClient(const std::string&, unsigned) {;}
    virtual void page(ParameterSource src, const Page& p); 
    virtual void param(ParameterSource src, const Parameter&); 
    virtual void changed(ParameterSource src, const Parameter& p);
private:
    unsigned int port_;
    std::shared_ptr<UdpTransmitSocket> socket_;
    char buffer_[OUTPUT_BUFFER_SIZE];
};

} //namespace