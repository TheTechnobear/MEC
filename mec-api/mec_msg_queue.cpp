#include "mec_msg_queue.h"

#include "mec_log.h"



namespace mec {

const int RING_BUFFER_SIZE=30;


class MsgQueue_impl {
public:
    MsgQueue_impl();
    ~MsgQueue_impl();

    bool addToQueue(MecMsg&);
    bool nextMsg(MecMsg&);
    bool isEmpty();
    bool isFull();
    int  available();
    int  pending();

private:
    MecMsg queue_[RING_BUFFER_SIZE];
    // unsigned readPtr_;
    // unsigned writePtr_;
    volatile unsigned readPtr_;
    volatile unsigned writePtr_;
};


/////////// Public Interface
MsgQueue::MsgQueue () {
    impl_.reset(new MsgQueue_impl());
} 

MsgQueue::~MsgQueue() {
}

bool MsgQueue::addToQueue(MecMsg& msg) {
    return impl_->addToQueue(msg);
}

bool MsgQueue::nextMsg(MecMsg& msg) {
    return impl_->nextMsg(msg);
}

bool MsgQueue::isEmpty() {
    return impl_->isEmpty();
}

bool MsgQueue::isFull() {
    return impl_->isFull();
}

int  MsgQueue::available() {
    return impl_->available();
}

int  MsgQueue::pending() {
    return impl_->pending();
}


/////////// Implementation 
MsgQueue_impl::MsgQueue_impl () {
    writePtr_ = readPtr_  = 0;
}

MsgQueue_impl::~MsgQueue_impl() {
    
}

bool MsgQueue_impl::addToQueue(MecMsg& msg) {
    unsigned next = (writePtr_ + 1) % RING_BUFFER_SIZE;

    if(next == readPtr_) {
        LOG_0("MsgQueue_impl : ring buffer overflow");
        return false;
    }
    queue_[writePtr_] = msg;
    writePtr_=next;
    return true; 
}

bool MsgQueue_impl::nextMsg(MecMsg& msg) {
    if (readPtr_ != writePtr_ ) {
        msg = queue_[readPtr_];
        readPtr_ =(readPtr_ + 1) % RING_BUFFER_SIZE;
        return true;
    }
    return false;
}

bool MsgQueue_impl::isEmpty() {
    return readPtr_ == writePtr_;
}

bool MsgQueue_impl::isFull() {
    return available()==0;    
}

int  MsgQueue_impl::available() {
    return RING_BUFFER_SIZE - pending();
    
}

int MsgQueue_impl::pending() {
        if(writePtr_ >= readPtr_) {
        return writePtr_ - readPtr_;
    }

    return writePtr_ + RING_BUFFER_SIZE - readPtr_;
}

}