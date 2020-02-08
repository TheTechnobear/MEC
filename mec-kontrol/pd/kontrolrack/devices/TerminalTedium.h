#pragma once

#include "KontrolDevice.h"
#include <KontrolModel.h>

#include <cstring>
#include <string>
#include <thread>
#include <crono>
#include <readerwriterqueue.h>

#include <TTuiDevice.h>

class TerminalTedium : public KontrolDevice {
public:
    TerminalTedium();
    virtual ~TerminalTedium();

    //KontrolDevice
    virtual bool init() override;
    virtual void changePot(unsigned pot, float value);
    virtual void changeEncoder(unsigned encoder, float value);
    virtual void encoderButton(unsigned encoder, bool value);

    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override;
    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Page &) override;
    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;
    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &, const Kontrol::Parameter &) override;
    void resource(Kontrol::ChangeSource, const Kontrol::Rack &, const std::string &, const std::string &) override;
    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::EntityId &, const std::string &) override;
    void savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;


    void displayParamLine(unsigned line, const Kontrol::Parameter &p);
    void displayTitle(const std::string &module, const std::string &page);


    void displayLine(unsigned display, unsigned line, const std::string& str);
    void clearDisplay(unsigned display);
    void flipDisplay(unsigned display);
    void writePoll();
    void invertLine(unsigned display,unsigned line);
private:
    void send(const char *data, unsigned size);
    void stop() override ;

    std::string asDisplayString(const Kontrol::Parameter &p, unsigned width) const;

    
    std::shared_ptr<DeviceMode> paramDisplay_;
    TTuiLite::TTuiDevice device_;

    std::chrono::system_clock::time_point encoderDown_;
    bool encoderLongHold_=false;
    bool encoderMenu_=false;

    // gpio will cause havoc on audio thread, so has to be on a different thread!
    struct TTMsg {

        static const int MAX_N_TT_MSGS = 64;
        static const int MAX_TT_MESSAGE_SIZE = 128;
        enum MsgType {
            RENDER,
            DISPLAY_CLEAR,
            DISPLAY_LINE,
            MAX_TYPE
        } type_;


        TTMsg() : type_(MAX_TYPE), display_(-1), line_(-1), size_(0) {;}
        TTMsg(MsgType t) : type_(t), display_(-1), line_(-1), size_(0) {;}
        TTMsg(MsgType t, int d) : TTMsg(t)  { display_ = d;}
        TTMsg(MsgType t, int d, int l ) : TTMsg(t)  { display_ = d; line_ = l;}
        TTMsg(MsgType t, int d, int l, const char* str, int sz ) : TTMsg(t) {
            display_ = d;
            line_ = l;
            size_ = (sz >= MAX_TT_MESSAGE_SIZE ? MAX_TT_MESSAGE_SIZE - 1 : sz );
            strncpy(buffer_, str, size_);
            buffer_[MAX_TT_MESSAGE_SIZE - 1] = 0;

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
