#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <string>
#include <set>

#include <libusb-1.0/libusb.h>

#include <picross/pic_thread.h>
#include <picross/pic_error.h>
#include <picross/pic_log.h>
#include <picross/pic_usb.h>
#include <picross/pic_time.h>
#include <picross/pic_fastalloc.h>
#include <picross/pic_strbase.h>
#include <picross/pic_ilist.h>


// URB = request block
// each block has a number of packets , referred to (by eigend at least) as frames
// frame is expected to be delivered in a certain time, and is queued up if recvd early (as we want time consistent) 
// the interval per frame is nanoSec/8000 for high speed, and nanoSec/1000 for low speed (



// the following may need to be refined
// ALLOCATED_URBS_READ - we need extra urbs as the poll is responsible for pulling off recieve queue
// if this is slow then we will run out of URBS
// if we run out we 'steal' from the recieve queue, this means the client misses some messages but
// so issue a warning, but continue

// need to determine best numbers for these!
//#define URBS_PER_PIPE_READ    	    32
//#define FRAMES_PER_URB_READ         8
#define URBS_PER_PIPE_READ    	    16
#define FRAMES_PER_URB_READ         4
#define ALLOCATED_URBS_READ   	    32

#define URBS_PER_PIPE_WRITE         16
#define FRAMES_PER_URB_WRITE        8

#define HISPEED_INC (1.0/8.0)
#define LOSPEED_INC (1.0)

#define LOG_SINGLE(x) x

namespace
{
	struct usbpipe_out_t;
	struct usbpipe_in_t;

    // outbound buffer
    struct usbbuf_out_t: pic::element_t<0>, pic::element_t<1>, virtual pic::lckobject_t
    {
        usbbuf_out_t(usbpipe_out_t *pipe);
        ~usbbuf_out_t();

        usbpipe_out_t *pipe_;
        libusb_transfer* buffer_;
        unsigned size_;
        unsigned psize_;
    };

	
	// outbound pipe, with queue
	struct usbpipe_out_t: virtual pic::lckobject_t
	{
		usbpipe_out_t(pic::usbdevice_t::impl_t *dev, pic::usbdevice_t::iso_out_pipe_t *pipe);
		~usbpipe_out_t();

		// callback from libusb
		static void completed(libusb_transfer* transfer);
		// submit buffer to usb
		void submit(usbbuf_out_t *buf);

		void append_free_queue(usbbuf_out_t *buf);
		usbbuf_out_t *pop_free_queue();

		pic::mutex_t out_pipe_lock_;
		pic::usbdevice_t::iso_out_pipe_t *pipe_;
		pic::usbdevice_t::impl_t *device_;
		pic::ilist_t<usbbuf_out_t,0,true> buffer_list_;
		pic::ilist_t<usbbuf_out_t,1> free_queue_;
		unsigned piperef_;
		unsigned size_;
		libusb_device_handle *dhandle_;
	};
	
	// inbound buffer
    struct usbbuf_in_t: pic::element_t<0>, pic::element_t<1>, virtual pic::lckobject_t
    {
        usbbuf_in_t(usbpipe_in_t *pipe);
        ~usbbuf_in_t();

        usbpipe_in_t *pipe_;
        libusb_transfer *buffer_;
        
        unsigned size_;
        unsigned psize_;
        bool completed_;
        unsigned consumed_;
        unsigned long long time_;
        unsigned long long frame_;
    };

    
	// inbound pipe, with queue of data
	struct usbpipe_in_t: virtual pic::lckobject_t
	{
		usbpipe_in_t(pic::usbdevice_t::impl_t *dev, pic::usbdevice_t::iso_in_pipe_t *pipe);
		~usbpipe_in_t();

		// callback from libusb
		static void completed(libusb_transfer* transfer);
		// submit buffer to usb
		void submit(usbbuf_in_t *buf);
		bool poll_pipe(unsigned long long);

		void start();
		usbbuf_in_t *pop_free_queue();
		usbbuf_in_t *pop_receive_queue();
		void append_free_queue(usbbuf_in_t *buf);
		void append_receive_queue(usbbuf_in_t *buf);
		void prepend_receive_queue(usbbuf_in_t *buf);
		void allocate_free_queue();

				
		pic::mutex_t in_pipe_lock_;
		pic::usbdevice_t::iso_in_pipe_t *pipe_;
		
		pic::ilist_t<usbbuf_in_t,0,true> 	buffer_list_;
		pic::ilist_t<usbbuf_in_t,1> 		free_queue_;
		pic::ilist_t<usbbuf_in_t,1> 		receive_queue_;

		pic::usbdevice_t::impl_t *device_;

		unsigned piperef_;
		unsigned size_;
		libusb_device_handle *dhandle_;

		unsigned long long frame_;
		unsigned long long last_;
		unsigned long long expected_;

		bool died_;
		bool stolen_;
	};

};

// bulk pipe for immediate write
struct pic::usbdevice_t::bulk_out_pipe_t::impl_t: virtual pic::lckobject_t
{
    impl_t(pic::usbdevice_t::impl_t *dev,pic::usbdevice_t::bulk_out_pipe_t *pipe);
    ~impl_t();

    void bulk_write(const void *data, unsigned len, unsigned timeout);

    pic::usbdevice_t::impl_t *device_;
    pic::usbdevice_t::bulk_out_pipe_t *pipe_;
    unsigned piperef_;
    unsigned size_;
    libusb_device_handle *dhandle_;
};

struct pic::usbdevice_t::impl_t: pic::thread_t, virtual pic::lckobject_t
{
	impl_t(const char *, unsigned, pic::usbdevice_t *);
	~impl_t();

