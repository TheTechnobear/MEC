#include "mec_msg_queue.h"

#include "mec_log.h"


const int RING_BUFFER_SIZE=30;


class MecMsgQueue_impl {
public:
    MecMsgQueue_impl();
    ~MecMsgQueue_impl();

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
MecMsgQueue::MecMsgQueue () {
    impl_.reset(new MecMsgQueue_impl());
} 

MecMsgQueue::~MecMsgQueue() {
}

bool MecMsgQueue::addToQueue(MecMsg& msg) {
    return impl_->addToQueue(msg);
}

bool MecMsgQueue::nextMsg(MecMsg& msg) {
    return impl_->nextMsg(msg);
}

bool MecMsgQueue::isEmpty() {
    return impl_->isEmpty();
}

bool MecMsgQueue::isFull() {
    return impl_->isFull();
}

int  MecMsgQueue::available() {
    return impl_->available();
}

int  MecMsgQueue::pending() {
    return impl_->pending();
}


/////////// Implementation 
MecMsgQueue_impl::MecMsgQueue_impl () {
    writePtr_ = readPtr_  = 0;
}

MecMsgQueue_impl::~MecMsgQueue_impl() {
    
}

bool MecMsgQueue_impl::addToQueue(MecMsg& msg) {
    unsigned next = (writePtr_ + 1) % RING_BUFFER_SIZE;

    if(next == readPtr_) {
        LOG_0("MecMsgQueue_impl : ring buffer overflow");
        return false;
    }
    queue_[writePtr_] = msg;
    writePtr_=next;
    return true; 
}

bool MecMsgQueue_impl::nextMsg(MecMsg& msg) {
    if (readPtr_ != writePtr_ ) {
        msg = queue_[readPtr_];
        readPtr_ =(readPtr_ + 1) % RING_BUFFER_SIZE;
        return true;
    }
    return false;
}

bool MecMsgQueue_impl::isEmpty() {
    return readPtr_ == writePtr_;
}

bool MecMsgQueue_impl::isFull() {
    return available()==0;    
}

int  MecMsgQueue_impl::available() {
    return RING_BUFFER_SIZE - pending();
    
}

int MecMsgQueue_impl::pending() {
        if(writePtr_ >= readPtr_) {
        return writePtr_ - readPtr_;
    }

    return writePtr_ + RING_BUFFER_SIZE - readPtr_;
}