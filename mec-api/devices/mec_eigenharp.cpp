#include "mec_eigenharp.h"

#include "mec_log.h"
#include "mec_prefs.h"
#include "../mec_surfacemapper.h"
#include "../mec_voice.h"


#include <set>

namespace mec {

////////////////////////////////////////////////
class EigenharpHandler : public EigenApi::Callback {
public:
    EigenharpHandler(Preferences &p, ICallback &cb)
            : prefs_(p),
              callback_(cb),
              valid_(true),
              voices_(p.getInt("voices", 15), p.getInt("velocity count", 5)),
              pitchbendRange_((float) p.getDouble("pitchbend range", 2.0)),
              stealVoices_(p.getBool("steal voices", true)),
              throttle_(p.getInt("throttle", 0) == 0 ? 0 : 1000000 / p.getInt("throttle", 0)) {
        if (valid_) {
            LOG_0("EigenharpHandler enabling for mecapi");
        }
    }

    bool isValid() { return valid_; }

    virtual void device(const char *dev, DeviceType dt, int rows, int cols, int ribbons, int pedals) {
        const char *dk = "defaut";
        switch (dt) {
            case EigenApi::Callback::PICO:
                dk = "pico";
                break;
            case EigenApi::Callback::TAU:
                dk = "tau";
                break;
            case EigenApi::Callback::ALPHA:
                dk = "alpha";
                break;
            default:
                dk = "default";
        }

        LOG_1("EigenharpHandler device d: " << dev << " dt: " << (int) dt) << " dk: " << dk;
        LOG_1(" r: " << rows << " c: " << cols);
        LOG_1(" s: " << ribbons << " p: " << pedals);

        if (prefs_.exists("mapping")) {
            Preferences map(prefs_.getSubTree("mapping"));
            if (map.exists(dk)) {
                Preferences devmap(map.getSubTree(dk));
                mapper_.load(devmap);
            }
        }
    }


    virtual void key(const char *dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r,
                     int y) {
        Voices::Voice *voice = voices_.voiceId(key);
        float mx = bipolar(r);
        float my = bipolar(y);
        float mz = unipolar(p);
        float mn = note(key, mx);
        if (a) {

            LOG_3("EigenharpHandler key device d: " << dev << " a: " << a);
            LOG_3(" c: " << course << " k: " << key);
            LOG_3(" r: " << r << " y: " << y << " p: " << p);
            LOG_3(" mn: " << mn << " mx: " << mx << " my: " << my << " mz: " << mz);

            if (!voice) {
                if (stolenKeys_.find(key) != stolenKeys_.end()) {
                    // this key has been stolen, must be released to reactivate it
                    return;
                }

                voice = voices_.startVoice(key);

                if (!voice && stealVoices_) {
                    LOG_2("voice steal required for " << key);
                    // no available voices, steal?
                    Voices::Voice *stolen = voices_.oldestActiveVoice();
                    callback_.touchOff(stolen->i_, stolen->note_, stolen->x_, stolen->y_, 0.0f);
                    stolenKeys_.insert(stolen->id_);
                    voices_.stopVoice(stolen);
                    voice = voices_.startVoice(key);
                    // if(voice) { LOG_1("voice steal found for " << key  "stolen from " << stolen->id_)); }
                }
            }

            if (voice) {
                if (voice->state_ == Voices::Voice::PENDING) {
                    voices_.addPressure(voice, mz);
                    if (voice->state_ == Voices::Voice::ACTIVE) {
                        LOG_2("start voice for " << key << " ch " << voice->i_);
                        callback_.touchOn(voice->i_, mn, mx, my, voice->v_); //v_ = calculated velocity
                        voice->t_ = t;
                    }
                    // dont send to callbacks until we have the minimum pressures for velocity
                } else {
                    if (throttle_ == 0 || (t - voice->t_) >= throttle_) {
                        LOG_2("continue voice for " << key << " ch " << voice->i_);
                        callback_.touchContinue(voice->i_, mn, mx, my, mz);
                        voice->t_ = t;
                    }
                }

                voice->note_ = mn;
                voice->x_ = mx;
                voice->y_ = my;
                voice->z_ = mz;
            }
            // else no voice available

        } else {

            if (voice) {
                LOG_2("stop voice for " << key << " ch " << voice->i_);
                callback_.touchOff(voice->i_, mn, mx, my, mz);
                voices_.stopVoice(voice);
            }
            stolenKeys_.erase(key);
        }
    }

    virtual void breath(const char *dev, unsigned long long t, unsigned val) {
        callback_.control(0, unipolar(val));
    }

    virtual void strip(const char *dev, unsigned long long t, unsigned strip, unsigned val) {
        callback_.control(0x10 + strip, unipolar(val));
    }

    virtual void pedal(const char *dev, unsigned long long t, unsigned pedal, unsigned val) {
        callback_.control(0x20 + pedal, unipolar(val));
    }

private:
    inline float clamp(float v, float mn, float mx) { return (std::max(std::min(v, mx), mn)); }

    float unipolar(int val) { return std::min(float(val) / 4096.0f, 1.0f); }

    float bipolar(int val) { return clamp(float(val) / 4096.0f, -1.0f, 1.0f); }

    //float   note(unsigned key, float mx) { return mapper_.noteFromKey(key) + (mx  * pitchbendRange_) ; }
    float note(unsigned key, float mx) {
        return mapper_.noteFromKey(key) + ((mx > 0.0 ? mx * mx : -mx * mx) * pitchbendRange_);
    }

    Preferences prefs_;
    ICallback &callback_;
    SurfaceMapper mapper_;
    Voices voices_;
    bool valid_;
    float pitchbendRange_;
    bool stealVoices_;
    unsigned long long throttle_;
    std::set<unsigned> stolenKeys_;
};


////////////////////////////////////////////////
Eigenharp::Eigenharp(ICallback &cb) :
        active_(false), callback_(cb), minPollTime_(100) {
}

Eigenharp::~Eigenharp() {
    deinit();
}

bool Eigenharp::init(void *arg) {
    Preferences prefs(arg);

    if (active_) {
        LOG_2("Eigenharp::init - already active deinit");
        deinit();
    }
    active_ = false;
    std::string fwDir = prefs.getString("firmware dir", "./resources/");
    minPollTime_ = prefs.getInt("min poll time", 100);
    eigenD_.reset(new EigenApi::Eigenharp(fwDir.c_str()));
    EigenharpHandler *pCb = new EigenharpHandler(prefs, callback_);
    if (pCb->isValid()) {
        eigenD_->addCallback(pCb);
        if (eigenD_->create()) {
            if (eigenD_->start()) {
                active_ = true;
                LOG_1("Eigenharp::init - started");
            } else {
                LOG_2("Eigenharp::init - failed to start");
            }
        } else {
            LOG_2("Eigenharp::init - create failed");
        }
    } else {
        LOG_2("Eigenharp::init - invalid callback");
        delete pCb;
    }
    return active_;
}

bool Eigenharp::process() {
    const int sleepTime = 0;
    if (active_) eigenD_->poll(sleepTime, minPollTime_);
    return true;
}

void Eigenharp::deinit() {
    if (!eigenD_) return;
    eigenD_->destroy();
    eigenD_.reset();
    active_ = false;
}

bool Eigenharp::isActive() {
    return active_;
}

}