	bool poll_pipe(unsigned long long);
	bool add_iso_in(iso_in_pipe_t *p);
	void set_iso_out(iso_out_pipe_t *p);
	bool add_bulk_out(bulk_out_pipe_t *p);
	void start_pipes();
	void stop_pipes();
	void detach();
	void close();
	void thread_main();
	void thread_init();
	void pipes_died(unsigned reason);

	libusb_device_handle* open_usb_device(const char* name);
	
	libusb_context *usbcontext_;
	
	pic::usbdevice_t::power_t *power;
	std::string name_;

	
	pic::flipflop_t<pic::lcklist_t<usbpipe_in_t *>::lcktype> inpipes_;
	pic::usbdevice_t *device_;
	pic::flipflop_t<usbpipe_out_t *> pipe_out_;

	libusb_device_handle *dhandle_;
            
	pic::mutex_t  device_lock_;

	bool stopping_;
	bool died_;

	bool hispeed_;
	unsigned count_;
	bool opened_;
	float inc_;
};

// enumerate usb devices
struct pic::usbenumerator_t::impl_t: pic::thread_t, virtual pic::tracked_t, virtual pic::lckobject_t
{
    impl_t(unsigned short, unsigned short, const f_string_t &, const f_string_t &);
    ~impl_t();

    void thread_main();
    void thread_init();
    void thread_pass();
    void stop() { stop_=true; wait(); }

    unsigned short vendor_;
    unsigned short product_;
    f_string_t added_,removed_;
    pic::mutex_t enum_lock_;
    bool stop_;
    
	libusb_context *usbcontext_;

    std::set<std::string > devices_;
};


void buildUsbName(char* buf,unsigned short vendor, unsigned short product,unsigned short address, unsigned short bus)
{
	sprintf(buf,"%04hx.%04hx.%04hx.%04hx",vendor,product,address,bus);
}


// usbpipe_out
usbpipe_out_t::usbpipe_out_t(pic::usbdevice_t::impl_t *dev, pic::usbdevice_t::iso_out_pipe_t *pipe): 
		pipe_(pipe), device_(dev), 
		piperef_(pipe->out_pipe_name()), size_(pipe->out_pipe_size())
{
    unsigned urb_count = URBS_PER_PIPE_WRITE;

    for(unsigned i=0;i<urb_count;i++)
    {
        usbbuf_out_t *buf = new usbbuf_out_t(this);
        buffer_list_.append(buf);
        append_free_queue(buf);
    }
}

usbbuf_out_t *usbpipe_out_t::pop_free_queue()
{
    pic::mutex_t::guard_t guard(&out_pipe_lock_);
    usbbuf_out_t *buf = free_queue_.pop_front();
    return buf;
}

void usbpipe_out_t::append_free_queue(usbbuf_out_t* buf)
{
	pic::mutex_t::guard_t guard(&out_pipe_lock_);
    free_queue_.append(buf);
}

usbpipe_out_t::~usbpipe_out_t()
{
}


void usbpipe_out_t::submit(usbbuf_out_t *buf)
{
	if(device_->stopping_)
	{
		return;
	}

//	LOG_SINGLE(fprintf(stderr,"S"));
	int status=0;
	if((status=libusb_submit_transfer(buf->buffer_))>=0)
	{
		pic::mutex_t::guard_t guard(&(device_->device_lock_));
		device_->count_++;
		return;
	}

	device_->died_ = true;
	device_->stopping_ = true;
	pic::logmsg() << "usbpipe_out_t::submit failed : " << libusb_error_name(status) << " (" << status << ")";
}

void usbpipe_out_t::completed(libusb_transfer* transfer)
{
//	LOG_SINGLE(fprintf(stderr,"C"));
    usbbuf_out_t *buf = (usbbuf_out_t *)(transfer->user_data);
    buf->pipe_->append_free_queue(buf);
    
    {
    pic::mutex_t::guard_t guard(&(buf->pipe_->device_->device_lock_));
    buf->pipe_->device_->count_--;
    }
    
    int status = transfer->status;
    if(status!=LIBUSB_TRANSFER_COMPLETED)
    {
        pic::logmsg() << "usbpipe_out_t::completed not completed " << libusb_error_name(status) << " (" << status << ")";
    }
    else
    {
    	// we also need to check status of every packet to see if it was transferred
    	for(int i=0;i<transfer->num_iso_packets;i++)
    	{
    		libusb_iso_packet_descriptor desc=transfer->iso_packet_desc[i];
    		if(desc.status!=LIBUSB_TRANSFER_COMPLETED)
    		{
    	        pic::logmsg() << "usbpipe_out_t::completed not completed packet" << libusb_error_name(desc.status) << " (" << desc.status << ")" 
    	        			<< " len = " << desc.length << " actual= " << desc.actual_length;
    		}
    	}
    }
}

//usbbuf_out_t
usbbuf_out_t::usbbuf_out_t(usbpipe_out_t *pipe): pipe_(pipe)
{
	psize_ = pipe_->size_;
	size_ = pipe_->size_*FRAMES_PER_URB_WRITE;

	buffer_ = libusb_alloc_transfer(FRAMES_PER_URB_WRITE);
	
	buffer_->dev_handle = pipe_->device_->dhandle_;
	buffer_->endpoint = pipe_->piperef_;
	buffer_->flags = 0 ; //LIBUSB_TRANSFER_FREE_BUFFER;
	buffer_->type = LIBUSB_TRANSFER_TYPE_ISOCHRONOUS;
	buffer_->timeout = 0;
	buffer_->status = LIBUSB_TRANSFER_COMPLETED;
	buffer_->length = size_;
	buffer_->actual_length = 0;
	buffer_->callback = usbpipe_out_t::completed;
	buffer_->user_data = this;
	buffer_->num_iso_packets = FRAMES_PER_URB_WRITE;
	buffer_->buffer = (unsigned char *) pic::nb_malloc(PIC_ALLOC_LCK,buffer_->length);

}

