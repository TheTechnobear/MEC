/**
** This class handles the soundplane by using the soundplane model, which then configures the touch tracker etc.
** This is the PREFERRED route.
**/

#include <iostream>
#include <unistd.h>
#include <string.h>

#include <pthread.h>

#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>


#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <unistd.h>

#include <iomanip>
#include <string.h>

#include <SoundplaneModel.h>
#include <SoundplaneMECOutput.h>
#include <MLAppState.h>

#include "mec.h"
#include "midi_output.h"

#include <mec_prefs.h>
#include <mec_voice.h>

#define GLOBAL_CH 0
#define BASE_CC 0

// hack for now, review these !
#define MPE_PB_RANGE 48.0f

class SoundplaneMidiCallback: public  MECCallback
{
public:
    SoundplaneMidiCallback(SoundplaneModel* pModel, MecPreferences& p)
        : pModel_(pModel), prefs_(p)
    {
        std::string device = prefs_.getString("device");
        if (output_.create(device)) {
            LOG_1(std::cout << "SoundplaneMidiCallback enabling for midi to " << device << std::endl;)
            LOG_1(std::cout << "TODO (SoundplaneMidiCallback) :" << std::endl;)
            LOG_1(std::cout << "- MPE init, including PB range" << std::endl;)
            LOG_1(std::cout << "- scales" << std::endl;)
        }
    }

    bool isValid() { return output_.isOpen();}

    virtual void device(const char* dev, int rows, int cols)
    {
        LOG_1(std::cout     << "soundplane midi device d: ")
        LOG_1(              << " r: " << rows << " c: " << cols)
        LOG_1(              << std::endl;)
        // TODO - send MPE init, including PB range
    }

    virtual void touch(const char* dev, unsigned long long t, bool a, int touch, float n, float x, float y, float z)
    {
        static const unsigned int NOTE_CH_OFFSET = 1;

        MecVoices::Voice* voice = voices_.voiceId(touch);
        if (a)
        {
            float fn = n;
            int   mn = note(fn);
            float mx = x;
            float my = y;
            float mz = z;

            // LOG_2(std::cout     << "soundplane midi touch device d: "   << dev      << " a: "   << a)
            // LOG_2(              << " touch: " <<  touch)
            // LOG_2(              << " note: " <<  n  << " mn: "   << mn << " fn: " << fn)
            // LOG_2(              << " x: " << x      << " y: "   << y    << " z: "   << z)
            // LOG_2(              << " mx: " << mx    << " my: "  << my   << " mz: "  << mz)
            // LOG_2(              << std::endl;)


            if (!voice) {
                voice = voices_.startVoice(touch);
                voice->note_ = mn;
                LOG_2(std::cout << "start voice for " << touch << " ch " << (voice->i_ + NOTE_CH_OFFSET) << std::endl;)
                float pb = (fn - voice->note_) / MPE_PB_RANGE;
                output_.startTouch(voice->i_ + NOTE_CH_OFFSET, voice->note_, pb, my, mz);
            }
            else {
                float pb = (fn - voice->note_) / MPE_PB_RANGE;
                // LOG_2(std::cout     << "soundplane midi touch continue pb1: " << (fn - voice->note_)  << " rpb: " << pb << std::endl;)
                output_.continueTouch(voice->i_ + NOTE_CH_OFFSET, voice->note_, pb, my, mz);
            }
            voice->x_ = mx;
            voice->y_ = my;
            voice->z_ = mz;
            voice->t_ = t;
        }
        else
        {
            if (voice) {
                LOG_2(std::cout << "stop voice for " << key << " ch " << (voice->i_ + NOTE_CH_OFFSET) << std::endl;)
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

    virtual void global(const char* dev, unsigned long long t, int id, float val)
    {
        output_.global(GLOBAL_CH, BASE_CC + id, val, false);
    }


private:
    int  note(float n) { return n; }

    SoundplaneModel* pModel_;
    MecPreferences prefs_;
    MidiOutput output_;
    MecVoices voices_;
};




void *soundplane_proc(void * arg)
{
    LOG_0(std::cout  << "soundplane_proc start" << std::endl;)

    MecPreferences prefs(arg);
    MecPreferences outprefs(prefs.getSubTree("outputs"));
    bool midiEnabled = outprefs.exists("midi");
    bool oscEnabled = outprefs.exists("osc");

    pthread_mutex_lock(&waitMtx);

    if (midiEnabled || oscEnabled) {

        SoundplaneModel* pModel = new SoundplaneModel();
        pModel->initialize();

        // generate a persistent state for the Model
        std::unique_ptr<MLAppState> pModelState = std::unique_ptr<MLAppState>(new MLAppState(pModel, "", "MadronaLabs", "Soundplane", 1));
        pModelState->loadStateFromAppStateFile();
        pModel->updateAllProperties();  //??
        pModel->setPropertyImmediate("osc_active", 0.0f);
        pModel->setPropertyImmediate("mec_active", 1.0f);

        // if (oscEnabled) {
        //     MecPreferences oscprefs(outprefs.getSubTree("osc"));
        //     SoundplaneOSCCallback *pCb = new SoundplaneOSCCallback(myD, oscprefs);
        //     if (pCb->isValid()) {
        //         myD.addCallback(pCb);
        //     } else {
        //         delete pCb;
        //     }
        // }
        if (midiEnabled) {
            MecPreferences midiprefs(outprefs.getSubTree("midi"));
            SoundplaneMidiCallback *pCb = new SoundplaneMidiCallback(pModel, midiprefs);
            if (pCb->isValid()) {
                pModel->mecOutput().connect(pCb);
                pModel->setPropertyImmediate("data_freq_mec", 500.0f);
            } else {
                delete pCb;
            }
        }
        while (keepRunning) {
            pthread_cond_wait(&waitCond, &waitMtx);
        }
        delete pModel;
    }

    LOG_0(std::cout  << "soundplane_proc stop" << std::endl;)
    pthread_mutex_unlock(&waitMtx);
    pthread_exit(NULL);
}

