#ifndef MECMSGQUEUE_H
#define MECMSGQUEUE_H

#include <memory>

namespace mec {

class ICallback;

struct MecMsg {
    enum type {
        TOUCH_ON,
        TOUCH_CONTINUE,
        TOUCH_OFF,
        CONTROL,
        MEC_CONTROL
    } type_;

    enum mec_cmd {
        SHUTDOWN,
        PING
    };

    union {
        struct {
            int     touchId_;
            float   note_, x_, y_, z_;
        } touch_;
        struct {
            int     controlId_;
            float   value_;
        } control_;
        struct {
            mec_cmd cmd_;
        } mec_control_;
    } data_;
};

class MsgQueue_impl;

class MsgQueue {
public:
    MsgQueue();
    ~MsgQueue();
    bool addToQueue(MecMsg&);
    bool nextMsg(MecMsg&);
    bool isEmpty();
    bool isFull();
    int  available();
    int  pending();
    bool process(ICallback&);

private:
    std::unique_ptr<MsgQueue_impl> impl_;
};

}

#endif// MECMSGQUEUE_H
