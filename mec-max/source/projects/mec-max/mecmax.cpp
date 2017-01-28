
#include <mec_api.h>
#include "mecmax.h"

#include <queue>
//#include <mutex>


///////////////////////////////////////////////////////
/// this forms the C to C++ interface that mec api uses
////////////////////////////////////////////////////////

///TODO
// - removed mutex, so wont work ;)
// - replace touch messages with lockfree ringbuffer
// - replace touchmsg, perhaps use similar to mec, note... has to be C compatible!

struct MecData {
    MecApi* pMecApi;
    MecCallback* pCallback;
};


//std::mutex queue_mutex;
std::queue<struct TouchMsg*> TouchMessages;

void handledTouch(struct TouchMsg* k)
{
    delete k;
}

struct TouchMsg* nextTouchMsg()
{
//    std::lock_guard<std::mutex> lock(queue_mutex);
    
    if (TouchMessages.empty()) return NULL;
    
    struct TouchMsg *pM=TouchMessages.front();
    TouchMessages.pop();
    return pM;
}


class MecMaxCallback: public MecCallback
{
public:
    MecMaxCallback()
    {
    }
    ~MecMaxCallback()
    {
    }
    
    virtual void touchOn(int touchId, float note, float x, float y, float z)
    {
        struct TouchMsg* m= new struct TouchMsg;
        m->t=touchId;
        m->n=note;
        m->x=x;
        m->y=y;
        m->z=z;
        queuePush(m);
    }
    
    virtual void touchContinue(int touchId, float note, float x, float y, float z)
    {
        struct TouchMsg* m= new struct TouchMsg;
        m->t=touchId;
        m->n=note;
        m->x=x;
        m->y=y;
        m->z=z;
        queuePush(m);
    }
    
    virtual void touchOff(int touchId, float note, float x, float y, float z)
    {
        struct TouchMsg* m= new struct TouchMsg;
        m->t=touchId;
        m->n=note;
        m->x=x;
        m->y=y;
        m->z=z;
        queuePush(m);
    }
    
    virtual void control(int ctrlId, float v)
    {
        // not yet implemented
    }

private:
    void queuePush(TouchMsg* m)
    {
//        std::lock_guard<std::mutex> lock(queue_mutex);
        TouchMessages.push(m);
    }
};



void* createMec(void (*pLogFn)(const char*), const char* fwDir)
{
    //    MecApi::setLogFunc(pLogFn);
    
    MecData* pData = new MecData;
    pData->pMecApi = new MecApi(fwDir);
    pData->pCallback = new MecMaxCallback();
    return (void*) pData;
}

void destroyMec(void *impl)
{
    MecData *pData = static_cast<MecData*>(impl);
    pData->pMecApi->unsubscribe(pData->pCallback);

    if(pData->pCallback) delete pData->pCallback; pData->pCallback = 0;
    if(pData->pMecApi) delete pData->pMecApi; pData->pMecApi = 0;
    if(pData)delete pData;
    pData = 0;
    
//    MecApi::setLogFunc(nullprt);
}

void startMec(void *impl)
{
    if(!impl) return;
    
    MecData *pData = static_cast<MecData*>(impl);
    pData->pMecApi->subscribe(pData->pCallback);
    pData->pMecApi->init();
}

void stopMec(void *impl)
{
    if(!impl) return;

    MecData *pData =static_cast<MecData*>(impl);
    if(pData->pCallback) {
        pData->pMecApi->unsubscribe(pData->pCallback);
        delete pData->pCallback; pData->pCallback = 0;
    }

    for(TouchMsg *pMsg=nextTouchMsg();pMsg!=NULL;pMsg=nextTouchMsg())
    {
        delete pMsg;
    }
}

void pollMec(void *impl)
{
    MecApi *pMecApi=static_cast<MecData*>(impl)->pMecApi;
    pMecApi->process();
}

void setMecLed(void* impl,int k, int c)
{
//    MecApi *pMecApi=static_cast<MecData*>(impl)->pMecApi;
//    pMecApi->setLED((unsigned int)k, (unsigned int) c);
}




