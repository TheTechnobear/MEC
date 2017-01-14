#include <eigenfreed/eigenfreed.h>

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <pthread.h>

#include <osc/OscOutboundPacketStream.h>
#include <ip/UdpSocket.h>

#include "mec.h"
#include "midi_output.h"

#include <mec_prefs.h>
#include <mec_surfacemapper.h>
#include <mec_voice.h>

#define OUTPUT_BUFFER_SIZE 1024

//hacks for now 
#define VELOCITY 1.0f
#define PB_RANGE 2.0f
#define MPE_PB_RANGE 48.0f

class EigenharpOSCCallback: public  EigenApi::Callback
{
public:
    EigenharpOSCCallback(EigenApi::Eigenharp& eh, MecPreferences& p)
        :   prefs_(p),
            transmitSocket_( IpEndpointName( p.getString("host", "127.0.0.1").c_str(), p.getInt("port", 9001) )),
            valid_(true)
    {
        if (valid_) {
            LOG_0(std::cout  << "eigenharp_proc enabling for osc" <<  std::endl;)
        }
    }

    bool isValid() { return valid_;}

    virtual void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals)
    {
        LOG_1(std::cout   << "eigenharp osc device d: "  << dev << " dt: " <<  (int) dt)
        LOG_1(            << " r: " << rows     << " c: " << cols)
        LOG_1(            << " s: " << ribbons  << " p: " << pedals)
        LOG_1(            << std::endl;)

        const char* dk = "defaut";
        switch(dt) {
            case EigenApi::Callback::PICO:  dk = "pico";    break;
            case EigenApi::Callback::TAU:   dk = "tau";     break;
            case EigenApi::Callback::ALPHA: dk = "alpha";   break;
            default: dk = "default";
        }

        if(prefs_.exists("mapping")) {
            MecPreferences map(prefs_.getSubTree("mapping"));
            if(map.exists(dk)) {
                MecPreferences devmap(map.getSubTree(dk));
                mapper_.load(devmap);
            }
        }
        
        osc::OutboundPacketStream op( buffer_, OUTPUT_BUFFER_SIZE );
        op << osc::BeginBundleImmediate
           << osc::BeginMessage( "/tb/device" )
           << dev << (int) dt << rows << cols << ribbons << pedals
           << osc::EndMessage
           << osc::EndBundle;
        transmitSocket_.Send( op.Data(), op.Size() );
    }

    virtual void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y)
    {
        osc::OutboundPacketStream op( buffer_, OUTPUT_BUFFER_SIZE );
        op << osc::BeginBundleImmediate
           << osc::BeginMessage( "/tb/key" )
           << dev << (int) t << (int) course << (int) key << a << (int) p << r << y << note(key) 
           << osc::EndMessage
           << osc::EndBundle;

        transmitSocket_.Send( op.Data(), op.Size() );
    }

    virtual void breath(const char* dev, unsigned long long t, unsigned val)
    {
    }

    virtual void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val)
    {
    }

    virtual void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val)
    {
    }

private:
    int     note(unsigned key) { return mapper_.noteFromKey(key); }

    MecPreferences prefs_;
    MecSurfaceMapper mapper_;
    UdpTransmitSocket transmitSocket_;
    char buffer_[OUTPUT_BUFFER_SIZE];
    bool valid_;
};

#define GLOBAL_CH 0
#define BREATH_CC 2
#define STRIP_BASE_CC 0
#define PEDAL_BASE_CC 11

class EigenharpMidiCallback: public  EigenApi::Callback
{
public:
    EigenharpMidiCallback(EigenApi::Eigenharp& eh, MecPreferences& p)
        :   prefs_(p)
    {
        std::string device = prefs_.getString("device");
		int virt = prefs_.getInt("virtual",0);
        if (output_.create(device,virt>0)) {
            LOG_1(std::cout << "EigenharpMidiCallback enabling for midi to " << device << std::endl;)
            LOG_1(std::cout << "TODO (EigenharpMidiCallback) :" << std::endl;)
            LOG_1(std::cout << "- velocity handling" << std::endl;)
            LOG_1(std::cout << "- MPE init, including PB range" << std::endl;)
            LOG_1(std::cout << "- scales" << std::endl;)
        }
    }

    bool isValid() { return output_.isOpen();}