usbbuf_out_t::~usbbuf_out_t()
{
    libusb_free_transfer(buffer_);
}



//iso_out_guard_t
pic::usbdevice_t::iso_out_guard_t::iso_out_guard_t(usbdevice_t *d): impl_(d->impl()), current_(0), guard_(0), dirty_(false)
{
    pic::flipflop_t<usbpipe_out_t *>::guard_t pipe_out(impl_->pipe_out_);

    usbpipe_out_t *p = pipe_out.value();

    if(p)
    {
    	usbbuf_out_t *u = p->pop_free_queue();

        if(u)
        {
//        	LOG_SINGLE(fprintf(stderr,"G"));
            guard_ = u;
            current_ = u->buffer_->buffer;

            for(int i=0;i<u->buffer_->num_iso_packets;i++)
            {
		u->buffer_->iso_packet_desc[i].length=u->psize_;
            }

            memset(current_,0,u->size_);
            dirty_ = false;
        }
        else
        {
            pic::logmsg() << "iso_out_guard_t::ctor: no buffers";
        }
    }

}

pic::usbdevice_t::iso_out_guard_t::~iso_out_guard_t()
{
    pic::flipflop_t<usbpipe_out_t *>::guard_t pipe_out(impl_->pipe_out_);
    usbpipe_out_t *p = pipe_out.value();
    usbbuf_out_t *u = (usbbuf_out_t *)guard_;

//	LOG_SINGLE(fprintf(stderr,"@"));
    if(p && u)
    {
//    	LOG_SINGLE(fprintf(stderr,"1"));
        if(dirty_)
        {
//        	LOG_SINGLE(fprintf(stderr,"2"));
            p->submit(u);
        }
        else
        {
            p->append_free_queue(u);
        }
    }
}

unsigned char *pic::usbdevice_t::iso_out_guard_t::advance()
{
    pic::flipflop_t<usbpipe_out_t *>::guard_t pipe_out(impl_->pipe_out_);
    usbpipe_out_t *p = pipe_out.value();
    usbbuf_out_t *u = (usbbuf_out_t *)guard_;

    if(p && u && dirty_)
    {
        p->submit(u);
        guard_ = 0;
        current_ = 0;

//    	LOG_SINGLE(fprintf(stderr,"A"));
        u = p->pop_free_queue();

        if(u)
        {
            guard_ = u;
            current_ = u->buffer_->buffer;

            for(int i=0;i<u->buffer_->num_iso_packets;i++)
            {
                u->buffer_->iso_packet_desc[i].length = u->psize_;
            }

            memset(current_,0,u->size_);
            dirty_ = false;
        }
        else
        {
            pic::logmsg() << "iso_out_guard_t::advance(): no buffers";
        }
    }

    return current_;
}


// usbpipe_in_t 

usbpipe_in_t::usbpipe_in_t(pic::usbdevice_t::impl_t *dev, pic::usbdevice_t::iso_in_pipe_t *pipe): 
		pipe_(pipe),device_(dev), piperef_(pipe->in_pipe_name()),
		size_(pipe->in_pipe_size()), frame_(0ULL),  died_(false),
		stolen_(false)
{
	LOG_SINGLE(pic::logmsg() << "pic::usbdevice_t::usbpipe_in_t::impl_t " << piperef_ ; )

	for(unsigned i=0;i<ALLOCATED_URBS_READ;i++)
    {
    	usbbuf_in_t *buf = new usbbuf_in_t(this);
    	buffer_list_.append(buf);
        free_queue_.append(buf);
    }
}


usbpipe_in_t::~usbpipe_in_t()
{
}



void usbpipe_in_t::submit(usbbuf_in_t *buf)
{
    if(device_->stopping_)
    {
//        LOG_SINGLE(fprintf(stderr,"!"));
        return;
    }
    int len = size_*FRAMES_PER_URB_READ;

    //LOG_SINGLE(fprintf(stderr,"S"));
    buf->consumed_ = 0;
    buf->frame_ = frame_;
    frame_ += FRAMES_PER_URB_READ;
    buf->buffer_->length=len;
    buf->buffer_->actual_length=0;
    for(int i=0;i<FRAMES_PER_URB_READ;i++)
    {
        buf->buffer_->iso_packet_desc[i].length = size_;
        buf->buffer_->iso_packet_desc[i].status = LIBUSB_TRANSFER_COMPLETED;
        buf->buffer_->iso_packet_desc[i].actual_length = 0;
    }

    int status;

    if((status=libusb_submit_transfer(buf->buffer_))>=0)
    {
        pic::mutex_t::guard_t guard(&(device_->device_lock_));;
        device_->count_++;
        return;
    }

    pic::logmsg() << "usbpipe_in_t::submit error " << libusb_error_name(status) << " (" << status << ")";
    device_->died_ = true;
    device_->stopping_ = true;
    device_->pipes_died(PIPE_UNKNOWN_ERROR);
}

