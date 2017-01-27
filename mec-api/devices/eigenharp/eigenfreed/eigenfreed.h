#ifndef EigenFreeD_h
#define EigenFreeD_h

namespace EigenApi
{
    class Callback
    {
    public:
    	enum DeviceType {
    		PICO,
    		TAU,
    		ALPHA
    	};
    	virtual ~Callback() {};
    	
        virtual void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals) {};
        	
        virtual void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y) {};
        virtual void breath(const char* dev, unsigned long long t, unsigned val) {};
        virtual void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val) {};
        virtual void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val) {};
    };
    
    class Eigenharp
    {
    public:
        Eigenharp(const char* fwDir);
        virtual ~Eigenharp();
        bool create();
        bool destroy();

        bool start();
        bool stop();

        // min time betwen polling in uS (1000=1ms)
        bool poll(long uSleep,long minPollTime=100);
        
        // note: callback ownership is retained by caller
        void addCallback(Callback* api);
        void removeCallback(Callback* api);
        void clearCallbacks();
        
        void setLED(const char* dev, unsigned int keynum,unsigned int colour);

    private:
        void *impl;
    };
    
    class Logger
    {
    public:
        static void setLogFunc(void (*pLogFn)(const char*));
        static void logmsg(const char*);
    private:
        static void (*_logmsg)(const char*);
    };
}



#endif
