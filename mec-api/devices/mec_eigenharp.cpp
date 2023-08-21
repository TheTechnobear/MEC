#include "mec_eigenharp.h"

#include "mec_log.h"
#include "../mec_surfacemapper.h"
#include "../mec_voice.h"
#include <unistd.h>


#include <set>

namespace mec {



////////////////////////////////////////////////
class EigenharpHandler : public EigenApi::Callback {
public:
    EigenharpHandler(EigenApi::Eigenharp* api, Preferences &p, ICallback &cb)
            : api_(api),
              prefs_(p),
              callback_(cb),
              valid_(true),
              voices_(static_cast<unsigned>(p.getInt("voices", Voices::NUM_VOICES)),
                      static_cast<unsigned>(p.getInt("velocity count", Voices::V_COUNT)),
                      static_cast<float>(p.getDouble("velocity curve", Voices::V_CURVE_AMT )),
                      static_cast<float>(p.getDouble("velocity scale", Voices::V_SCALE_AMT ))
                      ),
              pitchbendRange_((float) p.getDouble("pitchbend range", 2.0)),
              stealVoices_(p.getBool("steal voices", true)),
              throttle_(p.getInt("throttle", 0) == 0
                        ? 0 : 1000000ULL /
                              p.getInt("throttle",
                                       0)) {
        if (valid_) {
            LOG_0("EigenharpHandler enabling for mecapi");
        }
    }

    bool isValid() { return valid_; }

    static constexpr unsigned COURSE_OFFSET=120;

    void setLED(const char *dev, int key, int colour) {
        unsigned c = key / COURSE_OFFSET;
        unsigned k = key % COURSE_OFFSET;
        api_->setLED(dev, c , k, colour);
    }

    virtual void device(const char *dev, DeviceType dt, int rows, int cols, int ribbons, int pedals) {
        const char *dk;
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

        if(prefs_.exists(dk)) {
            Preferences device_prefs(prefs_.getSubTree(dk));

            if (device_prefs.exists("leds")) {
                Preferences leds(device_prefs.getSubTree("leds"));
                if (leds.exists("green")) {
                    Preferences::Array a(leds.getArray("green"));
                    for(int i=0;i<a.getSize();i++){ setLED(dev,a.getInt(i),1); }
                }
                if (leds.exists("orange")) {
                    Preferences::Array a(leds.getArray("orange"));
                    for(int i=0;i<a.getSize();i++){ setLED(dev,a.getInt(i),3); }
                }
                if (leds.exists("red")) {
                    Preferences::Array a(leds.getArray("red"));
                    for(int i=0;i<a.getSize();i++){ setLED(dev,a.getInt(i),2); }
                }
            }
            if (device_prefs.exists("mapping")) {
                Preferences map(device_prefs.getSubTree("mapping"));
                mapper_.load(map);
            }
        }
    }


    virtual void key(const char *dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r,
                     int y) {
        Voices::Voice *voice = voices_.voiceId(key);
        float mx = bipolar(r);
        float my = bipolar(y);
        float mz = unipolar(p);
        float mn = note(key, mx) + (COURSE_OFFSET * course);
        if (a) {

            LOG_3("EigenharpHandler key device d: " << dev << " a: " << a);
            LOG_3(" c: " << course << " k: " << key);
            LOG_3(" r: " << r << " y: " << y << " p: " << p);
            LOG_3(" mn: " << mn << " mx: " << mx << " my: " << my << " mz: " << mz);

            if (inactiveKeys_.find(key) != inactiveKeys_.end()) {
                // this key has been stolen, must be released to reactivate it
                return;
            }

            if (!voice) {

                voice = voices_.startVoice(key);

                if (!voice && stealVoices_) {
                    // LOG_1("voice steal required for " << key);
                    // no available voices, steal?
                    Voices::Voice *stolen = voices_.oldestActiveVoice();
                    if(stolen) {
                        if(stolen->state_ == Voices::Voice::ACTIVE) {
                            // LOG_1("voice stolen found for " << key  << " stolen from (active) " << stolen->id_);
                            callback_.touchOff(stolen->i_, stolen->note_, stolen->x_, stolen->y_, 0.0f);
                        } else {
                            // LOG_1("voice stolen found for " << key  << " stolen from (inactive) " << stolen->id_);
                        }
                        inactiveKeys_.insert((unsigned) stolen->id_);
                        voices_.stopVoice(stolen);
                        voice = voices_.startVoice(key);
                        // if(voice) { LOG_1("voice steal found for " << key << "stolen from " << stolen->id_); }
                   } else {
                     LOG_1("unable to steal voice " << key);
                   }
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
            } else {
                // else no voice available
                // LOG_2("mark inactive key " << key);
                inactiveKeys_.insert(key);
            }

        } else {
            if (inactiveKeys_.find(key) == inactiveKeys_.end()) {
                if (voice) {
                    if(voice->state_ == Voices::Voice::ACTIVE) {
                        LOG_2("stop voice for " << key << " ch " << voice->i_);
                        callback_.touchOff(voice->i_, mn, mx, my, mz);
                        voices_.stopVoice(voice);
                    }
                    else if(voice->state_ == Voices::Voice::PENDING) {
                        // dont send touchoff, as touchOn not sent
                        voices_.stopVoice(voice);
                    } else {
                        LOG_1("voice already inactive" << key << " ch " << voice->i_);
                    }
                } else {
                    LOG_1("trying to stop voice, but not found" << key);
                }
            } else {
                // LOG_2("remove inactive key " << key);
                inactiveKeys_.erase(key);
            }
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
    
    EigenApi::Eigenharp* api_;
    Preferences prefs_;
    ICallback &callback_;
    SurfaceMapper mapper_;
    Voices voices_;
    bool valid_;
    float pitchbendRange_;
    bool stealVoices_;
    unsigned long long throttle_;
    std::set<unsigned> inactiveKeys_;
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
    std::string fwDir = prefs.getString("firmware dir", "./");
    minPollTime_ = prefs.getInt("min poll time", 100);
    LOG_0("Eigenharp firmware dir : " << fwDir);
    EigenApi::FWR_Posix reader(fwDir.c_str());
    eigenD_.reset(new EigenApi::Eigenharp(reader));
    eigenD_->setPollTime(minPollTime_);
    EigenharpHandler *pCb = new EigenharpHandler(eigenD_.get(), prefs, callback_);
    if (pCb->isValid()) {
        eigenD_->addCallback(pCb);
        if (eigenD_->start()) {
            active_ = true;
            LOG_1("Eigenharp::init - started");
        } else {
            LOG_2("Eigenharp::init - failed to start");
        }
    } else {
        LOG_2("Eigenharp::init - invalid callback");
        delete pCb;
    }
    return active_;
}

bool Eigenharp::process() {
    if (active_) eigenD_->process();
    return true;
}

void Eigenharp::deinit() {
    LOG_1("Eigenharp:deinit");
    if (!eigenD_) return;
    eigenD_->stop();
    eigenD_.reset();
    active_ = false;
}

bool Eigenharp::isActive() {
    return active_;
}

}


