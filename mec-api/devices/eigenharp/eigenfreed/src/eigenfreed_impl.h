#ifndef EigenFreeD_Impl_h
#define EigenFreeD_Impl_h

#include <vector>
#include <picross/pic_usb.h>
#include <lib_alpha2/alpha2_active.h>
#include <lib_pico/pico_active.h>
#include <memory>

namespace EigenApi
{
	class EF_Harp;
	
    class EigenFreeD {
    public:
        EigenFreeD(const char* fwDir);
        virtual ~EigenFreeD();

        void addCallback(Callback* api);
        void removeCallback(Callback* api);
        void clearCallbacks();
        virtual bool create();
        virtual bool destroy();
        virtual bool start();
        virtual bool stop();
        virtual bool poll(long uSleep,long minPollTime);

        void setLED(const char* dev, unsigned int keynum,unsigned int colour);

		// logging
        static void logmsg(const char* msg);
        
        virtual void fireDeviceEvent(const char* dev, Callback::DeviceType dt, int rows, int cols, int ribbons, int pedals);
		virtual void fireKeyEvent(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y);
        virtual void fireBreathEvent(const char* dev, unsigned long long t, unsigned val);
        virtual void fireStripEvent(const char* dev, unsigned long long t, unsigned strip, unsigned val);
        virtual void firePedalEvent(const char* dev, unsigned long long t, unsigned pedal, unsigned val);

    private:
        const char* fwDir_;
        long long lastPollTime;
        std::vector<Callback*> callbacks_;
        std::vector<EF_Harp*> devices_;
    };

    class EF_Harp
    {
    public:
        
        EF_Harp(EigenFreeD& efd, const char* fwDir);
        virtual ~EF_Harp();
        
        const char* name();
        virtual bool create();
        virtual bool destroy();
        virtual bool start();
        virtual bool stop();
        virtual bool poll(long long t);

        bool stopping() { return stopping_;}
        
		virtual void fireKeyEvent(unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y);
        virtual void fireBreathEvent(unsigned long long t, unsigned val);
        virtual void fireStripEvent(unsigned long long t, unsigned strip, unsigned val);
        virtual void firePedalEvent(unsigned long long t, unsigned pedal, unsigned val);
        
        virtual void restartKeyboard() = 0;
        virtual void setLED(unsigned int keynum,unsigned int colour) = 0;

        pic::usbdevice_t* usbDevice() { return pDevice_;}

        std::string firmwareDir() { return fwDir_;}

        static bool loadFirmware(pic::usbdevice_t* pDevice,std::string ihxFile);
        static void logmsg(const char* msg);

        EigenFreeD& efd_;
protected:
        virtual std::string findDevice() = 0;
private:        
        // for controlling firmware loading
        static void pokeFirmware(pic::usbdevice_t* pDevice,int address,int byteCount,void* data);
        static void firmwareCpucs(pic::usbdevice_t* pDevice,const char* mode);
        static void resetFirmware(pic::usbdevice_t* pDevice);
        static void runFirmware(pic::usbdevice_t* pDevice);
        static void sendFirmware(pic::usbdevice_t* pDevice,int recType,int address,int byteCount,void* data);
        
        // for IHX processing
        static unsigned hexToInt(char* buf, int len);
        static char hexToChar(char* buf);
        static bool processIHXLine(pic::usbdevice_t* pDevice,int fd,int line);
        

        class IHXException
        {
        public:
            IHXException(const std::string& reason) : reason_(reason) {}
            const std::string& reason() { return reason_;}
        private:
            std::string reason_;
        };
        
        pic::usbdevice_t* pDevice_;
        std::string fwDir_;
        unsigned lastBreath_;
        unsigned lastStrip_[2];
        unsigned lastPedal_[4];
        bool stopping_;
    };
    
    
    class EF_Pico : public EF_Harp
    {
    public:
        EF_Pico(EigenFreeD& efd, const char* fwDir);
        virtual ~EF_Pico();
        
        virtual bool create();
        virtual bool destroy();
        virtual bool start();
        virtual bool stop();
        virtual bool poll(long long t);
        
        virtual void restartKeyboard();

        virtual void setLED(unsigned int keynum,unsigned int colour);
        virtual void fireKeyEvent(unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y);
        
		static bool isAvailable();

    private:
        std::string findDevice();
        bool loadPicoFirmware();
        pico::active_t *pLoop_;
        unsigned lastMode_[4];

        
    class Delegate : public pico::active_t::delegate_t
        {
        public:
        	Delegate(EF_Pico& p) : parent_(p) {;}
            void kbd_dead(unsigned reason);
            void kbd_raw(bool resync,const pico::active_t::rawkbd_t &);
            void kbd_key(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y);
            void kbd_strip(unsigned long long t, unsigned s);
            void kbd_breath(unsigned long long t, unsigned b);
            void kbd_mode(unsigned long long t, unsigned key, unsigned m);
        private:
            EF_Pico& parent_;
        } delegate_;
    };


    class EF_BaseStation : public EF_Harp
    {
    public:
        EF_BaseStation(EigenFreeD& efd, const char* fwDir);
        virtual ~EF_BaseStation();
        
        virtual bool create();
        virtual bool destroy();
        virtual bool start();
        virtual bool stop();
        virtual bool poll(long long t);
        
        virtual void restartKeyboard();
        virtual void setLED(unsigned int keynum,unsigned int colour);

        void fireAlphaKeyEvent(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y);

        static bool isAvailable();

        unsigned short* curMap() { return curmap_;}
        unsigned short* skpMap() { return skpmap_;}

    protected:
        alpha2::active_t* loop() { return pLoop_;}
    private:
        std::string findDevice();
        bool loadBaseStation();
        std::shared_ptr<alpha2::active_t::delegate_t> delegate_; 
        alpha2::active_t *pLoop_;
        unsigned short curmap_[9],skpmap_[9];
    };

    class EF_Alpha : public alpha2::active_t::delegate_t
    {
    public:
        EF_Alpha(EF_BaseStation& p) : parent_(p) {;}

        //alpha2::active_t::delegate_t
        void kbd_dead(unsigned reason);
        void kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4);
        void kbd_mic(unsigned char s,unsigned long long t, const float *samples);
        void kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y);
        void kbd_keydown(unsigned long long t, const unsigned short *bitmap);
        void pedal_down(unsigned long long t, unsigned pedal, unsigned p);
        void midi_data(unsigned long long t, const unsigned char *data, unsigned len);
            
    private:
        void fireAlphaKeyEvent(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y);
        EF_BaseStation& parent_;
    };

    class EF_Tau : public alpha2::active_t::delegate_t
    {
    public:
        EF_Tau(EF_BaseStation& p) : parent_(p) {;}

        //alpha2::active_t::delegate_t
        void kbd_dead(unsigned reason);
        void kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4);
        void kbd_mic(unsigned char s,unsigned long long t, const float *samples);
        void kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y);
        void kbd_keydown(unsigned long long t, const unsigned short *bitmap);
        void pedal_down(unsigned long long t, unsigned pedal, unsigned p);
        void midi_data(unsigned long long t, const unsigned char *data, unsigned len);
            
    private:
        void fireTauKeyEvent(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y);
        EF_BaseStation& parent_;
    };
}

#endif
