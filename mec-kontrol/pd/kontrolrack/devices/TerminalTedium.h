#pragma once

#include "KontrolDevice.h"

#include <KontrolModel.h>


#include <TTuiDevice.h>
#include <string>

class TerminalTedium : public KontrolDevice {
public:
    TerminalTedium();
    virtual ~TerminalTedium();

    //KontrolDevice
    virtual bool init() override;

    void displayPopup(const std::string &text,bool dblLine);
    void displayParamLine(unsigned line, const Kontrol::Parameter &p);
    void displayLine(unsigned line, const char *);
    void invertLine(unsigned line);
    void clearDisplay();
    void flipDisplay();

    void writePoll();
private:
    void send(const char *data, unsigned size);
    void stop() override ;

    std::string asDisplayString(const Kontrol::Parameter &p, unsigned width) const;

    TTuiLite::TTuiDevice device_;

    // gpio will cause havoc on audio thread, so has to be on a different thread!
    struct TTMsg {

        static const int MAX_N_TT_MSGS = 64;
        static const int MAX_TT_MESSAGE_SIZE = 128;
        enum MsgType {
            RENDER,
            DISPLAY_CLEAR, 
            DISPLAY_LINE,
        } type_;

        TTMsg(MsgType t) : type_(t), display_(-1), line_(-1), size_(0) {;}
        TTMsg(MsgType t, int d, int l ) : TTMsg(t), display_(d), line_(l) {;}
        TTMsg(MsgType t, int d, int l, const char* str, int sz ) : TTMsg(t), display_(d), line_(l)  {
            size_ = (sz>=MAX_TT_MESSAGE_SIZE ? MAX_TT_MESSAGE_SIZE -1 ) 
            strncpy(buffer_, str, size_);
            buffer_[MAX_TT_MESSAGE_SIZE-1]=0;

        }

        int display_;
        int line_;
        int size_;
        char buffer_[MAX_TT_MESSAGE_SIZE];
    };


    moodycamel::BlockingReaderWriterQueue<TTMsg> messageQueue_;
    bool running_;
    std::thread writer_thread_;
};
