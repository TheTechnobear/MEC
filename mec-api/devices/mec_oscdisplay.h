#pragma once



#include "../mec_device.h"


#include <KontrolModel.h>

#include <ip/UdpSocket.h>
#include <string>
#include <readerwriterqueue.h>
#include <thread>


namespace mec {

class OscDisplayParamMode;
class OscDisplayListener;


enum OscDisplayModes {
    OSM_MAINMENU,
    OSM_PRESETMENU,
    OSM_MODULEMENU,
    OSM_MODULESELECTMENU
};

class OscDisplayMode : public Kontrol::KontrolCallback {
public:
    OscDisplayMode() { ; }
    virtual ~OscDisplayMode() { ; }

    virtual void poll() = 0;
    virtual void activate() = 0;
    virtual void navPrev()=0;
    virtual void navNext()=0;
    virtual void navActivate()=0;
};


class OscDisplay : public Device, public Kontrol::KontrolCallback {
public:
    OscDisplay();
    virtual ~OscDisplay();

    //mec::Device
    bool init(void*) override;
    bool process() override;
    void deinit() override ;
    bool isActive() override;

    //Kontrol::KontrolCallback

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
    void addMode(OscDisplayModes mode, std::shared_ptr<OscDisplayMode>);
    void changeMode(OscDisplayModes);
    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;

    void changePot(unsigned pot, float value);


    void displayParamNum(unsigned num, const Kontrol::Parameter &p, bool local);
    void clearParamNum(unsigned num);
    void displayLine(unsigned line, const char *);
    void invertLine(unsigned line);
    void clearDisplay();
    void displayTitle(const std::string &module, const std::string &page);

    void writePoll();

    std::shared_ptr<UdpListeningReceiveSocket> readSocket() { return readSocket_; }

    std::shared_ptr<Kontrol::KontrolModel> model() { return Kontrol::KontrolModel::model(); }

    Kontrol::EntityId currentRack() { return currentRackId_; }

    void currentRack(const Kontrol::EntityId &p) { currentRackId_ = p; }

    Kontrol::EntityId currentModule() { return currentModuleId_; }

    void currentModule(const Kontrol::EntityId &modId);

    void midiLearn(bool b);
    bool midiLearn() { return midiLearnActive_;}
    void modulationLearn(bool b);
    bool modulationLearn() { return modulationLearnActive_;}

    unsigned menuTimeout() {return menuTimeout_;}
private:
    friend class OscDisplayPacketListener;
    friend class OscDisplayListener;

    std::vector<std::shared_ptr<Kontrol::Module>> getModules(const std::shared_ptr<Kontrol::Rack>& rack);

    bool listen(unsigned port);
    bool connect(const std::string& host, unsigned port);


    void navPrev();
    void navNext();
    void navActivate();

    void nextPage();
    void prevPage();
    void nextModule();
    void prevModule();

    void send(const char *data, unsigned size);
    void stop() override;

    struct OscMsg {
        static const int MAX_N_OSC_MSGS = 64;
        static const int MAX_OSC_MESSAGE_SIZE = 128;
        int size_;
        char buffer_[MAX_OSC_MESSAGE_SIZE];
        IpEndpointName origin_; // only used when for recv
    };

    static const unsigned int OUTPUT_BUFFER_SIZE = 1024;
    static char screenBuf_[OUTPUT_BUFFER_SIZE];


    bool writeRunning_;
    bool listenRunning_;
    bool active_;

    std::shared_ptr<UdpTransmitSocket> writeSocket_;
    moodycamel::BlockingReaderWriterQueue<OscMsg> writeMessageQueue_;
    std::thread writer_thread_;

    std::shared_ptr<UdpListeningReceiveSocket> readSocket_;
    std::shared_ptr<PacketListener> packetListener_;
    std::shared_ptr<OscDisplayListener> oscListener_;
    moodycamel::ReaderWriterQueue<OscMsg> readMessageQueue_;
    std::thread receive_thread_;
    unsigned listenPort_;

    Kontrol::EntityId currentRackId_;
    Kontrol::EntityId currentModuleId_;

    std::shared_ptr<OscDisplayParamMode> paramDisplay_;

    bool midiLearnActive_;
    bool modulationLearnActive_;

    OscDisplayModes currentMode_;
    std::map<OscDisplayModes, std::shared_ptr<OscDisplayMode>> modes_;
    std::vector<std::string> moduleOrder_;
    unsigned menuTimeout_;

};

}
