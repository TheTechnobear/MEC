#include "mec_eigenharp.h"

#include "mec_api.h"
#include "mec_log.h"
#include "mec_prefs.h"
#include "mec_surfacemapper.h"
#include "mec_voice.h"


////////////////////////////////////////////////
class MecEigenharpHandler: public  EigenApi::Callback
{
public:
    MecEigenharpHandler(MecPreferences& p, IMecCallback& cb)
        :   prefs_(p),
            callback_(cb),
            valid_(true),
            voices_(p.getInt("voices",15), p.getInt("velocity count",5)),
            pitchbendRange_((float) p.getDouble("pitchbend range", 2.0)),
            stealVoices_(p.getBool("steal voices",true))
    {
        if (valid_) {
            LOG_0("MecEigenharpHandler enabling for mecapi");
        }
    }

    bool isValid() { return valid_;}

    virtual void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals)
    {
        LOG_1("MecEigenharpHandler device d: "  << dev << " dt: " <<  (int) dt);
        LOG_1(" r: " << rows     << " c: " << cols);
        LOG_1(" s: " << ribbons  << " p: " << pedals);
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
        float   mx = bipolar(r);
        float   my = bipolar(y);
        float   mz = unipolar(p);
        float   mn = note(key,mx);
        if (a)
        {

            LOG_2("MecEigenharpHandler key device d: "  << dev  << " a: "  << a);
            LOG_2(" c: "   << course   << " k: "   << key);
            LOG_2(" r: "   << r        << " y: "   << y    << " p: "  << p);
            LOG_2(" mn: " << mn << " mx: "  << mx       << " my: "  << my   << " mz: " << mz);

            if (!voice) {
                voice = voices_.startVoice(key);
                // LOG_2("start voice for " << key << " ch " << voice->i_);

                if(!voice && stealVoices_) {
                    // no available voices, steal?
                    MecVoices::Voice* stolen = voices_.oldestActiveVoice();
                    callback_.touchOff(stolen->i_,stolen->note_,stolen->x_,stolen->y_,stolen->z_);
                    voices_.stopVoice(voice);
                    voice = voices_.startVoice(key);
                }
                if(voice) voices_.addPressure(voice, mz);
            }
            else {
                if (voice->state_ == MecVoices::Voice::PENDING) {
                    voices_.addPressure(voice, mz);
                    if (voice->state_ == MecVoices::Voice::ACTIVE) {
                        callback_.touchOn(voice->i_, mn, mx, my, voice->v_); //v_ = calculated velocity
                    }
                    // dont send to callbacks until we have the minimum pressures for velocity
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
                // LOG_2("stop voice for " << key << " ch " << voice->i_);
                callback_.touchOff(voice->i_,mn,mx,my,mz);
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
        callback_.control(0x20 + pedal,unipolar(val));
    }

private:
    inline  float clamp(float v,float mn, float mx) {return (std::max(std::min(v,mx),mn));}
    float   unipolar(int val) { return std::min( float(val) / 4096.0f, 1.0f); }
    float   bipolar(int val) { return clamp(float(val) / 4096.0f, -1.0f, 1.0f);}
    float   note(unsigned key,float mx) { return mapper_.noteFromKey(key) + (mx * pitchbendRange_) ; }

    MecPreferences prefs_;
    IMecCallback& callback_;
    MecSurfaceMapper mapper_;
    MecVoices voices_;
    bool valid_;
    float pitchbendRange_;
    bool stealVoices_;
};


////////////////////////////////////////////////
MecEigenharp::MecEigenharp(IMecCallback& cb) : 
    active_(false), callback_(cb), minPollTime_(100)  {
}

MecEigenharp::~MecEigenharp() {
    deinit();
}

bool MecEigenharp::init(void* arg) {
   MecPreferences prefs(arg);

   if (active_) {
        LOG_2("MecEigenharp::init - already active deinit");
        deinit();
    }
    active_ = false;
	std::string fwDir = prefs.getString("firmware dir","../eigenharp/resources/");
	minPollTime_ = prefs.getInt("min poll time",100);
    eigenD_.reset(new EigenApi::Eigenharp(fwDir.c_str()));
    MecEigenharpHandler *pCb = new MecEigenharpHandler(prefs,callback_);
    if (pCb->isValid()) {
        eigenD_->addCallback(pCb);
        if (eigenD_->create()) {
            if (eigenD_->start()) {
                active_ = true;
                LOG_1("MecEigenharp::init - started");
            } else {
                LOG_2("MecEigenharp::init - failed to start");
            }
        } else {
             LOG_2("MecEigenharp::init - create failed");
        }
    } else {
        LOG_2("MecEigenharp::init - invalid callback");
        delete pCb;
    }
    return active_;
}

bool MecEigenharp::process() {
    const int sleepTime = 0;
    if (active_) eigenD_->poll(sleepTime,minPollTime_);
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




