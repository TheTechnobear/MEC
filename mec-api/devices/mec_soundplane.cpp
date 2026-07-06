#include "mec_soundplane.h"


#include "mec_log.h"
#include "../mec_voice.h"
#include <set>


#ifdef __WINDOWS__
static __int64 __ticks_per_second = 0LL;

unsigned long long microtime()
{
    LARGE_INTEGER ticks;
    __int64 quot, rem;

    if(!__ticks_per_second)
    {
        LARGE_INTEGER l;
        QueryPerformanceFrequency(&l);
        __ticks_per_second = l.QuadPart;
    }

    QueryPerformanceCounter(&ticks);

    quot = ticks.QuadPart/__ticks_per_second;
    rem = ticks.QuadPart%__ticks_per_second;

    return (quot*1000000)+((rem*1000000)/__ticks_per_second);
}
#endif


#if defined(__LINUX__)

#include <time.h>
#include <sys/time.h>
unsigned long long microtime()
{
    struct timeval tv;
    unsigned long long now;

    gettimeofday(&tv,0);
    now = 1000000ULL * (unsigned long long)(tv.tv_sec);
    now += (unsigned long long)tv.tv_usec;

    return now;
}

#endif

#if defined(__APPLE__)

#include <CoreAudio/HostTime.h>
#include <time.h>

unsigned long long microtime()
{
    return AudioConvertHostTimeToNanos(AudioGetCurrentHostTime())/1000LL;
}

#endif




namespace mec {

   
class SPLiteCallback {
public:
    virtual ~SPLiteCallback() = default;
    virtual void onInit()   {;}
    virtual void onFrame()  {;}
    virtual void onDeinit() {;}
    virtual void onError(unsigned err, const char *errStr) {;}

    virtual void touchOn(unsigned voice, float x,float y, float z) = 0;
    virtual void touchContinue(unsigned voice, float x,float y, float z) = 0;
    virtual void touchOff(unsigned voice, float x,float y, float z) = 0;
};



////////////////////////////////////////////////
// TODO
// 1. voices not needed? as soundplane already does touch alloction, just need to detemine on and off
////////////////////////////////////////////////
class SoundplaneHandler : public ::SPLiteCallback {
public:
    // these are used to adjust velocity, they are highly dependent on hardware, to get the right feel, experiment!
    static constexpr float V_COUNT = 4; // samples to use for velocity , lets try 2-N,  (was 4)
    static constexpr float V_SCALE_AMT = 4.0f; // scale, to help V_COUNT pressures, quickly = max vel.  (was 4.0)
    static constexpr float V_CURVE_AMT = 4.0f; // a pow scaling, 1.0 = linear, < 1.0 = more sensitive,  > 1.0 = less sensitive (more firm pressure)  (was 4.0)

    SoundplaneHandler(Preferences &p, 
		    ICallback& cb)
            : prefs_(p),
              callback_(cb),
              valid_(true),
              voices_(static_cast<unsigned>(p.getInt("voices", Voices::NUM_VOICES)),
                      static_cast<unsigned>(p.getInt("velocity count", Voices::V_COUNT)),
                      static_cast<float>(p.getDouble("velocity curve", Voices::V_CURVE_AMT )),
                      static_cast<float>(p.getDouble("velocity scale", Voices::V_SCALE_AMT ))
                      ),
              stealVoices_(p.getBool("steal voices", true)),
              throttle_(p.getInt("throttle", 0) == 0
                        ? 0 : 1000000ULL /
                              p.getInt("throttle",
                                       0)) {
        if (valid_) {
            LOG_0("SoundplaneHandler enabling for mecapi");
        }
    }


    void onInit() override  {
        LOG_0("Soundplane initialised");
    }
    void onFrame() override {;}
    void onDeinit()override {;}
    void onError(unsigned err, const char *errStr) override {;}