usbbuf_in_t *usbpipe_in_t::pop_free_queue()
{
	pic::mutex_t::guard_t guard(&in_pipe_lock_);
    usbbuf_in_t *buf = free_queue_.pop_front();
    if(!buf)
	{
		if((buf = receive_queue_.pop_front()))
		{
			if(!stolen_)
			{
				stolen_ = true;
				pic::logmsg() << "usbpipe_in_t::pop_free_queue() stealing buffers";
			}
		}
	}
    return buf;
}

void usbpipe_in_t::append_free_queue(usbbuf_in_t *buf)
{
	pic::mutex_t::guard_t guard(&in_pipe_lock_);
    free_queue_.append(buf);
}


usbbuf_in_t *usbpipe_in_t::pop_receive_queue()
{
	pic::mutex_t::guard_t guard(&in_pipe_lock_);
    usbbuf_in_t *buf = receive_queue_.pop_front();
    return buf;
}

void usbpipe_in_t::prepend_receive_queue(usbbuf_in_t *buf)
{
	pic::mutex_t::guard_t guard(&in_pipe_lock_);
    receive_queue_.prepend(buf);
}

void usbpipe_in_t::append_receive_queue(usbbuf_in_t *buf)
{
    pic::mutex_t::guard_t guard(&in_pipe_lock_);
    receive_queue_.append(buf);
}

void usbpipe_in_t::completed(libusb_transfer* transfer)
{
    usbbuf_in_t *buf = (usbbuf_in_t *)(transfer->user_data);
    usbpipe_in_t *pipe = buf->pipe_;

    buf->time_ = pic_microtime();

    //const unsigned char *p = libusb_get_iso_packet_buffer(buf->buffer_,0);
    //printf("Buf %d [",pipe->piperef_);
    //for(int c=0;c<transfer->length;c++) printf("%02x",(unsigned) (*(p+c)));
    //printf("] %d %d %d\n",transfer->status, transfer->num_iso_packets,transfer->actual_length);

    // LOG_SINGLE(fprintf(stderr,"C"));
    int status = transfer->status;
    if(status!=LIBUSB_TRANSFER_COMPLETED)
    {
        pic::logmsg() << "usbpipe_in_t::completed unsuccessful " << libusb_error_name(status) << " (" << status << ")";
    }
    else
    {
	//if(transfer->length!=transfer->actual_length)
	//{
	//pic::logmsg() << "usbpipe_in_t::completed underfilled packet " << libusb_error_name(transfer->status) << " (" << transfer->status << ")" 
        //			<< " len = " << transfer->length << " actual= " << transfer->actual_length;
	//}
    	// we also need to check status of every packet to see if it was transferred
    	for(int i=0;i<transfer->num_iso_packets;i++)
    	{
    		libusb_iso_packet_descriptor desc=transfer->iso_packet_desc[i];
    		if(desc.status!=LIBUSB_TRANSFER_COMPLETED)
    		{
    	        	pic::logmsg() << "usbpipe_in_t::completed not completed packet" << libusb_error_name(desc.status) << " (" << desc.status << ")" 
    	        			<< " len = " << desc.length << " actual= " << desc.actual_length;
    		}
		//else if(desc.length!=desc.actual_length)
		//{
    	        //pic::logmsg() << "usbpipe_in_t::completed underfilled packet " << libusb_error_name(desc.status) << " (" << desc.status << ")" 
    	        // 		<< " len = " << desc.length << " actual= " << desc.actual_length;
		//}
    	}
    }

	pipe->append_receive_queue(buf);
	{
	    pic::mutex_t::guard_t guard(&(buf->pipe_->device_->device_lock_));
		pipe->device_->count_--;
	}
    if(!pipe->device_->stopping_)
	{
		if((buf = pipe->pop_free_queue()) != 0)
		{
			pipe->submit(buf);
			return;
		}
		pic::logmsg() << "usbpipe_in_t::completed free queue starved";
	}
}


void usbpipe_in_t::start()
{
	LOG_SINGLE(pic::logmsg() << "usbpipe_in_t::start()" ;  )
	frame_ = 0;
	expected_ = 0;
	last_ = 0;

	for(int i=0;i<URBS_PER_PIPE_READ;i++)
	{
		usbbuf_in_t *buf;

		if((buf = pop_free_queue()) != 0)
		{
			submit(buf);
		}
	}
}

bool usbpipe_in_t::poll_pipe(unsigned long long t)
{
//	LOG_SINGLE(pic::logmsg() << "usbpipe_in_t::poll_pipe()" ;  )
	usbbuf_in_t *buf = NULL;
    unsigned long long inc = device_->inc_;
    bool stolen = stolen_;
    stolen_=false;
				
	while((buf=pop_receive_queue()) != 0)
	{

	    //LOG_SINGLE(fprintf(stderr,"R"));

        if(buf->frame_ != expected_ && !stolen)  
        {
        	// note: we expect out of order frames if we have stolen from the recieve queue
        	LOG_SINGLE(pic::logmsg() << "poll_pipe frame out of order F:"<< buf->frame_ << " E:"<< expected_;)
        }

        while(buf->consumed_ < FRAMES_PER_URB_READ )
        {
            libusb_iso_packet_descriptor *h = &buf->buffer_->iso_packet_desc[buf->consumed_];
            const unsigned char *p = libusb_get_iso_packet_buffer(buf->buffer_,buf->consumed_);
            unsigned long long adj_time = buf->time_+inc*buf->consumed_;
            if(adj_time <= last_)
            {
                adj_time = last_+1;
            }

            if(t && adj_time >= t)
            {
//            	LOG_SINGLE(pic::logmsg() << "poll_pipe recv early T:"<< t<< " A:"<< adj_time;)
                expected_ = buf->frame_;
                prepend_receive_queue(buf);
                last_ = t-1;
                return false;//stolen;
            }
//        	LOG_SINGLE(pic::logmsg() << "poll_pipe DATA L:" << h->actual_length << " F: "<< buf->frame_<< " P:" << buf->consumed_ <<  " A:" << adj_time;)
			if(h->actual_length>0)
			{
            	pipe_->call_pipe_data(p,h->actual_length,buf->frame_+buf->consumed_,adj_time,t);
			}
            last_ = adj_time;
            buf->consumed_++;
        }

        expected_ = buf->frame_+FRAMES_PER_URB_READ;
	    // LOG_SINGLE(fprintf(stderr,"+"));
        append_free_queue(buf);
	}

    last_ = t-1;
    return false;//stolen;
}

