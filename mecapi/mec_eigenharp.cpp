#include "mec_eigenharp.h"

#include <iostream>

#include "mec_log.h"
#include "mec_prefs.h"
#include "mec_surfacemapper.h"
#include "mec_voice.h"

////////////////////////////////////////////////
class MecEigenharpHandler: public  EigenApi::Callback
{
public:
    MecEigenharpHandler(MecPreferences& p)
        :   prefs_(p),
            valid_(true)
    {
        if (valid_) {
            LOG_0(std::cout  << "MecEigenharpHandler enabling for mecapi" <<  std::endl;)
        }
    }

    bool isValid() { return valid_;}

    virtual void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals)
    {
        LOG_1(std::cout   << "eigenharp med device d: "  << dev << " dt: " <<  (int) dt)
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
    }

    virtual void breath(const char* dev, unsigned long long t, unsigned val)
    {
    }

    virtual void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val)
    {
    }

    virtual void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val)
    {
    }

private:
    int     note(unsigned key) { return mapper_.noteFromKey(key); }

    MecPreferences prefs_;
    MecSurfaceMapper mapper_;
    bool valid_;
};




////////////////////////////////////////////////
MecEigenharp::MecEigenharp() : active_(false) {
}
MecEigenharp::~MecEigenharp() {
    deinit();
}

bool MecEigenharp::init(void* arg) {
   MecPreferences prefs(arg);

   if (active_) {
        deinit();
    }
    active_ = false;

    eigenD_.reset(new EigenApi::Eigenharp("../eigenharp/resources/"));
    MecEigenharpHandler *pCb = new MecEigenharpHandler(prefs);
    if (pCb->isValid()) {
        eigenD_->addCallback(pCb);
        if (eigenD_->create()) {
            if (eigenD_->start()) {
                active_ = true;
            }
        }
    } else {
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
}

bool MecEigenharp::isActive() {
    return true;
}




