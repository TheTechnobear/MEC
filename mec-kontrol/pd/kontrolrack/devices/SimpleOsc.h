#pragma once

#include "KontrolDevice.h"

#include <KontrolModel.h>

#include <ip/UdpSocket.h>

#include <string>
#include <readerwriterqueue.h>
#include <thread>

class OscParamMode;
class SimpleOSCListener;

class SimpleOsc : public KontrolDevice {
public:
    SimpleOsc();
    virtual ~SimpleOsc();

    //KontrolDevice
    virtual bool init() override;

    void displayPopup(const std::string &text,bool dblLine);
    void displayParamNum(unsigned num, const Kontrol::Parameter &p);
    void clearParamNum(unsigned num);
    void displayLine(unsigned line, const char *);
    void invertLine(unsigned line);
    void clearDisplay();
    void displayTitle(const std::string& module, const std::string& page);

    void writePoll();
    bool listen(unsigned port);
    std::shared_ptr<UdpListeningReceiveSocket> readSocket() { return readSocket_; }
private:
    friend class SimpleOscPacketListener;
    friend class SimpleOSCListener;

    void nextPage();
    void prevPage();
    void nextModule();
    void prevModule();

    void send(const char *data, unsigned size);
    void stop() override ;
    void poll() override;

    struct OscMsg {
        static const int MAX_N_OSC_MSGS = 64;
        static const int MAX_OSC_MESSAGE_SIZE = 128;
        int size_;
        char buffer_[MAX_OSC_MESSAGE_SIZE];
        IpEndpointName origin_; // only used when for recv
    };

    static const unsigned int OUTPUT_BUFFER_SIZE = 1024;
    static char screenBuf_[OUTPUT_BUFFER_SIZE];

    bool connect();

    bool running_;

    std::shared_ptr<UdpTransmitSocket> writeSocket_;
    moodycamel::BlockingReaderWriterQueue<OscMsg> writeMessageQueue_;
    std::thread writer_thread_;

    std::shared_ptr<UdpListeningReceiveSocket> readSocket_;
    std::shared_ptr<PacketListener> packetListener_;
    std::shared_ptr<SimpleOSCListener> oscListener_;
    moodycamel::ReaderWriterQueue<OscMsg> readMessageQueue_;
    std::thread receive_thread_;
    unsigned listenPort_;

    std::shared_ptr<OscParamMode> paramDisplay_;
};