    virtual void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals)
    {
        LOG_1(std::cout   << "eigenharp midi device d: "  << dev << " dt: " <<  (int) dt)
        LOG_1(            << " r: " << rows     << " c: " << cols)
        LOG_1(            << " s: " << ribbons  << " p: " << pedals)
        LOG_1(            << std::endl;)

        const char* dk = "defaut";
        switch(dt) {
            case EigenApi::Callback::PICO:  dk = "pico";    break;
            case EigenApi::Callback::TAU:   dk = "tau";     break;
            case EigenApi::Callback::ALPHA: dk = "alpha";   break;
            default: dk = "default";
        }

        if(prefs_.exists("mapping")) {
            MecPreferences map(prefs_.getSubTree("mapping"));
            if(map.exists(dk)) {
                MecPreferences devmap(map.getSubTree(dk));
                mapper_.load(devmap);
            }
        }

        // TODO - send MPE init, including PB range
    }

    virtual void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y)
    {
        static const unsigned int NOTE_CH_OFFSET = 1;

        MecVoices::Voice* voice = voices_.voiceId(key);
        if (a)
        {
            int     mn = note(key);
            float   mx = bipolar(r);
            float   my = bipolar(y);
            float   mz = unipolar(p);

            LOG_2(std::cout     << "eigenharp midi key device d: "  << dev  << " a: "  << a)
            LOG_2(              << " c: "   << course   << " k: "   << key)
            LOG_2(              << " r: "   << r        << " y: "   << y    << " p: "  << p)
            LOG_2(              << " mx: "  << mx       << " my: "  << my   << " mz: " << mz)
            LOG_2(              << std::endl;)

            float pb = (mx * PB_RANGE) / MPE_PB_RANGE;
            if (!voice) {
                voice = voices_.startVoice(key);
                // LOG_2(std::cout << "start voice for " << key << " ch " << (voice->i_ + NOTE_CH_OFFSET) << std::endl;)
                voices_.addPressure(voice, mz);
            }
            else {
                if (voice->state_ == MecVoices::Voice::PENDING) {
                    voices_.addPressure(voice, mz);
                    if (voice->state_ == MecVoices::Voice::ACTIVE) {
                        output_.startTouch(voice->i_ + NOTE_CH_OFFSET, mn, pb, my, voice->v_);
                    }
                    //else ignore, till we have min number of pressures
                }
                else {
                    output_.continueTouch(voice->i_ + NOTE_CH_OFFSET, mn, pb, my, mz);
                }
            }
            voice->note_ = mn;
            voice->x_ = mx;
            voice->y_ = my;
            voice->z_ = mz;
            voice->t_ = t;
        }
        else
        {
            if (voice) {
                // LOG_2(std::cout << "stop voice for " << key << " ch " << (voice->i_ + NOTE_CH_OFFSET) << std::endl;)
                output_.stopTouch(voice->i_ + NOTE_CH_OFFSET);
                voice->note_ = 0;
                voice->x_ = 0;
                voice->y_ = 0;
                voice->z_ = 0;
                voice->t_ = 0;
                voices_.stopVoice(voice);
            }
        }
    }

    virtual void breath(const char* dev, unsigned long long t, unsigned val)
    {
        output_.global(GLOBAL_CH, BREATH_CC, unipolar(val), false);
    }

    virtual void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val)
    {
        output_.global(GLOBAL_CH, STRIP_BASE_CC + strip, unipolar(val), false);
    }

    virtual void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val)
    {
        output_.global(GLOBAL_CH, PEDAL_BASE_CC + pedal, unipolar(val), false);
    }

private:

    float   unipolar(int val) { return float(val) / 4096.0f;}
    float   bipolar(int val) { return float(val) / 4096.0f;}
    int     note(unsigned key) { return mapper_.noteFromKey(key); }

    MecPreferences prefs_;
    MidiOutput output_;
    MecVoices voices_;
    MecSurfaceMapper mapper_;
};


void *eigenharp_proc(void * arg)
{
    LOG_0(std::cout  << "eigenharp_proc start" << std::endl;)

    MecPreferences prefs(arg);

    MecPreferences outprefs(prefs.getSubTree("outputs"));
    bool midiEnabled = outprefs.exists("midi");
    bool oscEnabled = outprefs.exists("osc");

    if (midiEnabled || oscEnabled) {
        EigenApi::Eigenharp myD("../eigenharp/resources/");

        if (oscEnabled) {
            MecPreferences oscprefs(outprefs.getSubTree("osc"));
            EigenharpOSCCallback *pCb = new EigenharpOSCCallback(myD, oscprefs);
            if (pCb->isValid()) {
                myD.addCallback(pCb);
            } else {
                delete pCb;
            }
        }
        if (midiEnabled) {
            MecPreferences midiprefs(outprefs.getSubTree("midi"));
            EigenharpMidiCallback *pCb = new EigenharpMidiCallback(myD, midiprefs);
            if (pCb->isValid()) {
                myD.addCallback(pCb);
            } else {
                delete pCb;
            }
        }

        if (myD.create())
        {
            if (myD.start())
            {
                pthread_mutex_lock(&waitMtx);
                while (keepRunning)
                {
                    myD.poll(1000);
                    struct timespec ts;
                    getWaitTime(ts, 1000);
                    pthread_cond_timedwait(&waitCond, &waitMtx, &ts);
                }
                pthread_mutex_unlock(&waitMtx);
            }
            else
            {
                LOG_0(std::cerr  << "eigenharp_proc eigenapi start failed" << std::endl;)
            }
        }
        // important destructor is called, else 10.11 mac will panic
        myD.destroy();
    }
    LOG_0(std::cout  << "eigenharp_proc stop" << std::endl;)
    pthread_exit(NULL);
}


