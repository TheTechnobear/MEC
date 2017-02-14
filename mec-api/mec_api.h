#ifndef MEC_API_H
#define MEC_API_H

#include <string>


namespace mec {

class MecApi_Impl;

class ICallback {
public:
    enum MecControl {
        SHUTDOWN
    };

    virtual ~ICallback() {};
    virtual void touchOn(int touchId, float note, float x, float y, float z) = 0;
    virtual void touchContinue(int touchId, float note, float x, float y, float z) = 0;
    virtual void touchOff(int touchId, float note, float x, float y, float z) = 0;
    virtual void control(int ctrlId, float v) = 0;
    virtual void mec_control(int cmd, void* other) = 0;
};

class Callback : public ICallback {
public:
    virtual ~Callback() {};
    virtual void touchOn(int touchId, float note, float x, float y, float z) override {};
    virtual void touchContinue(int touchId, float note, float x, float y, float z) override {};
    virtual void touchOff(int touchId, float note, float x, float y, float z) override  {};
    virtual void control(int ctrlId, float v) override  {};
    virtual void mec_control(int cmd, void* other) override  {};
};

//////////////////////////////////////////
// new experimental surface api
//////////////////////////////////////////



typedef std::string SurfaceID;


// represents a single touch on a surface
// each active touch has a unique id, which is reused, often relates to a 'voice'
// x/y continuous across the entire surface
// r/c the surface is split into cells, defined by a row/column, the row can be considerd like a string on a stringed instrument
// the column is the position along that string (like a fret position , but fretless ;) 
// there is therefore a relationship between X-C , and R-Y

// touches originate from a device, and then are passed thru surfaces to allow there coordinates to be translated.
// a simple exampe is a device surfaces may be 'split' into 2 halfs, a 'split surface' will take the device touches and translate into touches for that 
// split... to the application these touches will be the same as if they came from different devices
struct Touch {
    Touch() {
        ;
    }
    Touch(int id, SurfaceID surface, float x, float y, float z, float r, float c) :
        id_(id), surface_(surface),
        x_(x), y_(y), z_(z),
        r_(r), c_(c) {

    }

    int   id_;
    SurfaceID   surface_;

    float x_; // typically pitch axis
    float y_; // typically timbre axis
    float z_; // typically pressure axis

    float r_; // string, sames axis as y.. but often used for pitch offsets (e.g 4ths), then Y is within this axis
    float c_; // pitch along string, usually proportional to x.

};

class ISurfaceCallback {
public:
    virtual void touchOn(const Touch&) = 0;
    virtual void touchContinue(const Touch&) = 0;
    virtual void touchOff(const Touch&) = 0;
};


// a musical touch, is a touch that has been converted into a pitched note using a scaler
struct MusicalTouch : public Touch {
    MusicalTouch() {
        ;
    }

    MusicalTouch(const Touch& t, float note) :
        Touch(t.id_, t.surface_, t.x_, t.y_, t.z_, t.r_, t.c_),
        note_(note)  {
        ;
    }

    float note_;
};

class IMusicalCallback {
public:
    virtual void touchOn(const MusicalTouch&) = 0;
    virtual void touchContinue(const MusicalTouch&) = 0;
    virtual void touchOff(const MusicalTouch&) = 0;
};


class MecApi {
public:
    MecApi(const std::string& configFile = "./mec.json");
    ~MecApi();
    void init();
    void process();  // periodically call to process messages

    void subscribe(ICallback*);
    void unsubscribe(ICallback*);

    void subscribe(ISurfaceCallback*);
    void unsubscribe(ISurfaceCallback*);

    void subscribe(IMusicalCallback*);
    void unsubscribe(IMusicalCallback*);

private:
    MecApi_Impl* impl_;
};

}

#endif //MEC_API_H