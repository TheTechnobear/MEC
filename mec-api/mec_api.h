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
    virtual void touchOn(int touchId, float note, float x, float y, float z) {};
    virtual void touchContinue(int touchId, float note, float x, float y, float z){};
    virtual void touchOff(int touchId, float note, float x, float y, float z) {};
    virtual void control(int ctrlId, float v) {};
    virtual void mec_control(int cmd, void* other) {};
};

//////////////////////////////////////////
// new experimental surface api
//////////////////////////////////////////


struct Touch {
    int   id_;
    int   surface_;

    float r_;
    float c_;

    float x_;
    float y_;
    float z_;
};

class ISurfaceCallback {
public:
    virtual void touchOn(const Touch&) = 0;
    virtual void touchContinue(const Touch&) = 0;
    virtual void touchOff(const Touch&) = 0;
};

struct MusicalTouch : public Touch {
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