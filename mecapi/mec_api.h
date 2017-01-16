#ifndef MEC_API_H
#define MEC_API_H

#include <string>

class MecApi_Impl;

class MecCallback {
public:
    virtual ~MecCallback() {};
    virtual void touchOn(int touchId, int note, float x, float y, float z) = 0;
    virtual void touchContinue(int touchId, int note, float x, float y, float z) = 0;
    virtual void touchOff(int touchId, int note, float x, float y, float z) = 0;
    virtual void control(int ctrlId, float v) = 0;
};

class MecApi {
public:
    MecApi(const std::string& configFile = "./mec.json");
    ~MecApi();
    void init();
    void process();  // periodically call to process messages

    void subscribe(MecCallback*);
    void unsubscribe(MecCallback*);
private:
    MecApi_Impl* impl_;
};


#endif //MEC_API_H