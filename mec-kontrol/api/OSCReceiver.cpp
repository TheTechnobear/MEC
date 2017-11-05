#include "OSCReceiver.h"

#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>

#include <memory.h>
#include <mec_log.h>

#include"Rack.h"

namespace Kontrol {

class KontrolPacketListener : public PacketListener {
public:
    KontrolPacketListener(PaUtilRingBuffer *queue) : queue_(queue) {
    }

    virtual void ProcessPacket(const char *data, int size,
                               const IpEndpointName &remoteEndpoint) {
        OscMsg msg;
        msg.origin_ = remoteEndpoint;
        msg.size_ = size > MAX_OSC_MESSAGE_SIZE ? MAX_OSC_MESSAGE_SIZE : size;
        memcpy(msg.buffer_, data, msg.size_);
        // this does a memcpy so safe!
        PaUtil_WriteRingBuffer(queue_, (void *) &msg, 1);
    }

private:
    PaUtilRingBuffer *queue_;
};


class KontrolOSCListener : public osc::OscPacketListener {
public:
    KontrolOSCListener(const OSCReceiver &recv) : receiver_(recv) { ; }


    virtual void ProcessMessage(const osc::ReceivedMessage &m,
                                const IpEndpointName &remoteEndpoint) {
        (void) remoteEndpoint; // suppress unused parameter warning

        try {
            // std::cout << "recieved osc message: " << m.AddressPattern() << std::endl;
            if (std::strcmp(m.AddressPattern(), "/Kontrol/changed") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *paramId = (arg++)->AsString();
                if (arg != m.ArgumentsEnd()) {
                    if (arg->IsString()) {
                        receiver_.changeParam(PS_OSC, rackId, moduleId, paramId,
                                              ParamValue(std::string(arg->AsString())));

                    } else if (arg->IsFloat()) {
                        receiver_.changeParam(PS_OSC, rackId, moduleId, paramId, ParamValue(arg->AsFloat()));
                    }
                }

            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/param") == 0) {
                std::vector<ParamValue> params;
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                while (arg != m.ArgumentsEnd()) {
                    if (arg->IsString()) {
                        params.push_back(ParamValue(std::string(arg->AsString())));

                    } else if (arg->IsFloat()) {
                        params.push_back(ParamValue(arg->AsFloat()));
                    }
                    arg++;
                }

                receiver_.createParam(PS_OSC, rackId, moduleId, params);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/page") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                // std::cout << "recieved page p1"<< std::endl;
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *pageId = (arg++)->AsString();

                const char *displayName = (arg++)->AsString();

                std::vector<EntityId> paramIds;
                while (arg != m.ArgumentsEnd()) {
                    paramIds.push_back((arg++)->AsString());
                }

                // std::cout << "recieved page " << id << std::endl;
                receiver_.createPage(PS_OSC, rackId, moduleId, pageId, displayName, paramIds);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/module") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                // std::cout << "recieved page p1"<< std::endl;
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *displayName = (arg++)->AsString();
                const char *type = (arg++)->AsString();

                // std::cout << "recieved module " << moduleId << std::endl;
                receiver_.createModule(PS_OSC, rackId, moduleId, displayName, type);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/rack") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                // std::cout << "recieved page p1"<< std::endl;
                const char *rackId = (arg++)->AsString();
                const char *host = (arg++)->AsString();
                unsigned port = (arg++)->AsInt32();

                // std::cout << "recieved module " << moduleId << std::endl;
                receiver_.createRack(PS_OSC, rackId, host, port);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/connect") == 0) {
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                osc::int32 port;
                args >> port >> osc::EndMessage;
                char buf[IpEndpointName::ADDRESS_STRING_LENGTH];
                remoteEndpoint.AddressAsString(buf);

                receiver_.createRack(PS_OSC, Rack::createId(buf, port), buf, port);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/metaData") == 0) {
                receiver_.publishMetaData();
            }
        } catch (osc::Exception &e) {
            // std::err << "error while parsing message: "
            //     << m.AddressPattern() << ": " << e.what() << "\n";
        }
    }

private:
    const OSCReceiver &receiver_;
};

OSCReceiver::OSCReceiver(const std::shared_ptr<KontrolModel> &param)
        : model_(param), port_(0) {
    PaUtil_InitializeRingBuffer(&messageQueue_, sizeof(OscMsg), MAX_N_OSC_MSGS, msgData_);
    packetListener_ = std::make_shared<KontrolPacketListener>(&messageQueue_);
    oscListener_ = std::make_shared<KontrolOSCListener>(*this);
}

OSCReceiver::~OSCReceiver() {
    stop();
}

void *thread_func(void *pReceiver) {
    OSCReceiver *pThis = static_cast<OSCReceiver *>(pReceiver);
    pThis->socket()->Run();
    return nullptr;
}

bool OSCReceiver::listen(unsigned port) {
    stop();
    port_ = port;
    try {
        socket_ = std::make_shared<UdpListeningReceiveSocket>(
                IpEndpointName(IpEndpointName::ANY_ADDRESS, port_),
                packetListener_.get());

        thread_create(&receive_thread_, thread_func, this);
    } catch (const std::runtime_error &e) {
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
    while (PaUtil_GetRingBufferReadAvailable(&messageQueue_)) {
        OscMsg msg;
        PaUtil_ReadRingBuffer(&messageQueue_, &msg, 1);
        oscListener_->ProcessPacket(msg.buffer_, msg.size_, msg.origin_);
    }
}

void OSCReceiver::createRack(
        ParameterSource src,
        const EntityId &rackId,
        const std::string &host,
        unsigned port) const {
    model_->createRack(src, rackId, host, port);
}

void OSCReceiver::createModule(
        ParameterSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const std::string &displayName,
        const std::string &type
) const {
    model_->createModule(src, rackId, moduleId, displayName, type);
}


void OSCReceiver::createParam(
        ParameterSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const std::vector<ParamValue> &args) const {
    model_->createParam(src, rackId, moduleId, args);
}

void OSCReceiver::changeParam(
        ParameterSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const EntityId &paramId,
        ParamValue f) const {
    model_->changeParam(src, rackId, moduleId, paramId, f);
}

void OSCReceiver::createPage(
        ParameterSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const EntityId &pageId,
        const std::string &displayName,
        const std::vector<EntityId> paramIds
) const {
    model_->createPage(src, rackId, moduleId, pageId, displayName, paramIds);
}


void OSCReceiver::publishMetaData() const {
    model_->publishMetaData();
}


} // namespace