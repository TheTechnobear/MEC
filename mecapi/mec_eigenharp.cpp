#include "mec_eigenharp.h"

#include <iostream>

#include "mec_api.h"
#include "mec_log.h"
#include "mec_prefs.h"
#include "mec_surfacemapper.h"
#include "mec_voice.h"


////////////////////////////////////////////////
class MecEigenharpHandler: public  EigenApi::Callback
{
public:
    MecEigenharpHandler(MecPreferences& p, MecCallback& cb)
        :   prefs_(p),
            callback_(cb),
            valid_(true),
            voices_(p.getInt("voices",15), p.getInt("velocity count",5))
    {
        if (valid_) {
            LOG_0(std::cout  << "MecEigenharpHandler enabling for mecapi" <<  std::endl;)
        }
    }

    bool isValid() { return valid_;}

    virtual void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals)
    {
        LOG_1(std::cout   << "MecEigenharpHandler device d: "  << dev << " dt: " <<  (int) dt)
        LOG_1(            << " r: " << rows     << " c: " << cols)
        LOG_1(            << " s: " << ribbons  << " p: " << pedals)
        LOG_1(            << std::endl;)
        const char* dk = "defaut";
        switch (dt) {
        case EigenApi::Callback::PICO:  dk = "pico";    break;
        case EigenApi::Callback::TAU:   dk = "tau";     break;
        case EigenApi::Callback::ALPHA: dk = "alpha";   break;
        default: dk = "default";
        }

        if (prefs_.exists("mapping")) {
            MecPreferences map(prefs_.getSubTree("mapping"));
            if (map.exists(dk)) {
                MecPreferences devmap(map.getSubTree(dk));
                mapper_.load(devmap);
            }
        }
    }

    virtual void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y)
    {
        MecVoices::Voice* voice = voices_.voiceId(key);
        int     mn = note(key);
        float   mx = bipolar(r);
        float   my = bipolar(y);
        float   mz = unipolar(p);
        if (a)
        {

            LOG_2(std::cout     << "MecEigenharpHandler key device d: "  << dev  << " a: "  << a)
            LOG_2(              << " c: "   << course   << " k: "   << key)
            LOG_2(              << " r: "   << r        << " y: "   << y    << " p: "  << p)
            LOG_2(              << " mx: "  << mx       << " my: "  << my   << " mz: " << mz)
            LOG_2(              << std::endl;)

            if (!voice) {
                voice = voices_.startVoice(key);
                // LOG_2(std::cout << "start voice for " << key << " ch " << voice->i_ << std::endl;)
                voices_.addPressure(voice, mz);
            }
            else {
                if (voice->state_ == MecVoices::Voice::PENDING) {
                    voices_.addPressure(voice, mz);
                    if (voice->state_ == MecVoices::Voice::ACTIVE) {
                        callback_.touchOn(voice->i_, mn, mx, my, voice->v_);
                    }
                    //else ignore, till we have min number of pressures
                }
                else {
                    callback_.touchContinue(voice->i_, mn, mx, my, mz);
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
                // LOG_2(std::cout << "stop voice for " << key << " ch " << voice->i_ << std::endl;)
                callback_.touchOff(voice->i_,mn,mx,my,mz);
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
        callback_.control(0,unipolar(val));
    }

    virtual void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val)
    {
        callback_.control(0x10 + strip,unipolar(val));
    }

    virtual void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val)
    {
        callback_.control(0x10 + pedal,unipolar(val));
    }

private:
    inline  float clamp(float v,float mn, float mx) {return (std::max(std::min(v,mx),mn));}
    float   unipolar(int val) { std::min( float(val) / 4096.0f, 1.0f); }
    float   bipolar(int val) { return clamp(float(val) / 4096.0f, -1.0f, 1.0f);}
    int     note(unsigned key) { return mapper_.noteFromKey(key); }

    MecPreferences prefs_;
    MecCallback& callback_;
    MecSurfaceMapper mapper_;
    MecVoices voices_;
    bool valid_;
};


////////////////////////////////////////////////
MecEigenharp::MecEigenharp(MecCallback& cb) : active_(false), callback_(cb) {
}

MecEigenharp::~MecEigenharp() {
    deinit();
}

bool MecEigenharp::init(void* arg) {
   MecPreferences prefs(arg);

   if (active_) {
        LOG_2(std::cout   << "MecEigenharp::init - already active deinit" << std::endl;)
        deinit();
    }
    active_ = false;

    eigenD_.reset(new EigenApi::Eigenharp("../eigenharp/resources/"));
    MecEigenharpHandler *pCb = new MecEigenharpHandler(prefs,callback_);
    if (pCb->isValid()) {
        eigenD_->addCallback(pCb);
        if (eigenD_->create()) {
            if (eigenD_->start()) {
                active_ = true;
                LOG_1(std::cout   << "MecEigenharp::init - started" << std::endl;)
            } else {
                LOG_2(std::cout   << "MecEigenharp::init - failed to start" << std::endl;)
            }
        } else {
             LOG_2(std::cout   << "MecEigenharp::init - create failed" << std::endl;)
        }
    } else {
        LOG_2(std::cout   << "MecEigenharp::init - invalid callback" << std::endl;)
        delete pCb;
    }
    return active_;
}

bool MecEigenharp::process() {
    if (active_) eigenD_->poll(0);
    return true;
}

void MecEigenharp::deinit() {
    if (!eigenD_) return;
    eigenD_->destroy();
    eigenD_.reset();
    active_ = false;
}

bool MecEigenharp::isActive() {
    return active_;
}




