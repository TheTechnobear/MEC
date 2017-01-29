#include "mec_soundplane.h"

#include <SoundplaneModel.h>
#include <MLAppState.h>

#include <SoundplaneMECOutput.h>

#include "../mec_log.h"
#include "../mec_prefs.h"
#include "../mec_voice.h"

////////////////////////////////////////////////
// TODO
// 2. voices not needed? as soundplane already does touch alloction, just need to detemine on and off
////////////////////////////////////////////////


class OscCommandListener : public osc::OscPacketListener {
public:
    OscCommandListener() : pSocket_(NULL) { ; }
    void setSocket(UdpListeningReceiveSocket* pS) {
        pSocket_ = pS;
    }
protected:
    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                 const IpEndpointName& remoteEndpoint )
    {
        (void) remoteEndpoint; // suppress unused parameter warning

        try {
            // example of parsing single messages. osc::OsckPacketListener
            // handles the bundle traversal.

            if ( strcmp( m.AddressPattern(), "/tb/mec/command" ) == 0 ) {
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                const char* cmd;
                args >> cmd >> osc::EndMessage;

                LOG_2("received /tb/mec/command message with argument: " << cmd );
                if (strcmp(cmd, "stop") == 0) {
                    pthread_mutex_lock(&waitMtx);
                    LOG_1( " OscCommandListener stopping " );
                    keepRunning = 0;
                    pthread_cond_broadcast(&waitCond);
                    pthread_mutex_unlock(&waitMtx);
                    if (pSocket_ != NULL) {
                        pSocket_->Break();
                    }
                    usleep(5 * 1000 * 1000);
                }
            }
        } catch ( osc::Exception& e ) {
            // any parsing errors such as unexpected argument types, or
            // missing arguments get thrown as exceptions.
            LOG_0("error while parsing message: " << m.AddressPattern() << ": " << e.what() );
        }
    }
private:
    UdpListeningReceiveSocket* pSocket_;
};





class MecOscT3DHandler: public  osc::OscPacketListener
{
public:
    MecOscT3DHandler(MecPreferences& p, MecMsgQueue& q)
        :   prefs_(p),
            queue_(q),
            valid_(true),
            socket_(nullptr) 
    {
        if (valid_) {
            LOG_0("MecOscT3DHandler enabling for mecapi");
        }
        for(int i=0;i<sizeof(activeTouches_);i++) {
            activeTouches_ = false;
        }
    }

    bool isValid() { return valid_;}

    void setSocket(UdpListeningReceiveSocket* pS) {
        socket_ = pS;
    }

    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                 const IpEndpointName& remoteEndpoint )
    {
        (void) remoteEndpoint; // suppress unused parameter warning

        try {
            // example of parsing single messages. osc::OsckPacketListener
            // handles the bundle traversal.

            osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
            if ( strcmp( m.AddressPattern(), "/t3d/command" ) == 0 ) {
                const char* cmd;
                args >> cmd >> osc::EndMessage;

                LOG_2("received /t3d/command message with argument: " << cmd );
                if (strcmp(cmd, "shutdown") == 0) {
                    pthread_mutex_lock(&waitMtx);
                    LOG_1( "T3D shutdown request" );
                    keepRunning = 0;
                    // pthread_cond_broadcast(&waitCond);
                    // pthread_mutex_unlock(&waitMtx);
                    // if (pSocket_ != NULL) {
                    //     pSocket_->Break();
                    // }
                    // usleep(5 * 1000 * 1000);

                    //callback->mec_control(SHUTDOWNMSG)
                }
            else if ( strcmp( m.AddressPattern(), "/t3d/tch" ) == 0 ) {
                //TODO its tch1...N
                std::string 
                int tId=0;
                float x=0.0f,y=0.0f,z=0.0f,note=0.0f;
                args >> x >> y >> z >>note  osc::EndMessage;

                MecMsg msg;
                msg.data_.touch_.touchId_ = t;
                msg.data_.touch_.note_ = note;
                msg.data_.touch_.x_ = x;
                msg.data_.touch_.y_ = y;
                msg.data_.touch_.z_ = z;

                if(note == 0.0f) { // noteoff 
                    msg.type_ = MecMsg::TOUCH_OFF;
                    activeTouches_[tId] = false;
                } else {
                    if(!activeTouches_[tId]) {
                        msg.type_ = MecMsg::TOUCH_ON;
                    } else {
                        msg.type_ = MecMsg::TOUCH_CONTINUE;
                    }
                }

                queue_.addToQueue(stolenMsg);
            }
            else if ( strcmp( m.AddressPattern(), "/t3d/frm" ) == 0 ) {
                int d1, d2;
                args >> d1 >> d2 >> osc::EndMessage;
            }  
        } catch ( osc::Exception& e ) {
            // any parsing errors such as unexpected argument types, or
            // missing arguments get thrown as exceptions.
            LOG_0("error while parsing message: " << m.AddressPattern() << ": " << e.what() );
        }
    }

private:
    inline  float clamp(float v, float mn, float mx) {return (std::max(std::min(v, mx), mn));}
    float   note(float n) { return n; }

    MecPreferences prefs_;
    MecMsgQueue& queue_;
    bool valid_;

    bool   activeTouches_[16];
};


////////////////////////////////////////////////
MecOscT3D::MecOscT3D(IMecCallback& cb) :
    active_(false), callback_(cb) {
}

MecOscT3D::~MecOscT3D() {
    deinit();
}

bool MecOscT3D::init(void* arg) {
    MecPreferences prefs(arg);

    if (active_) {
        deinit();
    }
    active_ = false;
    MecOscT3DHandler *pCb = new MecOscT3DHandler(prefs, queue_);

    if (pCb->isValid()) {
        active_ = true;
    } else {
        delete pCb;
    }


    OscCommandListener listener;
    UdpListeningReceiveSocket s(  IpEndpointName( IpEndpointName::ANY_ADDRESS, IN_PORT ), pCb );
    pCb->setSocket(&s);

// put this in another thread!
    s.RunUntilSigInt();
//    pCb->setSocket(NULL);

    return active_;
}

bool MecOscT3D::process() {
    MecMsg msg;
    while (queue_.nextMsg(msg)) {
        switch (msg.type_) {
        case MecMsg::TOUCH_ON:
            callback_.touchOn(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::TOUCH_CONTINUE:
            callback_.touchContinue(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::TOUCH_OFF:
            callback_.touchOff(
                msg.data_.touch_.touchId_,
                msg.data_.touch_.note_,
                msg.data_.touch_.x_,
                msg.data_.touch_.y_,
                msg.data_.touch_.z_);
            break;
        case MecMsg::CONTROL :
            callback_.control(
                msg.data_.control_.controlId_,
                msg.data_.control_.value_);
            break;
        default:
            LOG_0("MecOscT3D::process unhandled message type");
        }
    }
    return true;
}

void MecOscT3D::deinit() {
    LOG_0("MecOscT3D::deinit");
    active_ = false;

    //TODO deinit needs to tell listener to break
}

bool MecOscT3D::isActive() {
    return active_;
}




