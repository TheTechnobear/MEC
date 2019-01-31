#include "OSCBroadcaster.h"

#include <osc/OscOutboundPacketStream.h>
#include <mec_log.h>

namespace Kontrol {


//const std::string OSCBroadcaster::ADDRESS = "127.0.0.1";

#define POLL_TIMEOUT_MS 1000

OSCBroadcaster::OSCBroadcaster(Kontrol::ChangeSource src, unsigned keepAlive, bool master) :
        master_(master),
        port_(0),
        changeSource_(src),
        keepAliveTime_(keepAlive),
        messageQueue_(OscMsg::MAX_N_OSC_MSGS) {
}

OSCBroadcaster::~OSCBroadcaster() {
    stop();
}


void *osc_broadcaster_write_thread_func(void *pBroadcaster) {
    OSCBroadcaster *pThis = static_cast<OSCBroadcaster *>(pBroadcaster);
    pThis->writePoll();
    return nullptr;
}

bool OSCBroadcaster::connect(const std::string &host, unsigned port) {
    stop();
    try {
        host_ = host;
        port_ = port;
        socket_ = std::shared_ptr<UdpTransmitSocket>(new UdpTransmitSocket(IpEndpointName(host.c_str(), port_)));
    } catch (const std::runtime_error &e) {
        port_ = 0;
        socket_.reset();
        return false;
    }
    running_ = true;
#ifdef __COBALT__
    pthread_t ph = writer_thread_.native_handle();
    pthread_create(&ph, 0,osc_broadcaster_write_thread_func,this);
#else
    writer_thread_ = std::thread(osc_broadcaster_write_thread_func, this);
#endif
    return true;
}

void OSCBroadcaster::stop() {
    running_ = false;
    if (socket_) {
        writer_thread_.join();
        flush();
    }
    port_ = 0;
    socket_.reset();
}


void OSCBroadcaster::writePoll() {
    while (running_) {
        OscMsg msg;
        if (messageQueue_.wait_dequeue_timed(msg, std::chrono::milliseconds(POLL_TIMEOUT_MS))) {
            socket_->Send(msg.buffer_, (size_t) msg.size_);
        }
    }
}

void OSCBroadcaster::flush() {
    OscMsg msg;
    while (messageQueue_.try_dequeue(msg)) {
        socket_->Send(msg.buffer_, (size_t) msg.size_);
    }
}

bool OSCBroadcaster::isActive() {
    if (!socket_) return false;
    if (keepAliveTime_ == 0) return true;
#ifdef __COBALT__
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    int timeOut = keepAliveTime_ * 2 ;
    int dur = now.tv_sec - lastPing_.tv_sec;
    //std::cerr << "isActive " << dur << " <= " << timeOut  << " e= " << bool(dur <= timeOut) << std::endl;
    //std::cerr << "isActive time " << now.tv_sec <<  " > " << lastPing_.tv_sec  << std::endl;
#else
    static std::chrono::seconds timeOut(keepAliveTime_ * 2); // twice normal ping time
    auto now = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastPing_);
//    if(!(dur <= timeOut)) { std::cerr << "not active : " << dur.count() << std::endl; 
#endif
    return dur <= timeOut;
}


void OSCBroadcaster::send(const char *data, unsigned size) {
//    {
//        static unsigned maxsize = 0;
//        if(size>maxsize) {
//            maxsize = size;
//            LOG_0("OSCBroadcaster MAXSIZE " << maxsize);
//        }
//    }
    OscMsg msg;
    msg.size_ = (size > OscMsg::MAX_OSC_MESSAGE_SIZE ? OscMsg::MAX_OSC_MESSAGE_SIZE : size);
    memcpy(msg.buffer_, data, (size_t) msg.size_);
    messageQueue_.enqueue(msg);
}

void OSCBroadcaster::sendPing(unsigned port) {
    if (!socket_) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/ping")
        << (int32_t) port
        << (int32_t) keepAliveTime_
        << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

bool OSCBroadcaster::broadcastChange(ChangeSource src) {
    return src != changeSource_;
}


void OSCBroadcaster::ping(ChangeSource src, const std::string &host, unsigned port, unsigned keepAlive) {
    if ((port_ == port) && (host_ == host)) {
        changeSource_ = src;

        keepAliveTime_ = keepAlive;
        bool wasActive = isActive();
#ifdef __COBALT__
        clock_gettime(CLOCK_REALTIME, &lastPing_);
#else
        lastPing_ = std::chrono::steady_clock::now();
#endif
        if (!master_) {
            if (!wasActive) {
		// std::cerr << " !master : publishing meta data, from " << host  << ":" << port << std::endl;
                KontrolModel::model()->publishMetaData();
            }
        } else {
            if (keepAliveTime_ == 0 || !wasActive) {

                EntityId rackId = Rack::createId(host_, port_);
                publishStart(CS_LOCAL, KontrolModel::model()->getRacks().size());
                for (const auto &r:KontrolModel::model()->getRacks()) {
                    if (rackId != r->id()) {
                        std::cerr << " publishing meta data to " << rackId << " for " << r->id() << std::endl;
                        rack(CS_LOCAL, *r);
                        for (const auto &m : r->getModules()) {
                            module(CS_LOCAL, *r, *m);
                            for (const auto &p :  m->getParams()) {
                                param(CS_LOCAL, *r, *m, *p);
                            }
                            for (const auto &p : m->getPages()) {
                                if (p != nullptr) {
                                    page(CS_LOCAL, *r, *m, *p);
                                }
                            }
                            for (const auto &p :  m->getParams()) {
                                changed(CS_LOCAL, *r, *m, *p);
                            }
                            for (const auto &midiMap : m->getMidiMapping()) {
                                for (const auto &j : midiMap.second) {
                                    auto parameter = m->getParam(j);
                                    if (parameter) {
                                        assignMidiCC(CS_LOCAL, *r, *m, *parameter, midiMap.first);
                                    }
                                }
                            }
                        }
                        publishRackFinished(CS_LOCAL, *r);
                    }
                }
            }
        }
    }
}

void OSCBroadcaster::assignMidiCC(ChangeSource src, const Rack &rack, const Module &module, const Parameter &p,
                                  unsigned midiCC) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/assignMidiCC")
        << rack.id().c_str()
        << module.id().c_str()
        << p.id().c_str()
        << (int32_t) midiCC;

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::unassignMidiCC(ChangeSource src, const Rack &rack, const Module &module, const Parameter &p,
                                    unsigned midiCC) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/unassignMidiCC")
        << rack.id().c_str()
        << module.id().c_str()
        << p.id().c_str()
        << (int32_t) midiCC;

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}


