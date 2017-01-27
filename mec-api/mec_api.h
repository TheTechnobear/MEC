#ifndef MEC_API_H
#define MEC_API_H

#include <string>

class MecApi_Impl;

class IMecCallback {
public:
    virtual ~IMecCallback() {};
    virtual void touchOn(int touchId, float note, float x, float y, float z) = 0;
    virtual void touchContinue(int touchId, float note, float x, float y, float z) = 0;
    virtual void touchOff(int touchId, float note, float x, float y, float z) = 0;
    virtual void control(int ctrlId, float v) = 0;
};

class MecCallback : public IMecCallback {
public:
    virtual ~MecCallback() {};
    virtual void touchOn(int touchId, float note, float x, float y, float z) {};
    virtual void touchContinue(int touchId, float note, float x, float y, float z){};
    virtual void touchOff(int touchId, float note, float x, float y, float z) {};
    virtual void control(int ctrlId, float v) {};
};


class MecApi {
public:
    MecApi(const std::string& configFile = "./mec.json");
    ~MecApi();
    void init();
    void process();  // periodically call to process messages

    void subscribe(IMecCallback*);
    void unsubscribe(IMecCallback*);
private:
    MecApi_Impl* impl_;
};


#endif //MEC_API_H