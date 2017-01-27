
#ifndef MecMax_h
#define MecMax_h
/*
 * Provides both a C and C++ compatible interface
 *
 */


#ifdef __cplusplus
extern "C"
{
#endif
    struct TouchMsg
    {
        int t;
        float n;
        float x;
        float y;
        float z;
    };
    
    struct TouchMsg* nextTouchMsg();
    void handledTouch(struct TouchMsg* k);
    
    void* createMec(void (*pLogFn)(const char*), const char* fwDir);
    void destroyMec(void*);
    void stopMec(void*);
    void startMec(void*);
    void pollMec(void*);
    void setMecLed(void*,int, int);
    
#ifdef __cplusplus
}
#endif


#endif //MecMax_h 
