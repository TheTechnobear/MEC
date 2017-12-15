#include "OSCBroadcaster.h"

#include <osc/OscOutboundPacketStream.h>

#include <mec_log.h>

namespace Kontrol {


const std::string OSCBroadcaster::ADDRESS = "127.0.0.1";


OSCBroadcaster::OSCBroadcaster(bool master) : master_(master), port_(0) {
}

OSCBroadcaster::~OSCBroadcaster() {
    stop();
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
    return true;
}

void OSCBroadcaster::stop() {
    port_ = 0;
    socket_.reset();
}


bool OSCBroadcaster::isActive() {
    if (!socket_) return false;
    static std::chrono::seconds timeOut(10); // twice normal ping time

    auto now = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastPing_);
    return dur < timeOut;
}

void OSCBroadcaster::sendPing(unsigned port) {
    if (!socket_) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/ping")
        << (int32_t) port
        << osc::EndMessage
        << osc::EndBundle;

    socket_->Send(ops.Data(), ops.Size());
}

void OSCBroadcaster::ping(const std::string &host, unsigned port) {
    if (port == port_) {
        if (!master_) {
            bool wasActive = isActive();
            lastPing_ = std::chrono::steady_clock::now();
            //TODO: not sure I want reference to KontrolModel here!
            if (!wasActive) {
                KontrolModel::model()->publishMetaData();
            }
        } else {
            lastPing_ = std::chrono::steady_clock::now();
        }
    }
}

void OSCBroadcaster::rack(ParameterSource src, const Rack &p) {
    if (src != PS_LOCAL) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/rack")
        << p.id().c_str()
        << p.host().c_str()
        << (int32_t) p.port();

    ops << osc::EndMessage
        << osc::EndBundle;

    socket_->Send(ops.Data(), ops.Size());
}


void OSCBroadcaster::module(ParameterSource src, const Rack &rack, const Module &m) {
    if (src != PS_LOCAL) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/module")
        << rack.id().c_str()
        << m.id().c_str()
        << m.displayName().c_str()
        << m.type().c_str();

    ops << osc::EndMessage
        << osc::EndBundle;

    socket_->Send(ops.Data(), ops.Size());
}


void OSCBroadcaster::page(ParameterSource src, const Rack &rack, const Module &module, const Page &p) {
    if (src != PS_LOCAL) return;
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

    socket_->Send(ops.Data(), ops.Size());
}

void OSCBroadcaster::param(ParameterSource src, const Rack &rack, const Module &module, const Parameter &p) {
    if (src != PS_LOCAL) return;
    if (!isActive()) return;

    osc::OutboundPacketStream ops(buffer_, OUTPUT_BUFFER_SIZE);

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage("/Kontrol/param")
        << rack.id().c_str()
        << module.id().c_str();

    std::vector<ParamValue> values;
    p.createArgs(values);
    for (ParamValue v : values) {
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

    socket_->Send(ops.Data(), ops.Size());
}

void OSCBroadcaster::changed(ParameterSource src, const Rack &rack, const Module &module, const Parameter &p) {
    if (src != PS_LOCAL) return;
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

    socket_->Send(ops.Data(), ops.Size());
}

} // namespace