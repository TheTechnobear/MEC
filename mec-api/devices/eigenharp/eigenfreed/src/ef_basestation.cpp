#include <eigenfreed/eigenfreed.h>

#include "eigenfreed_impl.h"

#include <picross/pic_config.h>


#include <lib_alpha2/alpha2_usb.h>

#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>

#include <memory.h>

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

EF_BaseStation::EF_BaseStation(EigenFreeD& efd, const char* fwDir) : 
    EF_Harp(efd,fwDir), pLoop_(NULL)
{
}

EF_BaseStation::~EF_BaseStation()
{
}


bool EF_BaseStation::create()
{
    logmsg("create basestation");
    if (!EF_Harp::create()) return false;
    
    try {
        memset(curmap_,0,sizeof(curmap_));
        memset(skpmap_,0,sizeof(skpmap_));

        // std::string bs_config  = usbDevice()->control_in(0x40|0x80,0xc2,0,0,64);
        std::string inst_config= usbDevice()->control_in(0x40|0x80,0xc6,0,0,64);
        delegate_ = nullptr;
        switch((long) inst_config[0]) {
            case 1:
                logmsg("ALPHA detected");
                delegate_ = std::shared_ptr<alpha2::active_t::delegate_t> (new EF_Alpha(*this));
                break; 
            case 2:
                logmsg("TAU detected");
                delegate_ = std::shared_ptr<alpha2::active_t::delegate_t> (new EF_Tau(*this));
                break; 
            default:
                logmsg("unknown instrumented detected, assume ALPHA");
                delegate_ = std::shared_ptr<alpha2::active_t::delegate_t> (new EF_Alpha(*this));
                break; 
        }
        logmsg("create basestation loop");
        pLoop_ = new alpha2::active_t(usbDevice(), delegate_.get(),false);
        logmsg("created basestation loop");


        switch((long) inst_config[0]) {
            case 1:
                efd_.fireDeviceEvent(usbDevice()->name(), Callback::DeviceType::ALPHA, 0, 0, 2, 4);
                break; 
            case 2:
                efd_.fireDeviceEvent(usbDevice()->name(), Callback::DeviceType::TAU, 0, 0, 1, 4);
                break;
            default:
                efd_.fireDeviceEvent(usbDevice()->name(), Callback::DeviceType::ALPHA, 0, 0, 2, 4);
                break;
        }


    } catch (pic::error& e) {
        // error is logged by default, so dont need to repeat, but useful if we want line number etc for debugging
        // logmsg(e.what());
        return false;
    }

    return true;
}

bool EF_BaseStation::destroy()
{
    logmsg("destroy basestation....");
    EF_Harp::stop();
    if(pLoop_)
    {
//        pLoop_->stop();
        delete pLoop_;
        pLoop_=NULL;
    }
    logmsg("destroyed basestation");
    return EF_Harp::destroy();
}


bool EF_BaseStation::start()
{
    if (!EF_Harp::start()) return false;

    if(pLoop_==NULL) return false;
    pLoop_->start();
    pLoop_->debounce_time(DEFAULT_DEBOUNCE);
    logmsg("started basestation loop");
    return true;
}

bool EF_BaseStation::stop()
{
    if(pLoop_==NULL) return false;
//    pLoop_->stop();
//    logmsg("stopped loop");
    
    return EF_Harp::stop();
}

bool EF_BaseStation::poll(long long t)
{
    if (!EF_Harp::poll(t)) return false;
    pLoop_->poll(t);
    pLoop_->msg_flush();
    return true;
}

void EF_BaseStation::setLED(unsigned int keynum,unsigned int colour)
{
    if(pLoop_==NULL) return;
    pLoop_->msg_set_led(keynum, colour);
}


void EF_BaseStation::restartKeyboard()
{
    if(pLoop_!=NULL)
    {
        pLoop_->restart();
    }
}
  

bool EF_BaseStation::loadBaseStation()
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

    std::string fwfile = firmwareDir();
    fwfile.append(ihxFile);
    return loadFirmware(pDevice,fwfile);
}

std::string EF_BaseStation::findDevice()
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

bool EF_BaseStation::isAvailable()
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
    
   
    
} // namespace EigenApi

