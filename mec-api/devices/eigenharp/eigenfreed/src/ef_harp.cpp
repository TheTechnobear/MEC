#include <eigenfreed/eigenfreed.h>

#include "eigenfreed_impl.h"


#include <picross/pic_config.h>

#ifndef PI_WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif

#include <fcntl.h>

#ifdef PI_WINDOWS
#define IHX_OPENFLAGS (O_RDONLY|O_BINARY)
#else
#define IHX_OPENFLAGS (O_RDONLY)
#endif
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>


#define USB_TYPE_VENDOR 0x40
#define FIRMWARE_LOAD 0xa0
#define CPUCS_ADDR 0xe600
#define CONTROL_TIMEOUT 10000

#define FIRMWARE_DIR "../eigenharp/firmware/"


namespace EigenApi
{

EF_Harp::EF_Harp(EigenFreeD& efd, const char* fw) 
	: pDevice_(NULL),fwDir_(fw),efd_(efd), stopping_(false)
{
	;
}

EF_Harp::~EF_Harp() 
{
	destroy();
}

void EF_Harp::logmsg(const char* msg)
{
    pic::logmsg() << msg;
}
    
const char* EF_Harp::name()
{
    if(pDevice_ != NULL)
    {
        return pDevice_->name();
    }
    return NULL;
}


bool EF_Harp::create()
{
    logmsg("create EF_Harp");
    
    std::string usbdev = findDevice();
	if(usbdev.size()==0)
	{
		logmsg("unable to find device ");
		return false;
	}
    
    logmsg("found device");
    if(pDevice_!=NULL)
    {
        destroy();
    }
    
    try {
        pDevice_ = new pic::usbdevice_t(usbdev.c_str(),0);
        logmsg("created USB device");
    } catch (pic::error& e) {
        // error is logged by default, so dont need to repeat, but useful if we want line number etc for debugging
        //logmsg(e.what());
        return false;
    }

    return true;
}

bool EF_Harp::destroy()
{
    logmsg("destroy Eigenharp....");
  	stopping_ = true;
    if(pDevice_)
    {
        //pDevice_->detach();
        //pDevice_->close();
        delete pDevice_;
        pDevice_=NULL;
    }
    logmsg("destroyed Eigenharp");
    return true;
}

bool EF_Harp::start()
{
	stopping_ = false;
    return true;
}

bool EF_Harp::stop()
{
	stopping_ = true;
    return true;
}

bool EF_Harp::poll(long long t)
{
    return true;
}

void EF_Harp::fireKeyEvent(unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y)
{
	efd_.fireKeyEvent(pDevice_->name(), t, course, key, a, p, r, y);
}
    
void EF_Harp::fireBreathEvent(unsigned long long t, unsigned val)
{
    unsigned diff = lastBreath_ - val;
    if(diff > 10)
    {
        lastBreath_ = val;
        efd_.fireBreathEvent(pDevice_->name(),t, val);
    }
}
    
void EF_Harp::fireStripEvent(unsigned long long t, unsigned strip, unsigned val)
{
    unsigned diff = lastStrip_[strip - 1] - val;
    if(diff > 10)
    {
        lastStrip_[strip - 1]=val;
        efd_.fireStripEvent(pDevice_->name(),t, strip, val);
    }
}
    
void EF_Harp::firePedalEvent(unsigned long long t, unsigned pedal, unsigned val)
{
    unsigned diff = lastPedal_[pedal - 1] - val;
    if(diff > 10)
    {
        lastPedal_[pedal - 1]=val;
        efd_.firePedalEvent(pDevice_->name(),t, pedal, val);
    }
}

void EF_Harp::pokeFirmware(pic::usbdevice_t* pDevice,int address,int byteCount,void* data)
{
	pDevice->control_out(USB_TYPE_VENDOR,FIRMWARE_LOAD,address,0, data,byteCount,CONTROL_TIMEOUT);
    //	pic::logmsg() << "firmware poke type: "<< USB_TYPE_VENDOR << " req:" << FIRMWARE_LOAD << " address:" << address << " data:" << data << " byteCount:" << byteCount << " timeout:" << CONTROL_TIMEOUT;
}

void EF_Harp::firmwareCpucs(pic::usbdevice_t* pDevice,const char* mode)
{
	pDevice->control_out(USB_TYPE_VENDOR,FIRMWARE_LOAD,CPUCS_ADDR,0, mode,1,CONTROL_TIMEOUT);
    //	pic::logmsg() << "firmware cpucs type: "<< USB_TYPE_VENDOR << " req:" << FIRMWARE_LOAD << " address:" << CPUCS_ADDR << " data:" << mode << " byteCount:" << 1 << " timeout:" << CONTROL_TIMEOUT;
}

void EF_Harp::resetFirmware(pic::usbdevice_t* pDevice)
{
	static const char* on=new char(0x01);
	firmwareCpucs(pDevice,on);
}

void EF_Harp::runFirmware(pic::usbdevice_t* pDevice)
{
	static const char* off=new char(0x00);
	firmwareCpucs(pDevice,off);
}

void EF_Harp::sendFirmware(pic::usbdevice_t* pDevice,int recType,int address,int byteCount,void* data)
{
	switch(recType)
	{
        case 0 : // normal record
            pokeFirmware(pDevice,address,byteCount,data);
            break;
            
        case 1 : // eof
            
            break;
            
        default:
        {
            char buf[100];
            sprintf(buf,"invalid record type:  %x",recType);
            throw IHXException(buf);
        }
	}
}



unsigned EF_Harp::hexToInt(char* buf, int len)
{
    static const char* hexCharacters="0123456789ABCDEF";
	int value=0;
	for (int i=0;i<len;i++)
	{
		value*=16;
		const char* pos=strchr(hexCharacters,buf[i]);
		if(pos==NULL) throw IHXException("invalid hex character");
		int v=pos-hexCharacters;
		if(v<0 || v>15) throw IHXException("invalid hex character 2");
		value+=v;
	}
	return value;
}

char EF_Harp::hexToChar(char* buf)
{
	return (char) hexToInt(buf,2);
}

bool EF_Harp::processIHXLine(pic::usbdevice_t* pDevice,int fd,int line)
{
	bool isEof=false;
	std::string ouput;
	char startCh; // :
	char hexByteCount[2]; // 2 hex char
	char hexAddress[4];   // 4 hex char
	char hexRecType[2];   // 2 hex char
	char *hexData;        // byteCount of data
	char hexChecksum[2];  // 2 hex char
	char eol;           // 0a unix, 0d0a windows
    
	unsigned byteCount=0;
	unsigned recType=0;
	unsigned int address;
	char* data=0;
	unsigned char expectedChecksum=0;
	unsigned char checksum=0;
    
	if(read(fd,&startCh,1)<1) throw IHXException("unable process to start code (:)");
	if(startCh!=':')
    {
		char buf[100];
		sprintf(buf,"invalid start code (:) got  %x",startCh);
        throw IHXException(buf);
    }
	if(read(fd,&hexByteCount,2)<2) throw IHXException("unable process to byteCount");
	byteCount = hexToInt(hexByteCount,2);
	checksum += byteCount;
    
    
	if(read(fd,&hexAddress,4)<4) throw IHXException("unable process to address");
	unsigned high=hexToInt(hexAddress,2);
	unsigned low=hexToInt(hexAddress+2,2);
	checksum += high;
	checksum += low;
	address=high*0x100+low;
    
	if(read(fd,&hexRecType,2)<2) throw IHXException("unable process to recType");
	recType = hexToInt(hexRecType,2);
	if(recType==1)
	{
		isEof=true;
		if(byteCount>0) throw IHXException("byte count not zero for EOF record");
		if(high>0) throw IHXException("address not zero for EOF record");
		if(low>0) throw IHXException("address not zero for EOF record");
	}
    
	if(byteCount>0)
	{
		hexData = new char[byteCount*2];
		data = new char[byteCount];
		if(read(fd,hexData,byteCount*2)<int(byteCount*2)) throw IHXException("unable process to data");
		for(unsigned i=0;i<byteCount;i++)
		{
			data[i]=hexToChar(hexData+(i*2));
			checksum += data[i];
		}
        
		delete hexData;
	}
    
	if(read(fd,&hexChecksum,2)<2) throw IHXException("unable process to checksum");
	expectedChecksum=hexToChar(hexChecksum);
	unsigned char checkdigit= ! isEof ? (~checksum)+1 : 0xFF;
	if(expectedChecksum!=checkdigit)
	{
		char buf[100];
		sprintf(buf,"invalid checksum expected:%x, got %x, sum was %x",expectedChecksum,checkdigit,checksum);
    	throw IHXException(buf);
	}
    
	if(read(fd,&eol,1)<1) throw IHXException("unable process eol");
	if(eol==0x0d)
	{
		if(read(fd,&eol,1)<1) throw IHXException("unable process eol 2");
	}
	if(eol!=0x0a)
	{
		char buf[100];
		sprintf(buf,"invalid eol (0x0a) got:%x",eol);
    	throw IHXException(buf);
	}
	sendFirmware(pDevice,recType,address,byteCount,data);
    
	if (data!=0) delete data;
    
	return !isEof;
}



bool EF_Harp::loadFirmware(pic::usbdevice_t* pDevice,std::string ihxFile)
{
    std::string fwfile=ihxFile;
    // fwfile.append(ihxFile);
    int fd = pic::open(fwfile, IHX_OPENFLAGS);
    if(fd < 0)
    {
        fwfile=FIRMWARE_DIR;
        fwfile.append(ihxFile);
        fd = pic::open(fwfile, IHX_OPENFLAGS);
        if(fd < 0)
        {
            char buf[100];
            sprintf(buf,"unable to open IHX firmware: %s",fwfile.c_str());
            logmsg(buf);
            return false;
        }
    }
    pic::logmsg() << "using firmware " << fwfile;
    
	pDevice->start_pipes();
	int line=0;
    try
    {
    	resetFirmware(pDevice);
		while(processIHXLine(pDevice,fd,line))
		{
			line++;
		}
		runFirmware(pDevice);
    }
    catch (IHXException& e)
    {
		char buf[100];
		sprintf(buf,"error processing IHX firmware: %s, %i ",e.reason().c_str(),line);
    	logmsg(buf);
    	return false;
    }
    pDevice->detach();
    pDevice->close();
    delete pDevice;
    
	return true;
}

}