void OSCBroadcaster::assignModulation(ChangeSource src, const Rack &rack, const Module &module, const Parameter &p,
                                      unsigned bus) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/assignModulation")
        << rack.id().c_str()
        << module.id().c_str()
        << p.id().c_str()
        << (int32_t) bus;

    ops << osc::EndMessage
        << osc::EndBundle;
}

void OSCBroadcaster::unassignModulation(ChangeSource src, const Rack &rack, const Module &module, const Parameter &p,
                                        unsigned bus) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/unassignModulation")
        << rack.id().c_str()
        << module.id().c_str()
        << p.id().c_str()
        << (int32_t) bus;

    ops << osc::EndMessage
        << osc::EndBundle;

}

void OSCBroadcaster::publishStart(ChangeSource src, unsigned numRacks)
{
    if (numRacks == 0)
        return;
    if (!broadcastChange(src))
        return;
    if (!isActive())
        return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/publishStart")
        << (int32_t)numRacks
        << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::publishRackFinished(ChangeSource src, const Rack &rack)
{
    if (!broadcastChange(src))
        return;
    if (!isActive())
        return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/publishRackFinished")
        << rack.id().c_str()
        << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::updatePreset(ChangeSource src, const Rack &rack, std::string preset) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/updatePreset")
        << rack.id().c_str()
        << preset.c_str();

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::applyPreset(ChangeSource src, const Rack &rack, std::string preset) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;
    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/applyPreset")
        << rack.id().c_str()
        << preset.c_str();

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::saveSettings(ChangeSource src, const Rack &rack) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/saveSettings")
        << rack.id().c_str();

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}


void OSCBroadcaster::loadModule(ChangeSource src, const Rack &rack, const EntityId &moduleId,
                                const std::string &modType) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);
    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/loadModule")
        << rack.id().c_str()
        << moduleId.c_str()
        << modType.c_str();

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}


void OSCBroadcaster::rack(ChangeSource src, const Rack &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/rack")
        << p.id().c_str()
        << p.host().c_str()
        << (int32_t) p.port();

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}


void OSCBroadcaster::module(ChangeSource src, const Rack &rack, const Module &m) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

//    LOG_0("OSCBroadcaster::module " << m.id());

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/module")
        << rack.id().c_str()
        << m.id().c_str()
        << m.displayName().c_str()
        << m.type().c_str();

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}


void OSCBroadcaster::page(ChangeSource src, const Rack &rack, const Module &module, const Page &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/page")
        << rack.id().c_str()
        << module.id().c_str()
        << p.id().c_str()
        << p.displayName().c_str();

    for (const std::string &paramId : p.paramIds()) {
        ops << paramId.c_str();
    }

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::param(ChangeSource src, const Rack &rack, const Module &module, const Parameter &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/param")
        << rack.id().c_str()
        << module.id().c_str();

    std::vector<ParamValue> values;
    p.createArgs(values);
    for (const ParamValue &v : values) {
        switch (v.type()) {
            case ParamValue::T_Float: {
                ops << v.floatValue();
                break;
                case ParamValue::T_String:
                default:
                    ops << v.stringValue().c_str();
            }
        }
    }

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::changed(ChangeSource src, const Rack &rack, const Module &module, const Parameter &p) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/changed")
        << rack.id().c_str()
        << module.id().c_str()
        << p.id().c_str();

    switch (p.current().type()) {
        case ParamValue::T_Float:
            ops << p.current().floatValue();
            break;
        case ParamValue::T_String:
        default:
            ops << p.current().stringValue().c_str();

    }

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::resource(ChangeSource src, const Rack &rack, const std::string &type, const std::string &res) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/resource")
        << rack.id().c_str()
        << type.c_str()
        << res.c_str();


    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}

void OSCBroadcaster::deleteRack(ChangeSource src, const Rack &rack) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/deleteRack")
        << rack.id().c_str();

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}


void OSCBroadcaster::midiLearn(ChangeSource src, bool b) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/midiLearn")
        << b;

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());

}

void OSCBroadcaster::modulationLearn(ChangeSource src, bool b) {
    if (!broadcastChange(src)) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/modulationLearn")
        << b;

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());

}

void OSCBroadcaster::activeModule(ChangeSource source, const Rack &rack, const Module &module) {
    if (!broadcastChange(source)) return;
    if (!isActive()) return;

//    LOG_0("OSCBroadcaster::module " << m.id());

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/activeModule")
        << rack.id().c_str()
        << module.id().c_str();

    ops << osc::EndMessage
        << osc::EndBundle;

    send(ops.Data(), ops.Size());
}


} // namespace
