#include "OSCBroadcaster.h"

#include "osc/OscOutboundPacketStream.h"

namespace oKontrol {


const std::string OSCBroadcaster::ADDRESS = "127.0.0.1";


OSCBroadcaster::OSCBroadcaster() : port_(0) {
}

OSCBroadcaster::~OSCBroadcaster() {
    stop();
}

bool OSCBroadcaster::connect(const std::string& host, unsigned port) {
    stop();
    try {
        port_ = port;
        socket_ =  std::shared_ptr<UdpTransmitSocket> ( new UdpTransmitSocket( IpEndpointName( host.c_str(), port_ )));
    } catch (const std::runtime_error& e) {
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

void OSCBroadcaster::requestMetaData() {
    if (!socket_) return;
    osc::OutboundPacketStream ops( buffer_, OUTPUT_BUFFER_SIZE );

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage( "/oKontrol/metaData" )
        << osc::EndMessage
        << osc::EndBundle;

    socket_->Send( ops.Data(), ops.Size() );

}



void OSCBroadcaster::page(ParameterSource src, const Page& p) {
    if (!socket_) return;
    if (src != PS_LOCAL) return;

    osc::OutboundPacketStream ops( buffer_, OUTPUT_BUFFER_SIZE );

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage( "/oKontrol/page" )
        << p.id().c_str()
        << p.displayName().c_str();

    for (const std::string& paramId : p.paramIds()) {
        ops << paramId.c_str();
    }

    ops << osc::EndMessage
        << osc::EndBundle;

    socket_->Send( ops.Data(), ops.Size() );
}

void OSCBroadcaster::param(ParameterSource src, const Parameter& p) {
    if (!socket_) return;
    if (src != PS_LOCAL) return;

    osc::OutboundPacketStream ops( buffer_, OUTPUT_BUFFER_SIZE );

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage( "/oKontrol/param" );
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

    ops  << osc::EndMessage
         << osc::EndBundle;

    socket_->Send( ops.Data(), ops.Size() );
}

void OSCBroadcaster::changed(ParameterSource src, const Parameter& p) {
    if (!socket_) return;
    if (src != PS_LOCAL) return;

    osc::OutboundPacketStream ops( buffer_, OUTPUT_BUFFER_SIZE );

    ops << osc::BeginBundleImmediate
        << osc::BeginMessage( "/oKontrol/changed" )
        << p.id().c_str();

    switch (p.current().type()) {
    case ParamValue::T_Float:
        ops << p.current().floatValue();
        break;
    case ParamValue::T_String:
    default:
        ops << p.current().stringValue().c_str();

    }

    ops    << osc::EndMessage
           << osc::EndBundle;

    socket_->Send( ops.Data(), ops.Size() );
}

} // namespace