//usbbuf_in_t
usbbuf_in_t::usbbuf_in_t(usbpipe_in_t *pipe): pipe_(pipe)
{
    psize_ = pipe_->size_;
    size_ = psize_*FRAMES_PER_URB_READ;

    buffer_ = libusb_alloc_transfer(FRAMES_PER_URB_READ);

    buffer_->dev_handle = pipe_->device_->dhandle_;
    buffer_->flags = 0; //LIBUSB_TRANSFER_FREE_BUFFER;
    buffer_->endpoint = pipe_->piperef_;
    buffer_->type = LIBUSB_TRANSFER_TYPE_ISOCHRONOUS;
    buffer_->timeout = 0;
    buffer_->status = LIBUSB_TRANSFER_COMPLETED;
    buffer_->length = size_;
    buffer_->actual_length = 0;
    buffer_->callback = usbpipe_in_t::completed;
    buffer_->user_data = this;
    buffer_->num_iso_packets = FRAMES_PER_URB_READ;
    buffer_->buffer = (unsigned char *) pic::nb_malloc(PIC_ALLOC_LCK,buffer_->length);

    for(int i=0;i<FRAMES_PER_URB_READ;i++)
    {
        buffer_->iso_packet_desc[i].length = psize_;
        buffer_->iso_packet_desc[i].status = LIBUSB_TRANSFER_COMPLETED;
        buffer_->iso_packet_desc[i].actual_length = 0;
    }

}

usbbuf_in_t::~usbbuf_in_t()
{
    libusb_free_transfer(buffer_);
}



// bulk_out_pipe_t
pic::usbdevice_t::bulk_out_pipe_t::impl_t::impl_t(pic::usbdevice_t::impl_t *dev,pic::usbdevice_t::bulk_out_pipe_t *pipe): device_(dev), pipe_(pipe), 
		piperef_(pipe->out_pipe_name()), size_(pipe->out_pipe_size()),dhandle_(dev->dhandle_)
{
    pipe_->impl_ = this;
}

pic::usbdevice_t::bulk_out_pipe_t::impl_t::~impl_t()
{
}

void pic::usbdevice_t::bulk_out_pipe_t::impl_t::bulk_write(const void *data, unsigned len, unsigned timeout)
{
	int bytesSent=0;
	int status=0;
	status=libusb_bulk_transfer(dhandle_,piperef_,(unsigned char *) data,(int) len,&bytesSent,timeout);
	if(status<0)
	{
        pic::logmsg() << "bulk_out_pipe_t::impl_t::bulk_write failed :" << libusb_error_name(status) << " (" << status << ")";
	}
}

int affinity()
{
    const char* e=getenv("PI_USB_THREAD_AFFINITY");
    if(e==NULL) return 0;
    return atoi(e);
}


//usbdevice_t::impl_t
pic::usbdevice_t::impl_t::impl_t(const char *name, unsigned iface, pic::usbdevice_t *dev) : 
		thread_t(PIC_THREAD_PRIORITY_REALTIME,affinity()), device_(dev), pipe_out_(0), stopping_(false), count_(0), opened_(false)
{
	// intialise libusb, open the device and claim the interface
	int status=0,speed=0;

	if(libusb_init(&usbcontext_)<0)
    {
    	pic::logmsg() << "pic::usbdevice_t::impl_t : cannot initialise libusb for " << name;
    	return;
    }
    
    dhandle_=open_usb_device(name);
    
    if(dhandle_== 0ULL) return;
    
//	libusb_set_detach_kernel_driver(dhandle_,1);
	status = libusb_claim_interface(dhandle_, iface);
	if (status != LIBUSB_SUCCESS) {
		pic::logmsg() << "pic::usbdevice_t::impl_t  claim_interface failed: %s\n", libusb_error_name(status);
		return;
	}
	opened_ = true;    

	// determine if high speed usb device
    speed = libusb_get_device_speed(libusb_get_device(dhandle_));

    hispeed_=true;
    if(speed != LIBUSB_SPEED_HIGH && speed != LIBUSB_SPEED_SUPER)
    {
	LOG_SINGLE(pic::logmsg() << "usbdevice opened low speed" ;  )
    	hispeed_=false;
    }
    else
    {
	LOG_SINGLE(pic::logmsg() << "usbdevice opened high speed" ;  )
    }
    inc_=hispeed_?HISPEED_INC:LOSPEED_INC;
 
	LOG_SINGLE(pic::logmsg() << "usbdevice opened successfully" ;  )

	return;
}

