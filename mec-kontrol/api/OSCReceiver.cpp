#include "OSCReceiver.h"

#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>

#include <mec_log.h>


namespace Kontrol {

class KontrolPacketListener : public PacketListener {
public:
    KontrolPacketListener(moodycamel::ReaderWriterQueue<OSCReceiver::OscMsg>& queue) : queue_(queue) {
    }

    virtual void ProcessPacket(const char *data, int size,
                               const IpEndpointName &remoteEndpoint) {

//        {
//            static unsigned maxsize = 0;
//            if(size>maxsize) {
//                maxsize = size;
//                LOG_0("KontrolPacketListener MAXSIZE " << maxsize);
//            }
//        }
        OSCReceiver::OscMsg msg;
        msg.origin_ = remoteEndpoint;
        msg.size_ = (size > OSCReceiver::OscMsg::MAX_OSC_MESSAGE_SIZE ? OSCReceiver::OscMsg::MAX_OSC_MESSAGE_SIZE
                                                                      : size);
        memcpy(msg.buffer_, data, (size_t) msg.size_);
        queue_.enqueue(msg);
    }

private:
    moodycamel::ReaderWriterQueue<OSCReceiver::OscMsg>& queue_;
};


class KontrolOSCListener : public osc::OscPacketListener {
public:
    KontrolOSCListener(OSCReceiver &recv) : receiver_(recv) { ; }


