#pragma once

#include "KontrolModel.h"
#include <thread>
#include <memory>

#include <ip/UdpSocket.h>
#include <pa_ringbuffer.h>

namespace Kontrol {


class KontrolOSCListener;

class KontrolPacketListener;


class OSCReceiver {
public:
    OSCReceiver(const std::shared_ptr<KontrolModel> &param);
    ~OSCReceiver();
    bool listen(unsigned port = 9000);
    void poll();

    void stop();

    void createRack(
            ChangeSource src,
            const EntityId &rackId,
            const std::string &host,
            unsigned port
    ) const;

    void createModule(
            ChangeSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const std::string &displayName,
            const std::string &type
    ) const;

    void createParam(
            ChangeSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const std::vector<ParamValue> &args
    ) const;

    void createPage(
            ChangeSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const EntityId &pageId,
            const std::string &displayName,
            const std::vector<EntityId> paramIds
    ) const;

    void changeParam(
            ChangeSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const EntityId &paramId,
            ParamValue v) const;

    void createResource(ChangeSource src,
                        const EntityId &rackId,
                        const std::string &resType,
                        const std::string &resValue) const;

    void deleteRack(ChangeSource src,
                    const EntityId &rackId);

    void ping(ChangeSource src, const std::string &host, unsigned port, unsigned keepalive);

    void assignMidiCC(ChangeSource src,
                      const EntityId &rackId,
                      const EntityId &moduleId,
                      const EntityId &paramId,
                      unsigned midiCC);
    void unassignMidiCC(ChangeSource src,
                        const EntityId &rackId,
                        const EntityId &moduleId,
                        const EntityId &paramId,
                        unsigned midiCC);
    void updatePreset(ChangeSource src,
                      const EntityId &rackId,
                      std::string preset);
    void applyPreset(ChangeSource src,
                     const EntityId &rackId,
                     std::string preset);
    void saveSettings(ChangeSource src,
                      const EntityId &rackId);
    void loadModule(ChangeSource src,
                    const EntityId &rackId,
                    const EntityId &moduleId,
                    const std::string &moduleType);

    unsigned int port() { return port_; }

    std::shared_ptr<UdpListeningReceiveSocket> socket() { return socket_; }

private:
    friend class KontrolPacketListener;

    struct OscMsg {
        static const int MAX_N_OSC_MSGS = 128;
        static const int MAX_OSC_MESSAGE_SIZE = 512;
        IpEndpointName origin_;
        int size_;
        char buffer_[MAX_OSC_MESSAGE_SIZE];
    };

    std::shared_ptr<KontrolModel> model_;
    unsigned int port_;
    std::thread receive_thread_;
    std::shared_ptr<UdpListeningReceiveSocket> socket_;
    std::shared_ptr<PacketListener> packetListener_;
    std::shared_ptr<KontrolOSCListener> oscListener_;
    PaUtilRingBuffer messageQueue_;
    char msgData_[sizeof(OscMsg) * OscMsg::MAX_N_OSC_MSGS];
};

} //namespace