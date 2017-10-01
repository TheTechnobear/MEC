#include "OSCReceiver.h"

#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"

#include <memory.h>
#include <iostream>

namespace oKontrol {

class oKontrolPacketListener : public PacketListener {
public:
    oKontrolPacketListener(PaUtilRingBuffer* queue) : queue_(queue) {
    }
    virtual void ProcessPacket( const char *data, int size,
                                const IpEndpointName& remoteEndpoint) {
        OscMsg msg;
        msg.origin_ = remoteEndpoint;
        msg.size_ = size > MAX_OSC_MESSAGE_SIZE ? MAX_OSC_MESSAGE_SIZE : size;
        memcpy(msg.buffer_, data, msg.size_);
        // this does a memcpy so safe!
        PaUtil_WriteRingBuffer(queue_, (void*) &msg, 1);
    }
private:
    PaUtilRingBuffer* queue_;
};


class oKontrolOSCListener : public osc::OscPacketListener {
public:
    oKontrolOSCListener(const OSCReceiver& recv) : receiver_(recv) { ; }


    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                 const IpEndpointName& remoteEndpoint )
    {
        (void) remoteEndpoint; // suppress unused parameter warning

        try {
            // std::cout << "recieved osc message: " << m.AddressPattern() << std::endl;
            if ( std::strcmp( m.AddressPattern(), "/oKontrol/changed" ) == 0 ) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char* id = (arg++)->AsString();
                if ( arg != m.ArgumentsEnd() ) {
                    if (arg->IsString()) {
                        receiver_.changeParam(PS_OSC, std::string(id), ParamValue(std::string(arg->AsString())));

                    } else if (arg->IsFloat()) {
                        receiver_.changeParam(PS_OSC, std::string(id), ParamValue(arg->AsFloat()));
                    }
                }

            } else if ( std::strcmp( m.AddressPattern(), "/oKontrol/param" ) == 0 ) {
                std::vector<ParamValue> params;
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                while ( arg != m.ArgumentsEnd() ) {
                    if (arg->IsString()) {
                        params.push_back(ParamValue(std::string(arg->AsString())));

                    } else if (arg->IsFloat()) {
                        params.push_back(ParamValue(arg->AsFloat()));
                    }
                    arg++;
                }

                receiver_.addParam(PS_OSC,params);
            } else if ( std::strcmp( m.AddressPattern(), "/oKontrol/page" ) == 0 ) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                // std::cout << "recieved page p1"<< std::endl;

                const char* id = (arg++)->AsString();
                const char* displayname = (arg++)->AsString();

                std::vector<std::string> paramIds;
                while ( arg != m.ArgumentsEnd() ) {
                    paramIds.push_back((arg++)->AsString());
                }

                // std::cout << "recieved page " << id << std::endl;
                receiver_.addPage(PS_OSC, id, displayname, paramIds);
            } else if ( std::strcmp( m.AddressPattern(), "/oKontrol/connect" ) == 0 ) {
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                osc::int32 port;
                args >> port >> osc::EndMessage;
                char buf[IpEndpointName::ADDRESS_STRING_LENGTH];
                remoteEndpoint.AddressAsString(buf);

                receiver_.addClient(std::string(buf), port);
            } else if ( std::strcmp( m.AddressPattern(), "/oKontrol/metaData" ) == 0 ) {
                receiver_.publishMetaData();
            }
        } catch ( osc::Exception& e ) {
            // std::err << "error while parsing message: "
            //     << m.AddressPattern() << ": " << e.what() << "\n";
        }
    }
private:
    const OSCReceiver& receiver_;
};

OSCReceiver::OSCReceiver(const std::shared_ptr<ParameterModel>& param)
    : param_model_(param), port_(0) {
    PaUtil_InitializeRingBuffer(&messageQueue_, sizeof(OscMsg), MAX_N_OSC_MSGS, msgData_);
    packetListener_ = std::make_shared<oKontrolPacketListener> (&messageQueue_);
    oscListener_ = std::make_shared<oKontrolOSCListener> (*this);
}

OSCReceiver::~OSCReceiver() {
    stop();
}

void* thread_func(void* pReceiver) {
    OSCReceiver* pThis = static_cast<OSCReceiver*>(pReceiver);
    pThis->socket()->Run();
    return nullptr;
}

bool OSCReceiver::listen(unsigned port) {
    stop();
    port_ = port;
    try {
        socket_ = std::make_shared<UdpListeningReceiveSocket> (
                      IpEndpointName( IpEndpointName::ANY_ADDRESS, port_ ),
                      packetListener_.get() );

        thread_create(&receive_thread_, thread_func, this);
    } catch (const std::runtime_error& e) {
        return false;
    }
    return true;
}

void OSCReceiver::stop() {
    if (socket_) {
        socket_->AsynchronousBreak();
        thread_wait(receive_thread_);
        PaUtil_FlushRingBuffer(&messageQueue_);
    }
    port_ = 0;
    socket_.reset();
}

void OSCReceiver::poll() {
    while (PaUtil_GetRingBufferReadAvailable(&messageQueue_))
    {
        OscMsg msg;
        PaUtil_ReadRingBuffer(&messageQueue_, &msg, 1);
        oscListener_->ProcessPacket(msg.buffer_, msg.size_, msg.origin_);
    }
}

void OSCReceiver::addClient(const std::string& host, unsigned port) const {
    param_model_->addClient(host, port);
}

void OSCReceiver::addParam(ParameterSource src, const std::vector<ParamValue>& args) const {
    param_model_->addParam(src, args);
}


void OSCReceiver::addPage(
    ParameterSource src,
    const std::string& id,
    const std::string& displayName,
    const std::vector<std::string> paramIds
) const {
    param_model_->addPage(src, id, displayName, paramIds);
}

void OSCReceiver::changeParam(ParameterSource src, const std::string& id, ParamValue f) const {
    param_model_->changeParam(src, id, f);
}

void OSCReceiver::publishMetaData() const {
    param_model_->publishMetaData();
}


} // namespace