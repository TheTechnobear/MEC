#include "mec_soundplane.h"

#include <iostream>

#include <SoundplaneModel.h>
#include <MLAppState.h>

#include <SoundplaneMECOutput.h>


#include "mec_api.h"
#include "mec_log.h"
#include "mec_prefs.h"
#include "mec_voice.h"

////////////////////////////////////////////////
// TODO
// 1. callback should be on the same thread as process is called on
// 2. voices not needed? as soundplane already does touch alloction, just need to detemine on and off

////////////////////////////////////////////////
class MecSoundplaneHandler: public  MECCallback
{
public:
    MecSoundplaneHandler(MecPreferences& p, IMecCallback& cb)
        :   prefs_(p),
            callback_(cb),
            valid_(true),
            voices_(p.getInt("voices",15)),
            stealVoices_(p.getBool("steal voices",true))
    {
        if (valid_) {
            LOG_0(std::cout  << "MecSoundplaneHandler enabling for mecapi" <<  std::endl;)
        }
    }

    bool isValid() { return valid_;}

    virtual void device(const char* dev, int rows, int cols)
    {
        LOG_1(std::cout     << "MecSoundplaneHandler  device d: ")
        LOG_1(              << " r: " << rows << " c: " << cols)
        LOG_1(              << std::endl;)
    }

    virtual void touch(const char* dev, unsigned long long t, bool a, int touch, float n, float x, float y, float z)
    {
        static const unsigned int NOTE_CH_OFFSET = 1;

        MecVoices::Voice* voice = voices_.voiceId(touch);
        float fn = n;
        float mn = note(fn);
        float mx = clamp(x,-1.0f,1.0f);
        float my = clamp(y,-1.0f,1.0f);
        float mz = clamp(z, 0.0f,1.0f);
        if (a)
        {
            // LOG_2(std::cout     << "MecSoundplaneHandler  touch device d: "   << dev      << " a: "   << a)
            // LOG_2(              << " touch: " <<  touch)
            // LOG_2(              << " note: " <<  n  << " mn: "   << mn << " fn: " << fn)
            // LOG_2(              << " x: " << x      << " y: "   << y    << " z: "   << z)
            // LOG_2(              << " mx: " << mx    << " my: "  << my   << " mz: "  << mz)
            // LOG_2(              << std::endl;)

            if (!voice) {
                voice = voices_.startVoice(touch);
                // LOG_2(std::cout << "start voice for " << key << " ch " << voice->i_ << std::endl;)

                if(!voice && stealVoices_) {
                    // no available voices, steal?
                    MecVoices::Voice* stolen = voices_.oldestActiveVoice();
                    callback_.touchOff(stolen->i_,stolen->note_,stolen->x_,stolen->y_,stolen->z_);
                    voices_.stopVoice(voice);
                    voice = voices_.startVoice(touch);
                }
                if(!voice) callback_.touchOn(voice->i_, mn, mx, my, mz);
            }
            else {
                callback_.touchContinue(voice->i_, mn, mx, my, mz);
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
                LOG_2(std::cout << "stop voice for " << key << " ch " << voice->i_ << std::endl;)
                callback_.touchOff(voice->i_,mn,mx,my,mz);
                voices_.stopVoice(voice);
            }
        }
    }

    virtual void control(const char* dev, unsigned long long t, int id, float val)
    {
        callback_.control(id, clamp(val,-1.0f,1.0f));
    }

private:
    inline  float clamp(float v,float mn, float mx) {return (std::max(std::min(v,mx),mn));}
    float   note(float n) { return n; }

    MecPreferences prefs_;
    IMecCallback& callback_;
    MecVoices voices_;
    bool valid_;
    bool stealVoices_;
};


////////////////////////////////////////////////
MecSoundplane::MecSoundplane(IMecCallback& cb) : 
    active_(false), callback_(cb) {
}

MecSoundplane::~MecSoundplane() {
    deinit();
}

bool MecSoundplane::init(void* arg) {
   MecPreferences prefs(arg);

   if (active_) {
        deinit();
    }
    active_ = false;
    model_.reset(new SoundplaneModel());
    model_->initialize();

    // generate a persistent state for the Model
    std::unique_ptr<MLAppState> pModelState = std::unique_ptr<MLAppState>(new MLAppState(model_.get(), "", "MadronaLabs", "Soundplane", 1));
    pModelState->loadStateFromAppStateFile();
    model_->updateAllProperties();  //??
    model_->setPropertyImmediate("osc_active", 0.0f);
    model_->setPropertyImmediate("mec_active", 1.0f);

    MecSoundplaneHandler *pCb = new MecSoundplaneHandler(prefs,callback_);
    if (pCb->isValid()) {
        model_->mecOutput().connect(pCb);
        model_->setPropertyImmediate("data_freq_mec", 500.0f);
        active_ = true;
    } else {
        delete pCb;
    }

    return active_;
}

bool MecSoundplane::process() {
    //TODO
    //soundplane model callsback on it own threads
    //we need to therefore put the call backs in a 'queue' and then only call
    //the Mec callback when process is called.
    // if (active_) model_->poll(0);
    return true;
}

void MecSoundplane::deinit() {
    if (!model_) return;
    model_.reset();
    active_ = false;
}

bool MecSoundplane::isActive() {
    return active_;
}




