#include <eigenfreed/eigenfreed.h>

#include "eigenfreed_impl.h"

#include <picross/pic_config.h>


#include <lib_pico/pico_usb.h>

#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>


#define DEFAULT_DEBOUNCE 25000

// these are all hardcode in eigend, rather than in pico_ubs :(

#define PICO_PRE_LOAD 0x0001
#define PICO_FIRMWARE "pico.ihx"

#define PRODUCT_ID_PICO BCTPICO_USBPRODUCT

#define STRIP_THRESH 50
#define STRIP_MIN 110
#define STRIP_MAX 3050



namespace EigenApi
{

// public interface

EF_Pico::EF_Pico(EigenFreeD& efd, const char* fwDir) : EF_Harp(efd, fwDir), pLoop_(NULL), delegate_(*this)
{
}

EF_Pico::~EF_Pico()
{
}

bool EF_Pico::create()
{
    logmsg("create eigenharp pico");
    if (!EF_Harp::create()) return false;
    
    try {
        logmsg("close device to allow active_t to open");
        usbDevice()->detach();
        usbDevice()->close();
        logmsg("create pico loop");
        pLoop_ = new pico::active_t(usbDevice()->name(), &delegate_);
        pLoop_->load_calibration_from_device();
        logmsg("created pico loop");
        efd_.fireDeviceEvent(usbDevice()->name(), Callback::DeviceType::PICO, 0, 0, 1, 0);

    } catch (pic::error& e) {
        // error is logged by default, so dont need to repeat, but useful if we want line number etc for debugging
        // logmsg(e.what());
        return false;
    }

    return true;
}

bool EF_Pico::destroy()
{
    logmsg("destroy pico....");
    EF_Harp::stop();
    {
//        pLoop_->stop();
        delete pLoop_;
        pLoop_=NULL;
    }
    logmsg("destroyed pico");
    return EF_Harp::destroy();
}


bool EF_Pico::start()
{
    if (!EF_Harp::start()) return false;

    if(pLoop_==NULL) return false;
    pLoop_->start();
    logmsg("started loop");
    return true;
}

bool EF_Pico::stop()
{
    if(pLoop_==NULL) return false;
//    pLoop_->stop(); //?? 
//    logmsg("stopped loop");
    
    return EF_Harp::stop();
}
    
void EF_Pico::restartKeyboard()
{
    if(pLoop_!=NULL)
    {
        logmsg("restarting pico keyboard....");
        pLoop_->stop();
        pLoop_->start();
    }
}
    
void  EF_Pico::fireKeyEvent(unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y)
{
    if (course > 0) {
        if (lastMode_[key] == p) return;
        lastMode_[key] = p;
    }
    
    EF_Harp::fireKeyEvent(t, course, key, a, p, r, y);
}
    

bool EF_Pico::poll(long long t)
{
    if (!EF_Harp::poll(t)) return false;
    pLoop_->poll(t);
    // pLoop_->msg_flush();
    return true;
}

void EF_Pico::setLED(unsigned int keynum,unsigned int colour)
{
    if(pLoop_==NULL) return;
    pLoop_->set_led(keynum, colour);
}

bool EF_Pico::loadPicoFirmware()
{
    std::string ihxFile;
    
	std::string usbdev = pic::usbenumerator_t::find(BCTPICO_USBVENDOR,PICO_PRE_LOAD,false).c_str();
	if(usbdev.size()==0)
	{
        pic::logmsg() << "no pico connected/powered on?";
        return false;
	}
    else
    {
        ihxFile = PICO_FIRMWARE;
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
    std::string fwfile = firmwareDir();
    fwfile.append(ihxFile);
    return loadFirmware(pDevice,fwfile);
}

std::string EF_Pico::findDevice()
{
    std::string usbdev = pic::usbenumerator_t::find(BCTPICO_USBVENDOR,PRODUCT_ID_PICO,false).c_str();
    
	if(usbdev.size()==0)
	{
		logmsg("pico loading firmware...");
		if(loadPicoFirmware())
		{
			logmsg("pico firmware loaded");
            
			for (int i=0;i<10 && usbdev.size()==0 ;i++)
			{
				logmsg("attempting to find pico...");
                
                // can take a few seconds for pico to reregister itself
                usbdev = pic::usbenumerator_t::find(BCTPICO_USBVENDOR,PRODUCT_ID_PICO,false).c_str();
                    
				pic_microsleep(1000000);
			}
            char buf[100];
            sprintf(buf,"pico loaded dev: %s ", usbdev.c_str());
			logmsg(buf);
		}
		else
		{
			logmsg("error loading pico");
		}
	}
	return usbdev;
}

bool EF_Pico::isAvailable()
{
    std::string usbdev;
	usbdev = pic::usbenumerator_t::find(BCTPICO_USBVENDOR,PICO_PRE_LOAD,false).c_str();
	if(usbdev.size()>0) return true;
    usbdev = pic::usbenumerator_t::find(BCTPICO_USBVENDOR,PRODUCT_ID_PICO,false).c_str();
	if(usbdev.size()>0) return true;
	return false;
}
    
//PicoDelegate
void EF_Pico::Delegate::kbd_dead(unsigned reason)
{
    if(!parent_.stopping()) parent_.restartKeyboard();
}
    
void EF_Pico::Delegate::kbd_key(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y)
{
	parent_.fireKeyEvent(t,0,key,a,p,r,y);
}

void EF_Pico::Delegate::kbd_raw(bool resync,const pico::active_t::rawkbd_t &)
{
}

    
void EF_Pico::Delegate::kbd_strip(unsigned long long t, unsigned s)
{
        if(--s_count_ != 0)
        {
            return;
        }

        s_count_ = 20;

        switch(s_state_)
        {
            case 0:
                if(s<s_threshold_)
                    break;

                //pic::logmsg() << "strip starting: " << s;
                s_state_ = 1;
                s_count_ = 100;
                break;

            case 1:
                if(s<s_threshold_)
                {
                    s_state_ = 0;
                }
                else
                {
                    //pic::logmsg() << "strip origin: " << s;
                    s_state_ = 2;
                    // origin_ = s;
                }
                break;

            case 2:
                // if(std::abs(long(s-last_))<200 && s>threshold_)
                // {
                //     int o = origin_;
                //     o -= s;
                //     float f = (float)o/(float)range_;
                //     float abs = (float)(s-min_)/(float)(range_/2.0f)-1.0f;
                //     if(abs>1.0f)
                //         abs=1.0f;
                //     if(abs<-1.0f)
                //         abs=-1.0f;
                //     //pic::logmsg() << "strip " << s << " -> " << f << "  range=" << range_ << "  abs=" << abs;
                //     output_.add_value(1,piw::makefloat_bounded_nb(1,-1,0,f,t));
                //     output_.add_value(2,piw::makefloat_bounded_nb(1,-1,0,abs,t));
                //     break;
                // }

                if(std::abs(long(s-s_last_))<200 && s>s_threshold_)
                {
                    parent_.fireStripEvent(t,1,s);
                }
                s_state_ = 3;
                s_count_ = 80;
                break;

            case 3:
                if(s<s_threshold_)
                {
                    //pic::logmsg() << "strip ending";
                    parent_.fireStripEvent(t,1,2048);
                    s_state_ = 0;
                }
                else
                {
                    s_state_ = 2;
                }
                break;
        }

        s_last_ = s;
}

    
void EF_Pico::Delegate::kbd_breath(unsigned long long t, unsigned b)
{
    parent_.fireBreathEvent(t,b);
}

void EF_Pico::Delegate::kbd_mode(unsigned long long t, unsigned key, unsigned m)
{
    parent_.fireKeyEvent(t,1,key - 18,m > 0,m,0,0);
}
    
} // namespace EigenApi

