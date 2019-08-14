#include "mec_msg_queue.h"

#include "mec_api.h"
#include "mec_log.h"

#include <readerwriterqueue.h>
namespace mec {

const int MAX_QUEUE_SIZE = 32;


class MsgQueue_impl {
public:
    MsgQueue_impl();
    ~MsgQueue_impl();

    bool addToQueue(MecMsg &);
    bool nextMsg(MecMsg &);

private:
    moodycamel::ReaderWriterQueue<MecMsg> queue_;
};


/////////// Public Interface
MsgQueue::MsgQueue() {
    impl_.reset(new MsgQueue_impl());
}

MsgQueue::~MsgQueue() {
}

bool MsgQueue::addToQueue(MecMsg &msg) {
    return impl_->addToQueue(msg);
}

bool MsgQueue::nextMsg(MecMsg &msg) {
    return impl_->nextMsg(msg);
}


bool MsgQueue::process(ICallback &c) {
    MecMsg msg;
    while (nextMsg(msg)) {
        send(msg,c);
    }
    return true;
}


bool MsgQueue::send(MecMsg& msg, ICallback &c) {
    switch (msg.type_) {
        case MecMsg::TOUCH_ON:
            c.touchOn(
                    msg.data_.touch_.touchId_,
                    msg.data_.touch_.note_,
                    msg.data_.touch_.x_,
                    msg.data_.touch_.y_,
                    msg.data_.touch_.z_);
            break;
        case MecMsg::TOUCH_CONTINUE:
            c.touchContinue(
                    msg.data_.touch_.touchId_,
                    msg.data_.touch_.note_,
                    msg.data_.touch_.x_,
                    msg.data_.touch_.y_,
                    msg.data_.touch_.z_);
            break;
        case MecMsg::TOUCH_OFF:
            c.touchOff(
                    msg.data_.touch_.touchId_,
                    msg.data_.touch_.note_,
                    msg.data_.touch_.x_,
                    msg.data_.touch_.y_,
                    msg.data_.touch_.z_);
            break;
        case MecMsg::CONTROL :
            c.control(
                    msg.data_.control_.controlId_,
                    msg.data_.control_.value_);
            break;

        case MecMsg::MEC_CONTROL :
            if (msg.data_.mec_control_.cmd_ == MecMsg::SHUTDOWN) {
                LOG_1("posting shutdown request");
                c.mec_control(ICallback::SHUTDOWN, nullptr);
            }
        default:
            LOG_0("MsgQueue::process unhandled message type");
    }
    return true;
}




/////////// Implementation

MsgQueue_impl::MsgQueue_impl() : queue_(MAX_QUEUE_SIZE) {
}

MsgQueue_impl::~MsgQueue_impl() {

}

bool MsgQueue_impl::addToQueue(MecMsg &msg) {
    return queue_.try_enqueue(msg);
    return true;
}

bool MsgQueue_impl::nextMsg(MecMsg &msg) {
    return queue_.try_dequeue(msg);
}

}