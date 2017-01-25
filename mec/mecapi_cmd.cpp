#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <pthread.h>

#include <osc/OscOutboundPacketStream.h>
#include <ip/UdpSocket.h>

#include "mec.h"
#include "midi_output.h"

#include <mec_api.h>
#include <mec_prefs.h>

#define OUTPUT_BUFFER_SIZE 1024

//hacks for now 
#define VELOCITY 1.0f
#define PB_RANGE 2.0f
#define MPE_PB_RANGE 48.0f

class MecOSCCallback: public  IMecCallback
{
public:
    MecOSCCallback(MecPreferences& p)
        :   prefs_(p),
            transmitSocket_( IpEndpointName( p.getString("host", "127.0.0.1").c_str(), p.getInt("port", 9001) )),
            valid_(true)
    {
        if (valid_) {
            LOG_0( "mecapi_proc enabling for osc");
        }
    }

    bool isValid() { return valid_;}

    void touchOn(int touchId, float note, float x, float y, float z)
    {
        static std::string topic = "touchOn";
        sendMsg(topic,touchId,note,x,y,z);
    }

    void touchContinue(int touchId, float note, float x, float y, float z)
    {
        static std::string topic = "touchContinue";
        sendMsg(topic,touchId,note,x,y,z);
    }    

    void touchOff(int touchId, float note, float x, float y, float z)
    {
        static std::string topic = "touchOff";
        sendMsg(topic,touchId,note,x,y,z);

    }    

    void control(int ctrlId, float v)
    {
        osc::OutboundPacketStream op( buffer_, OUTPUT_BUFFER_SIZE );
        op << osc::BeginBundleImmediate
           << osc::BeginMessage( "tb/control")
           << ctrlId << v 
           << osc::EndMessage
           << osc::EndBundle;
        transmitSocket_.Send( op.Data(), op.Size() );
    }

    void sendMsg(std::string topic, int touchId, float note, float x, float y, float z) 
    {
        osc::OutboundPacketStream op( buffer_, OUTPUT_BUFFER_SIZE );
        op << osc::BeginBundleImmediate
           << osc::BeginMessage( topic.c_str())
           << touchId << note << x << y << z 
           << osc::EndMessage
           << osc::EndBundle;
        transmitSocket_.Send( op.Data(), op.Size() );
    }    

private:
    MecPreferences prefs_;
    UdpTransmitSocket transmitSocket_;
    char buffer_[OUTPUT_BUFFER_SIZE];
    bool valid_;
};

#define GLOBAL_CH 0
#define NOTE_CH_OFFSET 1
#define BREATH_CC 2
#define STRIP_BASE_CC 0
#define PEDAL_BASE_CC 11

class MecMidiCallback: public  IMecCallback
{
public:
    MecMidiCallback(MecPreferences& p)
        :   prefs_(p), output_(p.getInt("voices", 15), (float) p.getDouble("pitchbend range", 48.0))
    {
        std::string device = prefs_.getString("device");
		int virt = prefs_.getInt("virtual",0);
        if (output_.create(device,virt>0)) {
            LOG_1( "MecMidiCallback enabling for midi to " << device );
            LOG_1( "TODO (MecMidiCallback) :" );
            LOG_1( "- MPE init, including PB range" );
        }
        if(!output_.isOpen()) {
            LOG_0( "MecMidiCallback not open, so invalid for" << device );
        }
    }

    bool isValid() { return output_.isOpen();}

    void touchOn(int touchId, float note, float x, float y, float z)
    {
       output_.touchOn(touchId + NOTE_CH_OFFSET, note, x, y , z);
    }

    void touchContinue(int touchId, float note, float x, float y, float z)
    {
       output_.touchContinue(touchId + NOTE_CH_OFFSET, note, x, y , z);
    }    

    void touchOff(int touchId, float note, float x, float y, float z)
    {
        output_.touchOff(touchId + NOTE_CH_OFFSET);
    }    

    void control(int ctrlId, float v)
    {
        output_.control(GLOBAL_CH, ctrlId,v);
    }
private:
    MecPreferences prefs_;
    MidiOutput output_;
};


void *mecapi_proc(void * arg)
{
    LOG_0( "mecapi_proc start");

    MecPreferences prefs(arg);

    MecPreferences outprefs(prefs.getSubTree("outputs"));
    bool midiEnabled = outprefs.exists("midi");
    bool oscEnabled = outprefs.exists("osc");

    MecApi mecapi;

    if (midiEnabled || oscEnabled) {
        // currently either midi or osc
        // need to add subscripton list to mec api
        if (midiEnabled) {
            MecPreferences midiprefs(outprefs.getSubTree("midi"));
            MecMidiCallback *pCb = new MecMidiCallback(midiprefs);
            if (pCb->isValid()) {
                mecapi.subscribe(pCb);
            } else {
                delete pCb;
            }
        } else if (oscEnabled) {
            MecPreferences oscprefs(outprefs.getSubTree("osc"));
            MecOSCCallback *pCb = new MecOSCCallback(oscprefs);
            if (pCb->isValid()) {
                mecapi.subscribe(pCb);
            } else {
                delete pCb;
            }
        } 

        mecapi.init();

        pthread_mutex_lock(&waitMtx);
        while (keepRunning)
        {
            mecapi.process();
            struct timespec ts;
            getWaitTime(ts, 1000);
            pthread_cond_timedwait(&waitCond, &waitMtx, &ts);
        }
        pthread_mutex_unlock(&waitMtx);
    }
    LOG_0( "mecapi_proc stop");
    pthread_exit(NULL);
}


