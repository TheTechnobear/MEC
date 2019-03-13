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
    SoundplaneHandler(Preferences &p, MsgQueue &q)
            : prefs_(p),
              queue_(q),
              valid_(true),
              voices_(static_cast<unsigned>(p.getInt("voices", 15))),
              stealVoices_(p.getBool("steal voices", true)) {
        if (valid_) {
            LOG_0("SoundplaneHandler enabling for mecapi");
        }
    }


    void onInit() override  {;}
    void onFrame() override {;}
    void onDeinit()override {;}
    void onError(unsigned err, const char *errStr) override {;}

    void touchOn(unsigned voice, float x, float y, float z) override {
        unsigned ix = unsigned(x);
        unsigned iy = unsigned(y);
        unsigned fn = (ix + y * 4) + (x -ix);

        touch(true, voice, fn, x-ix, y-iy, z );
    }
    void touchContinue(unsigned voice, float x,float y, float z) override {
        unsigned ix = unsigned(x);
        unsigned iy = unsigned(y);
        unsigned fn = (ix + y * 4) + (x -ix);

        touch(true, voice, fn, x-ix, y-iy, z );

    }

    void touchOff(unsigned voice, float x,float y, float z) override {
        unsigned ix = unsigned(x);
        unsigned iy = unsigned(y);
        unsigned fn = (ix + y * 4) + (x -ix);

        touch(false, voice, fn, x-ix, y-iy, z );
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
        float my = clamp((y - 0.5f) * 2.0f, -1.0f, 1.0f);
        float mz = clamp(z, 0.0f, 1.0f);
        unsigned long long t = 0;

        MecMsg msg;
        msg.type_ = MecMsg::TOUCH_OFF;
        msg.data_.touch_.touchId_ = -1;
        msg.data_.touch_.note_ = mn;
        msg.data_.touch_.x_ = mx;
        msg.data_.touch_.y_ = my;
        msg.data_.touch_.z_ = mz;
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
                // LOG_2(std::cout << "start voice for " << key << " ch " << voice->i_ << std::endl;)

                if (!voice && stealVoices_) {
                    // no available voices, steal?
                    Voices::Voice *stolen = voices_.oldestActiveVoice();

                    MecMsg stolenMsg;
                    stolenMsg.type_ = MecMsg::TOUCH_OFF;
                    stolenMsg.data_.touch_.touchId_ = stolen->i_;
                    stolenMsg.data_.touch_.note_ = stolen->note_;
                    stolenMsg.data_.touch_.x_ = stolen->x_;
                    stolenMsg.data_.touch_.y_ = stolen->y_;
                    stolenMsg.data_.touch_.z_ = 0.0f;
                    stolenTouches_.insert((unsigned) stolen->id_);
                    queue_.addToQueue(stolenMsg);
                    voices_.stopVoice(stolen);

                    voice = voices_.startVoice(touch);
                }

                if (voice) {
                    msg.type_ = MecMsg::TOUCH_ON;
                    msg.data_.touch_.touchId_ = voice->i_;
                    queue_.addToQueue(msg);
                    voice->note_ = mn;
                    voice->x_ = mx;
                    voice->y_ = my;
                    voice->z_ = mz;
                    voice->t_ = t;
                }
            } else {
                msg.type_ = MecMsg::TOUCH_CONTINUE;
                msg.data_.touch_.touchId_ = voice->i_;
                queue_.addToQueue(msg);
                voice->note_ = mn;
                voice->x_ = mx;
                voice->y_ = my;
                voice->z_ = mz;
                voice->t_ = t;
            }

        } else {
            if (voice) {
                // LOG_2("stop voice for " << touch << " ch " << voice->i_ );
                msg.type_ = MecMsg::TOUCH_OFF;
                msg.data_.touch_.touchId_ = voice->i_;
                msg.data_.touch_.z_ = 0.0;
                queue_.addToQueue(msg);
                voices_.stopVoice(voice);
            }
            stolenTouches_.erase(touch);
        }
    }

//    virtual void control(const char *dev, unsigned long long t, int id, float val) {
//        MecMsg msg;
//        msg.type_ = MecMsg::CONTROL;
//        msg.data_.control_.controlId_ = id;
//        msg.data_.control_.value_ = clamp(val, -1.0f, 1.0f);
//        queue_.addToQueue(msg);
//    }

private:
    inline float clamp(float v, float mn, float mx) { return (std::max(std::min(v, mx), mn)); }

    float note(float n) { return n; }

    Preferences prefs_;
    MsgQueue &queue_;
    Voices voices_;
    bool valid_;
    bool stealVoices_;
    std::set<unsigned> stolenTouches_;
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

    if (active_) {
        deinit();
    }
    active_ = false;

    device_ = std::unique_ptr<SPLiteDevice>(new SPLiteDevice());
    device_->maxTouches(4);


    std::shared_ptr<::SPLiteCallback> callback
        = std::shared_ptr<::SPLiteCallback>(new SoundplaneHandler(prefs, queue_));
    device_->addCallback(callback);

    device_->start();
    active_ = true;

    return active_;
}

bool Soundplane::process() {
    return queue_.process(callback_);
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