libusb_device_handle* pic::usbdevice_t::impl_t::open_usb_device(const char* name)
{
	libusb_device_handle *device=0ULL;
	libusb_device *dev, **devs;
	
	// here we have to look for the device that we found in the enumeration...
	// we need to take care as there may be more than one... im uniquely identifying in buildUsbName
	pic::logmsg() << "pic::usbdevice_t::impl_t::open_usb_device " << name;
	ssize_t cnt = libusb_get_device_list(usbcontext_, &devs);
    try
    {

        int i=0;
        unsigned short vendor,product,addr,busNum;
 	    char buffer[21];

	    while ((int) cnt>0 && (dev = devs[i++]) != NULL) {
        	struct libusb_device_descriptor desc;
        	int r = libusb_get_device_descriptor(dev, &desc);
        	if (r < 0) {
        		pic::logmsg() << "pic::usbdevice_t::impl_t::open_usb_device : failed to get device descriptor";
        	    goto end;
        	}

			product=desc.idProduct;
        	vendor=desc.idVendor;
        	busNum=libusb_get_bus_number(dev);
        	addr=libusb_get_device_address(dev);
       	    buildUsbName(buffer,vendor,product,addr,busNum);
            name_ = name;
    		// pic::logmsg() << "pic::usbdevice_t::impl_t::open_usb_device: trying device " << buffer;
       	    if(strcmp(name,buffer)==0)
       	    {
        		pic::logmsg() << "pic::usbdevice_t::impl_t::open_usb_device: found device " << name;
        		int status = libusb_open(dev, &device);
        		if(status<0)
        		{
        			device=0ULL;
        			pic::logmsg() << "pic::usbdevice_t::impl_t::open_usb_device failed" <<  libusb_error_name(status);
        			goto end;
        		}
        		pic::logmsg() << "pic::usbdevice_t::impl_t::open_usb_device: opened device " << name;
        		goto end;
       	    }
        }        
        
    }
	CATCHLOG()
end:
    libusb_free_device_list(devs, 1);
    return device;
}


bool pic::usbdevice_t::add_bulk_out(bulk_out_pipe_t *p)
{
    return impl_->add_bulk_out(p);
}

bool pic::usbdevice_t::add_iso_in(iso_in_pipe_t *p)
{
	return impl_->add_iso_in(p);	
}

void pic::usbdevice_t::set_iso_out(iso_out_pipe_t *p)
{
	impl_->set_iso_out(p);
}

bool pic::usbdevice_t::impl_t::add_bulk_out(bulk_out_pipe_t *p)
{
    new pic::usbdevice_t::bulk_out_pipe_t::impl_t(this,p);
    return true;
}

bool pic::usbdevice_t::impl_t::add_iso_in(iso_in_pipe_t *p)
{
	try
	{
		usbpipe_in_t *pipe  = new usbpipe_in_t(this,p);
		inpipes_.alternate().push_back(pipe);
		inpipes_.exchange();
		return true;
	}
	catch(std::exception &e)
	{
		pic::logmsg() << "cannot add pipe: " << e.what();
		return false;
	}
	catch(...)
	{
		pic::logmsg() << "cannot add pipe: unknown error";
		return false;
	}
}

void pic::usbdevice_t::impl_t::set_iso_out(iso_out_pipe_t *p)
{
	try
	{
		pipe_out_.set(new usbpipe_out_t(this,p));
	}
	catch(std::exception &e)
	{
		pic::logmsg() << "cannot add pipe: " << e.what();
	}
	catch(...)
	{
		pic::logmsg() << "cannot add pipe: unknown error";
	}
}

pic::usbdevice_t::impl_t::~impl_t()
{
    close();

    libusb_exit(usbcontext_);
}

void pic::usbdevice_t::impl_t::close()
{
    detach();
    if(opened_)
    {
    	PIC_ASSERT(dhandle_!=0ULL);
    	libusb_release_interface(dhandle_, 0);
    	libusb_close(dhandle_);
    	opened_ = false;
    }
}

void pic::usbdevice_t::impl_t::pipes_died(unsigned reason)
{
    stopping_ = true;

    if(power)
    {
        power->pipe_died(reason);
    }
}

bool pic::usbdevice_t::impl_t::poll_pipe(unsigned long long t)
{
	//	LOG_SINGLE(pic::logmsg() << "usbdevice_t::impl_t::poll_pipe" ;  )
    pic::flipflop_t<pic::lcklist_t<usbpipe_in_t *>::lcktype>::guard_t g(inpipes_);
    pic::lcklist_t<usbpipe_in_t *>::lcktype::const_iterator i;

    bool stolen=false;
    for(i=g.value().begin(); i!=g.value().end(); i++)
    {
        if((*i)->poll_pipe(t))
        {
            stolen = true;
        }
    }

    return stolen;
}


pic::usbdevice_t::usbdevice_t(const char *name, unsigned iface)
{
	pic::logmsg() << "pic::usbdevice_t::usbdevice_t usb device create " << name << " iface " << iface ;
	impl_ = new usbdevice_t::impl_t(name,iface,this);
}

void pic::usbdevice_t::start_pipes()
{
	impl_->start_pipes();
}

void pic::usbdevice_t::impl_t::stop_pipes()
{
    if(!isrunning())
    {
        return;
    }

    if(power)
    {
        power->pipe_stopped();
    }

	stopping_ = true;
    wait();
}


