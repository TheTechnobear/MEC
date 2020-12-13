#include "mec_soundplane.h"


#include "mec_log.h"
#include "../mec_voice.h"
#include <set>

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
    SoundplaneHandler(Preferences &p, 
		    ICallback& cb)
            : prefs_(p),
              callback_(cb),
              valid_(true),
              voices_(static_cast<unsigned>(p.getInt("voices", 15))),
              stealVoices_(p.getBool("steal voices", true)),
              noteOffset_(p.getInt("note offset", 37)) {
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

        unsigned touch = (unsigned) itouch;
        Voices::Voice *voice = voices_.voiceId(touch);
        float fn = n;
        float mn = note(fn);
        float mx = clamp(x, -1.0f, 1.0f);
        float my = clamp(y, -1.0f, 1.0f);
        float mz = clamp(z,  0.0f, 1.0f);
        unsigned long long t = 0;

        if (a) {
            // LOG_1("SoundplaneHandler  touch device d: "   << dev      << " a: "   << a)
            // LOG_1(" touch: " <<  touch);
            // LOG_1(" note: " <<  n  << " mn: "   << mn << " fn: " << fn);
            // LOG_1(" x: " << x      << " y: "   << y    << " z: "   << z);
            // LOG_1(" mx: " << mx    << " my: "  << my   << " mz: "  << mz);
            if (!voice) {
                if (stolenTouches_.find(touch) != stolenTouches_.end()) {
                    // this key has been stolen, must be released to reactivate it
                    return;
                }

                voice = voices_.startVoice(touch);
                //LOG_1("start voice for " << touch << " ch " << voice->i_);

                if (!voice && stealVoices_) {
                    // no available voices, steal?
                    Voices::Voice *stolen = voices_.oldestActiveVoice();
                    callback_.touchOff(stolen->i_, stolen->note_, stolen->x_, stolen->y_, 0.0f);
                    voices_.stopVoice(stolen);

                    voice = voices_.startVoice(touch);
                }

                if (voice) {
                    // callback_.touchOn(voice->i_, mn, mx, my, voice->v_); //v_ = calculated velocity
                    callback_.touchOn(voice->i_, mn, mx, my, mz); 
                    voice->note_ = mn;
                    voice->x_ = mx;
                    voice->y_ = my;
                    voice->z_ = mz;
                    voice->t_ = t;
                }
            } else {
        		callback_.touchContinue(voice->i_, mn, mx, my, mz);
                voice->note_ = mn;
                voice->x_ = mx;
                voice->y_ = my;
                voice->z_ = mz;
                voice->t_ = t;
            }

        } else {
            if (voice) {
                //LOG_1("stop voice for " << touch << " ch " << voice->i_ );
                //msg.type_ = MecMsg::TOUCH_OFF;
                //msg.data_.touch_.touchId_ = voice->i_;
                //msg.data_.touch_.z_ = 0.0;
                //queue_.addToQueue(msg);
                callback_.touchOff(voice->i_, mn, mx, my, mz);
                voices_.stopVoice(voice);
            }
            stolenTouches_.erase(touch);
        }
    }
private:
    inline float clamp(float v, float mn, float mx) { return (std::max(std::min(v, mx), mn)); }

    float note(float n) { return n+noteOffset_; }

    Preferences prefs_;
    ICallback &callback_;
    Voices voices_;
    bool valid_;
    bool stealVoices_;
    std::set<unsigned> stolenTouches_;
    int noteOffset_;
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

