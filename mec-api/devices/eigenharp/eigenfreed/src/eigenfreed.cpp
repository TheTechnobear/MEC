#include <eigenfreed/eigenfreed.h>

#include "eigenfreed_impl.h"

#include <picross/pic_config.h>

#define VERSION_STRING "eigenfreed v0.3 Alpha/Tau/Pico, experimental - Author: TheTechnobear"

#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>
#include <string.h>


namespace EigenApi
{


void EigenFreeD::logmsg(const char* msg)
{
    pic::logmsg() << msg;
}


// public interface

EigenFreeD::EigenFreeD(const char* fwDir) : fwDir_(fwDir),lastPollTime(0)
{
}

EigenFreeD::~EigenFreeD()
{
    destroy();
}

void EigenFreeD::addCallback(EigenApi::Callback* api)
{
    // do not allow callback to be added twice
    std::vector<Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        if(*iter==api)
        {
            return;
        }
    }
    callbacks_.push_back(api);
}

void EigenFreeD::removeCallback(EigenApi::Callback* api)
{
    std::vector<Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        if(*iter==api)
        {
            callbacks_.erase(iter);
            return;
        }
    }
}

void EigenFreeD::clearCallbacks()
{
    std::vector<Callback*>::iterator iter;
    while(!callbacks_.empty())
    {
        callbacks_.pop_back();
    }
}



bool EigenFreeD::create()
{
    logmsg(VERSION_STRING);
    logmsg("create EigenFreeD");
    pic_init_time();
    pic_set_foreground(true);
    if(EF_BaseStation::isAvailable()) 
    {
		EF_Harp *pDevice = new EF_BaseStation(*this, fwDir_);
        pDevice->create();
		devices_.push_back(pDevice);
    }
    if(EF_Pico::isAvailable()) 
    {
		EF_Harp *pDevice = new EF_Pico(*this, fwDir_);
        pDevice->create();
		devices_.push_back(pDevice);
    }
    return devices_.size() > 0;
}

bool EigenFreeD::destroy()
{
    logmsg("destroy EigenFreeD....");
    //? stop();
    bool ret = true;
    std::vector<EF_Harp*>::iterator iter;
	for(iter=devices_.begin();iter!=devices_.end();iter++)
	{    
		EF_Harp *pDevice = *iter;
		ret &= pDevice->destroy();
	}	
	devices_.clear();
    logmsg("destroyed EigenFreeD");
    return true;
}

bool EigenFreeD::start()
{
    bool ret = true;
    std::vector<EF_Harp*>::iterator iter;
	for(iter=devices_.begin();iter!=devices_.end();iter++)
	{    
		EF_Harp *pDevice = *iter;
		ret &= pDevice->start();
	}	
    return true;
}

bool EigenFreeD::stop()
{
    bool ret = true;
    std::vector<EF_Harp*>::iterator iter;
	for(iter=devices_.begin();iter!=devices_.end();iter++)
	{    
		EF_Harp* pDevice = *iter;
		ret &= pDevice->stop();
	}	

    clearCallbacks();
    return true;
}

bool EigenFreeD::poll(long uSleepTime,long minPollTime)
{
    long long t=pic_microtime();
    long long diff = t-lastPollTime;
    if(uSleepTime>0) {
        if(diff < uSleepTime && diff > 0) {
            pic_microsleep(diff);
        }
    }
    if(diff>minPollTime)
    {
        lastPollTime = t;
    	bool ret=true;
		std::vector<EF_Harp*>::iterator iter;
		for(iter=devices_.begin();iter!=devices_.end();iter++)
		{    
			EF_Harp *pDevice = *iter;
			ret &= pDevice->poll(uSleepTime);
		}	
        return ret;
    }
    return false;
}

    
void EigenFreeD::fireDeviceEvent(const char* dev, 
                                 Callback::DeviceType dt, int rows, int cols, int ribbons, int pedals)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
		EigenApi::Callback *cb=*iter;
		cb->device(dev, dt, rows, cols, ribbons, pedals);
	}
}
void EigenFreeD::fireKeyEvent(const char* dev,unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->key(dev, t, course, key, a, p, r, y);
    }
}
    
void EigenFreeD::fireBreathEvent(const char* dev, unsigned long long t, unsigned val)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->breath(dev, t, val);
    }
}
    
void EigenFreeD::fireStripEvent(const char* dev, unsigned long long t, unsigned strip, unsigned val)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->strip(dev, t, strip, val);
    }
}
    
void EigenFreeD::firePedalEvent(const char* dev, unsigned long long t, unsigned pedal, unsigned val)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->pedal(dev, t, pedal, val);
    }
}
    
void EigenFreeD::setLED(const char* dev, unsigned int keynum,unsigned int colour)
{
    std::vector<EF_Harp*>::iterator iter;
    for(iter=devices_.begin();iter!=devices_.end();iter++)
    {
        EF_Harp* pDevice = *iter;
        if(dev == NULL || dev == pDevice->name() || strcmp(dev,pDevice->name()) == 0) {
            pDevice->setLED(keynum,colour);
        }
    }
}


  
} // namespace EigenApi