    virtual void ProcessMessage(const osc::ReceivedMessage &m,
                                const IpEndpointName &remoteEndpoint) {
        try {
            char host[IpEndpointName::ADDRESS_STRING_LENGTH];
            remoteEndpoint.AddressAsString(host);
            ChangeSource changedSrc = ChangeSource::createRemoteSource(host, remoteEndpoint.port);
            // std::err << "received osc message: " << m.AddressPattern() << std::endl;
            if (std::strcmp(m.AddressPattern(), "/Kontrol/changed") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *paramId = (arg++)->AsString();
                if (arg != m.ArgumentsEnd()) {
                    if (arg->IsString()) {
                        receiver_.changeParam(changedSrc, rackId, moduleId, paramId,
                                              ParamValue(std::string(arg->AsString())));

                    } else if (arg->IsFloat()) {
//                        std::cerr << "changed " << paramId << " : " << arg->AsFloat() << std::endl;
                        receiver_.changeParam(changedSrc, rackId, moduleId, paramId, ParamValue(arg->AsFloat()));
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

                receiver_.createParam(changedSrc, rackId, moduleId, params);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/page") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                // std::cerr << "received page p1"<< std::endl;
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *pageId = (arg++)->AsString();

                const char *displayName = (arg++)->AsString();

                std::vector<EntityId> paramIds;
                while (arg != m.ArgumentsEnd()) {
                    paramIds.push_back((arg++)->AsString());
                }

                // std::cout << "received page " << id << std::endl;
                receiver_.createPage(changedSrc, rackId, moduleId, pageId, displayName, paramIds);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/module") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *displayName = (arg++)->AsString();
                const char *type = (arg++)->AsString();

//                 std::cout << "received module " << moduleId << std::endl;
                receiver_.createModule(changedSrc, rackId, moduleId, displayName, type);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/rack") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *host = (arg++)->AsString();
                unsigned port = (unsigned) (arg++)->AsInt32();

                // std::cout << "received rack " << rackId << std::endl;
                receiver_.createRack(changedSrc, rackId, host, port);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/ping") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                unsigned port = (unsigned) (arg++)->AsInt32();
                unsigned keepAlive = 0;
                if (arg != m.ArgumentsEnd()) {
                    keepAlive = (unsigned) (arg++)->AsInt32();
                }
                receiver_.ping(changedSrc, std::string(host), port, keepAlive);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/activeModule") == 0) {
                    osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                    const char *rackId = (arg++)->AsString();
                    const char *moduleId = (arg++)->AsString();

                    receiver_.activeModule(changedSrc, rackId, moduleId);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/resource") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
//                 std::cout << "received resource p1"<< std::endl;
                const char *rackId = (arg++)->AsString();
                const char *resType = (arg++)->AsString();
                const char *resValue = (arg++)->AsString();

//                 std::cout << "received resource " << rackId <<  " : " << resType << " : " << resValue << std::endl;
                receiver_.createResource(changedSrc, rackId, resType, resValue);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/deleteRack") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();

                receiver_.deleteRack(changedSrc, rackId);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/assignMidiCC") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *paramId = (arg++)->AsString();
                unsigned midiCC = (unsigned) (arg++)->AsInt32();
                receiver_.assignMidiCC(changedSrc, rackId, moduleId, paramId, midiCC);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/unassignMidiCC") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *paramId = (arg++)->AsString();
                unsigned midiCC = (unsigned) (arg++)->AsInt32();
                receiver_.unassignMidiCC(changedSrc, rackId, moduleId, paramId, midiCC);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/assignModulation") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *paramId = (arg++)->AsString();
                unsigned bus = (unsigned) (arg++)->AsInt32();
                receiver_.assignModulation(changedSrc, rackId, moduleId, paramId, bus);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/unassignModulation") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *moduleId = (arg++)->AsString();
                const char *paramId = (arg++)->AsString();
                unsigned bus = (unsigned) (arg++)->AsInt32();
                receiver_.unassignModulation(changedSrc, rackId, moduleId, paramId, bus);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/publishStart") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                auto numRacks = (unsigned)arg->AsInt32();
                receiver_.publishStart(changedSrc, numRacks);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/publishRackFinished") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = arg->AsString();
                receiver_.publishRackFinished(changedSrc, rackId);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/updatePreset") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *preset = (arg++)->AsString();
                receiver_.updatePreset(changedSrc, rackId, preset);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/applyPreset") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *preset = (arg++)->AsString();
                receiver_.applyPreset(changedSrc, rackId, preset);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/saveSettings") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                receiver_.saveSettings(changedSrc, rackId);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/loadModule") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                const char *rackId = (arg++)->AsString();
                const char *modId = (arg++)->AsString();
                const char *modType = (arg++)->AsString();
                receiver_.loadModule(changedSrc, rackId, modId, modType);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/midiLearn") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                bool b = (arg++)->AsBool();
                receiver_.midiLearn(changedSrc, b);
            } else if (std::strcmp(m.AddressPattern(), "/Kontrol/modulationLearn") == 0) {
                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                bool b = (arg++)->AsBool();
                receiver_.modulationLearn(changedSrc, b);
            }
        } catch (osc::Exception &e) {
            // std::err << "error while parsing message: "
            //     << m.AddressPattern() << ": " << e.what() << "\n";
        }
    }

private:
    OSCReceiver &receiver_;
};

OSCReceiver::OSCReceiver(const std::shared_ptr<KontrolModel> &param)
        : model_(param), port_(0), messageQueue_(OscMsg::MAX_N_OSC_MSGS) {
    packetListener_ = std::make_shared<KontrolPacketListener>(messageQueue_);
    oscListener_ = std::make_shared<KontrolOSCListener>(*this);
}

OSCReceiver::~OSCReceiver() {
    stop();
}

void *osc_receiver_read_thread_func(void *pReceiver) {
    auto *pThis = static_cast<OSCReceiver *>(pReceiver);
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

        receive_thread_ = std::thread(osc_receiver_read_thread_func, this);
    } catch (const std::runtime_error &e) {
        return false;
    }
    return true;
}

void OSCReceiver::stop() {
    if (socket_) {
        socket_->AsynchronousBreak();
        receive_thread_.join();
        OscMsg msg;
        while(messageQueue_.try_dequeue(msg));
    }
    port_ = 0;
    socket_.reset();
}

void OSCReceiver::poll() {
    OscMsg msg;
    while (messageQueue_.try_dequeue(msg)) {
        oscListener_->ProcessPacket(msg.buffer_, msg.size_, msg.origin_);
    }
}

void OSCReceiver::createRack(
        ChangeSource src,
        const EntityId &rackId,
        const std::string &host,
        unsigned port) const {
    model_->createRack(src, rackId, host, port);
}

void OSCReceiver::createModule(
        ChangeSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const std::string &displayName,
        const std::string &type
) const {
    model_->createModule(src, rackId, moduleId, displayName, type);
}


void OSCReceiver::createParam(
        ChangeSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const std::vector<ParamValue> &args) const {
    model_->createParam(src, rackId, moduleId, args);
}

void OSCReceiver::changeParam(
        ChangeSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const EntityId &paramId,
        ParamValue f) const {
    model_->changeParam(src, rackId, moduleId, paramId, f);
}

void OSCReceiver::createPage(
        ChangeSource src,
        const EntityId &rackId,
        const EntityId &moduleId,
        const EntityId &pageId,
        const std::string &displayName,
        const std::vector<EntityId> paramIds
) const {
    model_->createPage(src, rackId, moduleId, pageId, displayName, paramIds);
}


void OSCReceiver::createResource(ChangeSource src,
                                 const EntityId &rackId,
                                 const std::string &resType,
                                 const std::string &resValue) const {
    model_->createResource(src, rackId, resType, resValue);
}

void OSCReceiver::deleteRack(ChangeSource src, const EntityId & rackId)
{
	model_->deleteRack(src, rackId);
}

void OSCReceiver::ping(ChangeSource src,
                       const std::string &host,
                       unsigned port,
                       unsigned keepalive) {
    model_->ping(src, host, port, keepalive);
}

void OSCReceiver::activeModule(ChangeSource src,
                    const EntityId &rackId,
                    const EntityId &moduleId) {
    model_->activeModule(src,rackId,moduleId);
}

void OSCReceiver::assignMidiCC(ChangeSource src, const EntityId &rackId, const EntityId &moduleId,
                               const EntityId &paramId, unsigned midiCC) {
    model_->assignMidiCC(src, rackId, moduleId, paramId, midiCC);
}

void OSCReceiver::unassignMidiCC(ChangeSource src, const EntityId &rackId, const EntityId &moduleId,
                                 const EntityId &paramId, unsigned midiCC) {
    model_->unassignMidiCC(src, rackId, moduleId, paramId, midiCC);
}


void OSCReceiver::assignModulation(ChangeSource src, const EntityId &rackId, const EntityId &moduleId,
                               const EntityId &paramId, unsigned bus) {
    model_->assignModulation(src, rackId, moduleId, paramId, bus);
}

void OSCReceiver::unassignModulation(ChangeSource src, const EntityId &rackId, const EntityId &moduleId,
                                 const EntityId &paramId, unsigned bus) {
    model_->unassignModulation(src, rackId, moduleId, paramId, bus);
}

void OSCReceiver::publishStart(ChangeSource src, unsigned numRacks) {
    model_->publishStart(src, numRacks);
}

void OSCReceiver::publishRackFinished(ChangeSource src, const EntityId & rackId) {
    model_->publishRackFinished(src, rackId);
}

void OSCReceiver::updatePreset(ChangeSource src, const EntityId &rackId, std::string preset) {
    model_->updatePreset(src, rackId, preset);
}

void OSCReceiver::applyPreset(ChangeSource src, const EntityId &rackId, std::string preset) {
    model_->applyPreset(src, rackId, preset);
}

void OSCReceiver::saveSettings(ChangeSource src, const EntityId &rackId) {
    model_->saveSettings(src, rackId);
}


void OSCReceiver::loadModule(ChangeSource src,
                             const EntityId &rackId,
                             const EntityId &moduleId,
                             const std::string &moduleType) {
    model_->loadModule(src, rackId, moduleId, moduleType);
}

void OSCReceiver::midiLearn(ChangeSource src,bool b) {
    model_->midiLearn(src,b);
}

void OSCReceiver::modulationLearn(ChangeSource src,bool b) {
    model_->modulationLearn(src,b);
}


} // namespace