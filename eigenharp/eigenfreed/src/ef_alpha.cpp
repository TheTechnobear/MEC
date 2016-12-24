#include <eigenfreed/eigenfreed.h>

#include "eigenfreed_impl.h"

#include <picross/pic_config.h>


#include <lib_alpha2/alpha2_usb.h>

#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>


#define DEFAULT_DEBOUNCE 25000

// max poll in uS (1000=1ms)
#define MAX_POLL_TIME 100

// these are all hardcode in eigend, rather than in alpha2_usb :(
#define PRODUCT_ID_BSP BCTKBD_USBPRODUCT
#define PRODUCT_ID_PSU 0x0105

#define BASESTATION_PRE_LOAD 0x0002
#define BASESTATION_FIRMWARE "bs_mm_fw_0103.ihx"

#define PSU_PRE_LOAD 0x0003
#define PSU_FIRMWARE "psu_mm_fw_0102.ihx"

namespace EigenApi
{



// public interface

EF_Alpha::EF_Alpha(EigenFreeD& efd, const char* fwDir) : EF_Harp(efd,fwDir), pLoop_(NULL), delegate_(*this)
{
}

EF_Alpha::~EF_Alpha()
{
}


bool EF_Alpha::create()
{
    logmsg("create eigenharp alpha");
    if (!EF_Harp::create()) return false;
    
    try {
        logmsg("create alpha loop");
        pLoop_ = new alpha2::active_t(pDevice_, &delegate_,false);
        logmsg("created alpha loop");
        efd_.fireDeviceEvent(pDevice_->name(), Callback::DeviceType::ALPHA, 0, 0, 2, 4);
    } catch (pic::error& e) {
        // error is logged by default, so dont need to repeat, but useful if we want line number etc for debugging
        // logmsg(e.what());
        return false;
    }

    return true;
}

bool EF_Alpha::destroy()
{
    logmsg("destroy alpha....");
    if(pLoop_)
    {
//        pLoop_->stop();
        delete pLoop_;
        pLoop_=NULL;
    }
    logmsg("destroyed alpha");
    return EF_Harp::destroy();
}


bool EF_Alpha::start()
{
    if (!EF_Harp::start()) return false;

    if(pLoop_==NULL) return false;
    pLoop_->start();
    pLoop_->debounce_time(DEFAULT_DEBOUNCE);
    logmsg("started alpha loop");
    return true;
}

bool EF_Alpha::stop()
{
    if(pLoop_==NULL) return false;
//    pLoop_->stop();
//    logmsg("stopped loop");
    
    return EF_Harp::stop();
}

bool EF_Alpha::poll(long uSleepTime)
{
    if (!EF_Harp::poll(uSleepTime)) return false;
    long long t=pic_microtime();
    pLoop_->poll(t);
    pLoop_->msg_flush();
    return true;
}

void EF_Alpha::setLED(unsigned int keynum,unsigned int colour)
{
    if(pLoop_==NULL) return;
    pLoop_->msg_set_led(keynum, colour);
}


void EF_Alpha::restartKeyboard()
{
    if(pLoop_!=NULL)
    {
        pLoop_->restart();
    }
}
    
void EF_Alpha::fireAlphaKeyEvent(unsigned long long t, unsigned key, unsigned p, int r, int y)
{
    unsigned course = key >= 120;
    EF_Harp::fireKeyEvent(t, course, key, isKeyDown(key),p, ( r - 2048) * 2, ( y - 2048 ) * 2);
}

bool EF_Alpha::isKeyDown(unsigned int k)
{
    return alpha2::active_t::keydown(k,keymap_);
}
    
void EF_Alpha::setKeymap(const unsigned short *bitmap)
{
    keymap_ = bitmap;
}

bool EF_Alpha::loadBaseStation()
{
    std::string ihxFile;
    
    
	std::string usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,BASESTATION_PRE_LOAD,false).c_str();
	if(usbdev.size()==0)
	{
        usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,PSU_PRE_LOAD,false).c_str();
        if (usbdev.size()==0)
        {
            pic::logmsg() << "no basestation connected/powered on?";
            return false;
        }
        ihxFile = PSU_FIRMWARE;
	}
    else
    {
        ihxFile = BASESTATION_FIRMWARE;
    }

    pic::usbdevice_t* pDevice;
	try
	{
		pDevice=new pic::usbdevice_t(usbdev.c_str(),0);
		pDevice->set_power_delegate(0);
	}
	catch(std::exception & e)
	{
		char buf[100];
		sprintf(buf,"unable to open device: %s ", e.what());
    	logmsg(buf);
		return false;
	}
    return loadFirmware(pDevice,ihxFile);
}

std::string EF_Alpha::findDevice()
{
    std::string usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,PRODUCT_ID_BSP,false).c_str();
    if(usbdev.size()==0) usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,PRODUCT_ID_PSU,false).c_str();
    
	if(usbdev.size()==0)
	{
		logmsg("basestation loading...");
		if(loadBaseStation())
		{
			logmsg("basestation loaded");
            
			for (int i=0;i<10 && usbdev.size()==0 ;i++)
			{
				logmsg("attempting to find basestation...");
                
                // can take a few seconds for basestation to reregister itself
				usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,PRODUCT_ID_BSP,false).c_str();
                if(usbdev.size()==0) usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,PRODUCT_ID_PSU,false).c_str();
                
				pic_microsleep(1000000);
			}
            char buf[100];
            sprintf(buf,"basestation loaded dev: %s ", usbdev.c_str());
			logmsg(buf);
		}
		else
		{
			logmsg("error loading basestation");
		}
	}
	return usbdev;
}

bool EF_Alpha::isAvailable()
{
    std::string usbdev;
	usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,BASESTATION_PRE_LOAD,false).c_str();
	if(usbdev.size()>0) return true;
    usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,PRODUCT_ID_BSP,false).c_str();
	if(usbdev.size()>0) return true;
	usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,PSU_PRE_LOAD,false).c_str();
	if(usbdev.size()>0) return true;
    usbdev = pic::usbenumerator_t::find(BCTKBD_USBVENDOR,PRODUCT_ID_PSU,false).c_str();
	if(usbdev.size()>0) return true;
	return false;
}
    

// EF_Alpha::Delegate

void EF_Alpha::Delegate::kbd_dead(unsigned reason)
{
	parent_.restartKeyboard();
}

void EF_Alpha::Delegate::kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y)
{
    if(key <KBD_KEYS) {
        parent_.fireAlphaKeyEvent(t,key,p,r,y);
    }
    
    switch(key) {
        case KBD_BREATH1 : {
            parent_.fireBreathEvent(t, p);
            break;
        }
        case KBD_STRIP1 : {
            parent_.fireStripEvent(t, 1, p);
            break;
        }
        case KBD_STRIP2 : {
            parent_.fireStripEvent(t, 2, p);
            break;
        }
//        case KBD_ACCEL : break;
//        case KBD_DESENSE : break;
        default: ;
    }
}

void EF_Alpha::Delegate::kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4)
{
}

void EF_Alpha::Delegate::kbd_keydown(unsigned long long t, const unsigned short *bitmap)
{
	parent_.setKeymap(bitmap);
}

void EF_Alpha::Delegate::kbd_mic(unsigned char s,unsigned long long t, const float *data)
{
}

void EF_Alpha::Delegate::midi_data(unsigned long long t, const unsigned char *data, unsigned len)
{
}

void EF_Alpha::Delegate::pedal_down(unsigned long long t, unsigned pedal, unsigned value)
{
    parent_.firePedalEvent(t, pedal, value);
}
    
    
    
} // namespace EigenApi

