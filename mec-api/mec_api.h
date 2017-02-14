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


struct Touch {
    Touch() {
        ;
    }
    Touch(int id, int surface, float x, float y, float z, float r, float c) :
        id_(id), surface_(surface),
        x_(x), y_(y), z_(z),
        r_(r), c_(c) {

    }

    int   id_;
    int   surface_;

    float x_;
    float y_;
    float z_;

    float r_; // string 
    float c_; // fret

};

class ISurfaceCallback {
public:
    virtual void touchOn(const Touch&) = 0;
    virtual void touchContinue(const Touch&) = 0;
    virtual void touchOff(const Touch&) = 0;
};

struct MusicalTouch : public Touch {
    MusicalTouch() {
        ;
    }

    MusicalTouch(const Touch& t, float note, int scaler) :
        Touch(t.id_, t.surface_, t.x_, t.y_, t.z_, t.r_, t.c_),
        note_(note), scaler_(scaler)   {
        ;
    }

    float note_;
    int   scaler_;
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