    void touchOn(unsigned voice, float x, float y, float z) override {
        unsigned ix = unsigned(x);
        unsigned iy = unsigned(y);
        float fn = (ix + (iy * 4)) + (x -ix - 0.5f);
        float fx = (x-float(ix)-0.5f) * 2.0f;
        float fy = (y-float(iy)-0.5f) * 2.0f;
        float fz = z;
        //fprintf(stderr,"on %d %f %f %f, %f - %f %f %f \n", voice, x, y, z, fn, fx,fy,fz);

        touch(true, voice, fn, fx,fy,fz);
    }
    void touchContinue(unsigned voice, float x,float y, float z) override {
        unsigned ix = unsigned(x);
        unsigned iy = unsigned(y);
        float fn = (ix + (iy * 4)) + (x -ix + 0.5f);
        float fx = (x-float(ix)-0.5f) * 2.0f;
        float fy = (y-float(iy)-0.5f) * 2.0f;
        float fz = z;
        //fprintf(stderr,"cont %d %f %f %f, %f - %f %f %f \n", voice, x, y, z, fn, fx,fy,fz);

        touch(true, voice, fn, fx,fy,fz);
    }

    void touchOff(unsigned voice, float x,float y, float z) override {
        unsigned ix = unsigned(x);
        unsigned iy = unsigned(y);
        float fn = (ix + (iy * 4)) + (x -ix + 0.5f);
        float fx = (x-float(ix)-0.5f) * 2.0f;
        float fy = (y-float(iy)-0.5f) * 2.0f;
        float fz = 0.0f; //z;
        //fprintf(stderr,"off %d %f %f %f, %f - %f %f %f \n", voice, x, y, z, fn, fx,fy,fz);
        touch(false, voice, fn, fx,fy,fz);
    }

    bool isValid() { return valid_; }
//    virtual void device(const char *dev, int rows, int cols) {
//        LOG_1("SoundplaneHandler  device d: " << dev);
//        LOG_1(" r: " << rows << " c: " << cols);
//    }

    void touch(bool a, int itouch, float n, float x, float y, float z) {
        static const unsigned int NOTE_CH_OFFSET = 1;

        unsigned key = (unsigned) itouch;
        Voices::Voice *voice = voices_.voiceId(key);
        float fn = n;
        float mn = note(fn);
        float mx = clamp(x, -1.0f, 1.0f);
        float my = clamp(y, -1.0f, 1.0f);
        float mz = clamp(z,  0.0f, 1.0f);
        unsigned long long t = 0;
        if(throttle_ > 0) {
            t = microtime();
        }
        if (a) {

            LOG_3("SoundplaneHandler key device d: " << dev << " a: " << a);
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
private:
    inline float clamp(float v, float mn, float mx) { return (std::max(std::min(v, mx), mn)); }

    float note(float n) { return n; }

    Preferences prefs_;
    ICallback &callback_;
    Voices voices_;
    bool valid_;
    bool stealVoices_;
    unsigned long long throttle_;
    std::set<unsigned> inactiveKeys_;
};


////////////////////////////////////////////////
Soundplane::Soundplane(ICallback &cb) :
        active_(false), callback_(cb) {
}

Soundplane::~Soundplane() {
    deinit();
}

bool Soundplane::init(void *arg) {
    Preferences prefs(arg);
    //prefs.print();
    unsigned maxtouch = static_cast<unsigned>(prefs.getInt("voices", 15));
    LOG_1("max voices : " << maxtouch);

    if (active_) {
        deinit();
    }
    active_ = false;

    device_ = std::unique_ptr<SPLiteDevice>(new SPLiteDevice());


    std::shared_ptr<::SPLiteCallback> callback
        = std::shared_ptr<::SPLiteCallback>(new SoundplaneHandler(prefs, callback_));
    device_->addCallback(callback);

    device_->start();
    device_->maxTouches(maxtouch);
    active_ = true;

    return active_;
}

bool Soundplane::process() {
    return device_->process();
}

void Soundplane::deinit() {
    LOG_0("Soundplane::deinit");
    if (!device_) return;
    LOG_0("Soundplane::reset model");
    device_->stop();
    device_.reset();
    active_ = false;
}

bool Soundplane::isActive() {
    return active_;
}


}

