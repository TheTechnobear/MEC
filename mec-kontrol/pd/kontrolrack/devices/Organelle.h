#pragma once

#include "KontrolDevice.h"

#include <KontrolModel.h>

#include <ip/UdpSocket.h>

#include <string>
#include <readerwriterqueue.h>
#include <thread>

class Organelle : public KontrolDevice {
public:
    Organelle();
    virtual ~Organelle();

    //KontrolDevice
    virtual bool init() override;

    void displayPopup(const std::string &text,bool dblLine);
    void displayParamLine(unsigned line, const Kontrol::Parameter &p);
    void displayLine(unsigned line, const char *);
    void invertLine(unsigned line);
    void clearDisplay();
    void flipDisplay();

    void writePoll();


    void sendEnableSubMenu();
    void sendGoHome();
private:
    void send(const char *data, unsigned size);
    void stop() override ;

    struct OscMsg {
        static const int MAX_N_OSC_MSGS = 64;
        static const int MAX_OSC_MESSAGE_SIZE = 128;
        int size_;
        char buffer_[MAX_OSC_MESSAGE_SIZE];
    };

    static const unsigned OUTPUT_BUFFER_SIZE = 1024;
    static char screenBuf_[OUTPUT_BUFFER_SIZE];


    bool connect();

    std::string asDisplayString(const Kontrol::Parameter &p, unsigned width) const;
    std::shared_ptr<UdpTransmitSocket> socket_;

    moodycamel::BlockingReaderWriterQueue<OscMsg> messageQueue_;
    bool running_;
    std::thread writer_thread_;
};