void pic::usbdevice_t::impl_t::thread_main()
{
    for(;;)
    {
        if(stopping_ )
        {
            pic::logmsg() << "usbdevice_t::impl_t::thread_main()- stopping...";
            unsigned c;

            {
        		pic::mutex_t::guard_t guard(&(device_lock_));
                c = count_;
            }

            if(c == 0)
            {
               	pic::logmsg() << "usbdevice_t::impl_t::thread_main()- stopped";
                break;
            }
        }

		// handle events for 1 second, then wake up to see if we are stopping, and 
        // and check to see that we are still sending requests
        int status;
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if((status=libusb_handle_events_timeout_completed(usbcontext_,&tv,0))<0)
        {
            if(status == LIBUSB_ERROR_INTERRUPTED) continue;  // this can be caused by some posix events, normal to just re-poll
        	pic::logmsg() << "usbdevice_t::impl_t::thread_main() USB thread dying: " << libusb_error_name(status) << " (" << status << ")";
            break;
        }
    }

    pipes_died(PIPE_UNKNOWN_ERROR);
    died_ = true;
    stopping_ = true;	
	return;
}


void pic::usbdevice_t::impl_t::thread_init()
{
	pic::lcklist_t<usbpipe_in_t *>::lcktype::const_iterator i;
	
    if(power)
    {
        power->pipe_started();
    }

	for(i=inpipes_.alternate().begin(); i!=inpipes_.alternate().end(); i++)
	{
		(*i)->start();				
	}
	
}

void pic::usbdevice_t::impl_t::start_pipes()
{
    if(isrunning())
    {
        return;
    }

	stopping_ = false;
    run();
	pic::logmsg() << "usbdevice_t::impl_t::start_pipes() : pipes started!" ; 
}

void pic::usbdevice_t::impl_t::detach()
{
    pic::logmsg() << "usbdevice_t::impl_t::detach() : detaching client";

    stop_pipes();

	while( inpipes_.alternate().size() > 0 )
	{
		usbpipe_in_t *p = inpipes_.alternate().front();
		inpipes_.alternate().pop_front();
        inpipes_.exchange();
        delete p;		
	}

    if(pipe_out_.alternate())
    {
        usbpipe_out_t *p = pipe_out_.alternate();
        pipe_out_.set(0);
        delete p;
    }

	power = 0;
	//device_ = NULL;
    pic::logmsg() << "usbdevice_t::impl_t::detach() : done detaching client";
}

void pic::usbdevice_t::stop_pipes()
{
    impl_->stop_pipes();
}

pic::usbdevice_t::~usbdevice_t()
{
	delete impl_;
}

bool pic::usbdevice_t::poll_pipe(unsigned long long t)
{
	return impl_->poll_pipe(t);
}

void pic::usbdevice_t::control_in(unsigned char type, unsigned char req, unsigned short val, unsigned short ind, void *buffer, unsigned len, unsigned timeout)
{
    int status = libusb_control_transfer(impl_->dhandle_,type,req,val,ind,(unsigned char *)buffer,len,timeout);

    if(status < 0)
    {
    	pic::logmsg() << "pic::usbdevice_t::control_in request failed: " << status << " - " << libusb_error_name(status) << ' ' << std::hex << (int)type << ':' << (int)req;
	}
}

void pic::usbdevice_t::control_out(unsigned char type, unsigned char req, unsigned short val, unsigned short ind, const void *buffer, unsigned len, unsigned timeout)
{
    int status = libusb_control_transfer(impl_->dhandle_,type,req,val,ind,(unsigned char *)buffer,len,timeout);

    if(status < 0)
    {
       	pic::logmsg() << "pic::usbdevice_t::control_out request failed: " << status << " - " << libusb_error_name(status) << ' ' << std::hex << (int)type << ':' << (int)req;
	}
}

void pic::usbdevice_t::control(unsigned char type, unsigned char req, unsigned short val, unsigned short ind, unsigned timeout)
{
    int status = libusb_control_transfer(impl_->dhandle_,type,req,val,ind,0,0,timeout);

    if(status < 0)
    {
       	pic::logmsg() << "pic::usbdevice_t::control request failed: " << status << " - " << libusb_error_name(status) << ' ' << std::hex << (int)type << ':' << (int)req;
	}
}

void pic::usbdevice_t::bulk_out_pipe_t::bulk_write(const void *data, unsigned len, unsigned timeout)
{
    impl_->bulk_write(data,len,timeout);
}

void pic::usbdevice_t::set_power_delegate(power_t *p)
{
    impl_->power = p;
}

void pic::usbdevice_t::detach()
{
    impl_->detach();
}

void pic::usbdevice_t::close()
{
    impl_->close();
}

const char *pic::usbdevice_t::name()
{
    return impl_->name_.c_str();
}


//pic::usbenumerator_t
pic::usbenumerator_t::impl_t::impl_t(unsigned short v, unsigned short p, const f_string_t &a, const f_string_t &r): pic::thread_t(0), vendor_(v), product_(p), added_(a), removed_(r)
{
    if(libusb_init(&usbcontext_)<0)
    {
    	pic::logmsg() << "pic::usbenumerator_t : cannot initialise libusb for enumerator";
    }
}

pic::usbenumerator_t::impl_t::~impl_t()
{
    tracked_invalidate();
    stop();

    libusb_exit(usbcontext_);
}



