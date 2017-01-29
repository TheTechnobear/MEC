#include "mec_soundplane.h"

#include <SoundplaneModel.h>
#include <MLAppState.h>

#include <SoundplaneMECOutput.h>

#include "../mec_log.h"
#include "../mec_prefs.h"
#include "../mec_voice.h"

////////////////////////////////////////////////
// TODO
// 1. voices not needed? as soundplane already does touch alloction, just need to detemine on and off
////////////////////////////////////////////////
class MecSoundplaneHandler: public  SoundplaneMECCallback
{
public:
    MecSoundplaneHandler(MecPreferences& p, MecMsgQueue& q)
        :   prefs_(p),
            queue_(q),
            valid_(true),
            voices_(p.getInt("voices", 15)),
            stealVoices_(p.getBool("steal voices", true))
    {
        if (valid_) {
            LOG_0("MecSoundplaneHandler enabling for mecapi");
        }
    }

    bool isValid() { return valid_;}

    virtual void device(const char* dev, int rows, int cols)
    {
        LOG_1("MecSoundplaneHandler  device d: " << dev);
        LOG_1(" r: " << rows << " c: " << cols);
    }

    virtual void touch(const char* dev, unsigned long long t, bool a, int touch, float n, float x, float y, float z)
    {
        static const unsigned int NOTE_CH_OFFSET = 1;

        MecVoices::Voice* voice = voices_.voiceId(touch);
        float fn = n;
        float mn = note(fn);
        float mx = clamp(x, -1.0f, 1.0f);
        float my = clamp((y - 0.5 ) * 2.0 , -1.0f, 1.0f);
        float mz = clamp(z, 0.0f, 1.0f);

        MecMsg msg;
        msg.type_ = MecMsg::TOUCH_OFF;
        msg.data_.touch_.touchId_ = -1;
        msg.data_.touch_.note_ = mn;
        msg.data_.touch_.x_ = mx;
        msg.data_.touch_.y_ = my;
        msg.data_.touch_.z_ = mz;
        if (a)
        {
            // LOG_1("MecSoundplaneHandler  touch device d: "   << dev      << " a: "   << a)
            // LOG_1(" touch: " <<  touch);
            // LOG_1(" note: " <<  n  << " mn: "   << mn << " fn: " << fn);
            // LOG_1(" x: " << x      << " y: "   << y    << " z: "   << z);
            // LOG_1(" mx: " << mx    << " my: "  << my   << " mz: "  << mz);

            if (!voice) {
                voice = voices_.startVoice(touch);
                // LOG_2(std::cout << "start voice for " << key << " ch " << voice->i_ << std::endl;)

                if (!voice && stealVoices_) {
                    // no available voices, steal?
                    MecVoices::Voice* stolen = voices_.oldestActiveVoice();
                    
                    MecMsg stolenMsg;
                    stolenMsg.type_ = MecMsg::TOUCH_OFF;
                    stolenMsg.data_.touch_.touchId_ = stolen->i_;
                    stolenMsg.data_.touch_.note_ = stolen->note_;
                    stolenMsg.data_.touch_.x_ = stolen->x_;
                    stolenMsg.data_.touch_.y_ = stolen->y_;
                    stolenMsg.data_.touch_.z_ = stolen->z_;

                    queue_.addToQueue(stolenMsg);

                    voices_.stopVoice(voice);
                    voice = voices_.startVoice(touch);
                }

                if (voice)  {
                    msg.type_ = MecMsg::TOUCH_ON;
                    msg.data_.touch_.touchId_ = voice->i_;
                    queue_.addToQueue(msg);
                }
            }
            else {
                msg.type_ = MecMsg::TOUCH_CONTINUE;
                msg.data_.touch_.touchId_ = voice->i_;
                queue_.addToQueue(msg);
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
                // LOG_2("stop voice for " << touch << " ch " << voice->i_ );
                msg.type_ = MecMsg::TOUCH_OFF;
                msg.data_.touch_.touchId_ = voice->i_;
                queue_.addToQueue(msg);

                voices_.stopVoice(voice);
            }
        }
    }

    virtual void control(const char* dev, unsigned long long t, int id, float val)
    {
        MecMsg msg;
        msg.type_ = MecMsg::CONTROL;
        msg.data_.control_.controlId_ = id;
        msg.data_.control_.value_ = clamp(val, -1.0f, 1.0f);
        queue_.addToQueue(msg);
    }

private:
    inline  float clamp(float v, float mn, float mx) {return (std::max(std::min(v, mx), mn));}
    float   note(float n) { return n; }

    MecPreferences prefs_;
    MecMsgQueue& queue_;
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
    std::string appDir = prefs.getString("app state dir", ".");

    // generate a persistent state for the Model
    std::unique_ptr<MLAppState> pModelState = std::unique_ptr<MLAppState>(new MLAppState(model_.get(), "", "MadronaLabs", "Soundplane", 1, appDir));
    pModelState->loadStateFromAppStateFile();
    model_->updateAllProperties();  //??
    model_->setPropertyImmediate("midi_active", 0.0f);
    model_->setPropertyImmediate("osc_active",  0.0f);
    model_->setPropertyImmediate("mec_active",  1.0f);
    model_->setPropertyImmediate("data_freq_mec", 500.0f);

    MecSoundplaneHandler *pCb = new MecSoundplaneHandler(prefs, queue_);
    if (pCb->isValid()) {
        model_->mecOutput().connect(pCb);
        LOG_0("MecSoundplane::init - model init");
        model_->initialize();
        active_ = true;
        LOG_0("MecSoundplane::init - complete");
    } else {
        LOG_0("MecSoundplane::init - delete callback");
        delete pCb;
    }

    return active_;
}

bool MecSoundplane::process() {
    MecMsg msg;
    while (queue_.nextMsg(msg)) {
        switch (msg.type_) {
        case MecMsg::TOUCH_ON:
            callback_.touchOn(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::TOUCH_CONTINUE:
            callback_.touchContinue(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::TOUCH_OFF:
            callback_.touchOff(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::CONTROL :
            callback_.control(
                msg.data_.control_.controlId_,
                msg.data_.control_.value_);
            break;
        default:
            LOG_0("MecSoundplane::process unhandled message type");
        }
    }
    return true;
}

void MecSoundplane::deinit() {
    LOG_0("MecSoundplane::deinit");
    if (!model_) return;
    LOG_0("MecSoundplane::reset model");
    model_.reset();
    active_ = false;
}

bool MecSoundplane::isActive() {
    return active_;
}




