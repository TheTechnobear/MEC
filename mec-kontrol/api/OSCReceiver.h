#pragma once

#include "KontrolModel.h"
#include <memory>

#include <ip/UdpSocket.h>
#include <pa_ringbuffer.h>


#ifndef __WINDOWS__
// pthread
#include <pthread.h>

#define thread_t pthread_t
#define thread_create(thp, func, argp) { \
    bool t_error = false; \
    if (pthread_create(thp, NULL, func, (void *) argp) != 0) { \
        t_error = true; \
    } \
}
#define thread_wait(th) { \
    bool t_error = true; \
    if (pthread_join(th, NULL) != 0) { \
        t_error = true; \
    } \
}
#else
// Windows
#include <windows.h>
#define thread_t HANDLE
#define thread_create(thp,func,argp) { \
    bool t_error = false; \
    bool thread_; \
    DWORD thid; \
    *(thp) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) func, (LPVOID) argp, 0, &thid); \
    if (*(thp) == 0) { \
        t_error = true; \
    } \
}
#define thread_wait(th) { \
    bool t_error = false; \
    WaitForSingleObject(th, INFINITE); \
    CloseHandle(th); \
}
#endif

namespace Kontrol {


class KontrolOSCListener;

static const int MAX_N_OSC_MSGS = 64;
static const int MAX_OSC_MESSAGE_SIZE = 512;
struct OscMsg {
    IpEndpointName origin_;
    int size_;
    char buffer_[MAX_OSC_MESSAGE_SIZE];
};


class OSCReceiver {
public:
    OSCReceiver(const std::shared_ptr<KontrolModel> &param);
    ~OSCReceiver();
    bool listen(unsigned port = 9000);
    void poll();

    void stop();

    void createRack(
            ParameterSource src,
            const EntityId &rackId,
            const std::string &host,
            unsigned port
    ) const;

    void createModule(
            ParameterSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const std::string &displayName,
            const std::string &type
    ) const;

    void createParam(
            ParameterSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const std::vector<ParamValue> &args
    ) const;

    void createPage(
            ParameterSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const EntityId &pageId,
            const std::string &displayName,
            const std::vector<EntityId> paramIds
    ) const;

    void changeParam(
            ParameterSource src,
            const EntityId &rackId,
            const EntityId &moduleId,
            const EntityId &paramId,
            ParamValue v) const;

    void ping(const std::string &host, unsigned port);

    std::shared_ptr<UdpListeningReceiveSocket> socket() { return socket_; }

private:

    std::shared_ptr<KontrolModel> model_;
    unsigned int port_;
    thread_t receive_thread_;
    std::shared_ptr<UdpListeningReceiveSocket> socket_;
    std::shared_ptr<PacketListener> packetListener_;
    std::shared_ptr<KontrolOSCListener> oscListener_;
    PaUtilRingBuffer messageQueue_;
    char msgData_[sizeof(OscMsg) * MAX_N_OSC_MSGS];
    bool ping_;
};

} //namespace