void pic::usbenumerator_t::impl_t::thread_pass()
{
	pic::logmsg() << "pic::usbenumerator_t::thread_pass : searching V " << vendor_ << " P " << product_;

	std::set<std::string> working;
	working.clear();

	int count = 0;
    libusb_device **devs;
	ssize_t cnt = libusb_get_device_list(usbcontext_, &devs);
    try
    {

        libusb_device *dev;
        int idx=0;
        while ((int) cnt>0 && (dev = devs[idx++]) != NULL) {
        	struct libusb_device_descriptor desc;
        	int r = libusb_get_device_descriptor(dev, &desc);
        	if (r < 0) {
        		pic::logmsg() << "pic::usbenumerator_t::enumerate : failed to get device descriptor";
        	    libusb_free_device_list(devs, 1);
        		libusb_exit(usbcontext_);
        		return;
        	}
        	
        	count++;
        	
        	unsigned busNum= libusb_get_bus_number(dev);
        	unsigned addr= libusb_get_device_address(dev);
        	// pic::logmsg() << "pic::usbenumerator_t::enumerate details : "
        	// 			<< " V " << desc.idVendor
        	// 			<< " P " << desc.idProduct
        	// 			<< " B " << busNum
        	// 			<< " A " << addr;
        	
        	if (desc.idVendor==vendor_ &&  desc.idProduct==product_)
        	{
        	    char buffer[21];
        	    buildUsbName(buffer,desc.idVendor,desc.idProduct,addr,busNum);
            	pic::logmsg() << "pic::usbenumerator_t::enumerate device found : " << buffer;
                working.insert(buffer);
        	}
        }
        
        // compare to existing set
		std::set<std::string>::const_iterator i;
		
		pic::mutex_t::guard_t g(enum_lock_);
		
		for(i=devices_.begin(); i!=devices_.end(); i++)
		{
			if(working.find(*i)==working.end())
			{
				pic::logmsg() << "usb device detached" << i->c_str();
		
				try
				{
					removed_(i->c_str());
				}
				CATCHLOG()
			}
		}
		
		for(i=working.begin(); i!=working.end(); i++)
		{
			if(devices_.find(*i)==devices_.end())
			{
				pic::logmsg() << "usb device attached" << i->c_str();
				try
				{
					added_(i->c_str());
				}
				CATCHLOG()
			}
		}
		
		devices_ = working;
    }
	CATCHLOG()
    libusb_free_device_list(devs, 1);

}

void pic::usbenumerator_t::impl_t::thread_init()
{
    stop_=false;
}

void pic::usbenumerator_t::impl_t::thread_main()
{
#ifdef DEBUG_DATA_ATOMICITY
    std::cout << "Started USB enumerator thread with ID " << pic_current_threadid() << std::endl;
#endif
 
    while(!stop_)
    {
        thread_pass();
        sleep(1000);
    }
}

pic::usbenumerator_t::usbenumerator_t(unsigned short v, unsigned short p, const f_string_t &a)
{
    impl_ = new impl_t(v,p,a,f_string_t());
}

pic::usbenumerator_t::usbenumerator_t(unsigned short v, unsigned short p, const f_string_t &a, const f_string_t &r)
{
    impl_ = new impl_t(v,p,a,r);
}

pic::usbenumerator_t::~usbenumerator_t()
{
    delete impl_;
}

void pic::usbenumerator_t::start()
{
    impl_->run();
}

void pic::usbenumerator_t::stop()
{
    impl_->stop();
}


unsigned pic::usbenumerator_t::enumerate(unsigned short vendor, unsigned short product, const f_string_t &callback)
{
	pic::logmsg() << "pic::usbenumerator_t::enumerate : seaerching V " << vendor << " P " << product;
	libusb_context* context;
	libusb_init(&context);
    int count = 0;
    libusb_device **devs;
	ssize_t cnt = libusb_get_device_list(context, &devs);
    try
    {

        int i=0;
        libusb_device *dev;
        
        while ((int) cnt>0 && (dev = devs[i++]) != NULL) {
        	struct libusb_device_descriptor desc;
        	int r = libusb_get_device_descriptor(dev, &desc);
        	if (r < 0) {
        		pic::logmsg() << "pic::usbenumerator_t::enumerate : failed to get device descriptor";
        	    libusb_free_device_list(devs, 1);
        		libusb_exit(context);
        		return 0;
        	}
        	
        	count++;
        	
        	unsigned busNum= libusb_get_bus_number(dev);
        	unsigned addr= libusb_get_device_address(dev);
        	pic::logmsg() << "pic::usbenumerator_t::enumerate details : "
        				<< " V " << desc.idVendor
        				<< " P " << desc.idProduct
        				<< " B " << busNum
        				<< " A " << addr;
        	
//            uint8_t path[8];
//        	r = libusb_get_port_numbers(dev, path, sizeof(path));
//        	if (r > 0) {
//            	pic::logmsg() << "pic::usbenumerator_t::enumerate path : " << path[0];
//        		for (j = 1; j < r; j++)
//        		{
//                	pic::logmsg() << "pic::usbenumerator_t::enumerate path : :"<< j << " - " << path[j];
//        		}
//        	}
        	
        	if (desc.idVendor==vendor &&  desc.idProduct==product)
        	{
        	    char buffer[21];
        	    buildUsbName(buffer,vendor,product,addr,busNum);
//        	    unsigned long name;
            	pic::logmsg() << "pic::usbenumerator_t::enumerate found : " << buffer;
                callback(buffer);
        	}
        }        
        
    }
	CATCHLOG()
    libusb_free_device_list(devs, 1);
	libusb_exit(context);
    return count;
}

int pic::usbenumerator_t::gc_traverse(void *v,void *a) const
{
    int r;
    pic::mutex_t::guard_t g(impl_->enum_lock_);
    if((r=impl_->added_.gc_traverse(v,a))!=0) return r;
    if((r=impl_->removed_.gc_traverse(v,a))!=0) return r;
    return 0;
}

int pic::usbenumerator_t::gc_clear()
{
    pic::mutex_t::guard_t g(impl_->enum_lock_);
    impl_->added_.gc_clear();
    impl_->removed_.gc_clear();
    return 0;
}
