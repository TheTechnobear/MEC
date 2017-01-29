#include "mec_osct3d.h"


#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>


#include "../mec_log.h"
#include "../mec_prefs.h"
#include "../mec_voice.h"

////////////////////////////////////////////////
// TODO
////////////////////////////////////////////////


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
            activeTouches_[i] = false;
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
            static const std::string A_COMMAND = "/t3d/command";
            static const std::string A_TOUCH = "/t3d/tch";
            static const std::string A_FRM= "/t3d/frm";


            osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
            std::string addr = m.AddressPattern();
            if ( addr.length() >8 && addr.find(A_TOUCH)==0){
                std::string touch = addr.substr(8);
                int tId=std::stoi(touch);
                float x=0.0f,y=0.0f,z=0.0f,note=0.0f;
                args >> x >> y >> z >> note >>  osc::EndMessage;

                MecMsg msg;
                msg.data_.touch_.touchId_ = tId;
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
                        activeTouches_[tId] = true;
                    } else {
                        msg.type_ = MecMsg::TOUCH_CONTINUE;
                    }
                }

                queue_.addToQueue(msg);
            }
            else if ( addr == A_FRM) {
                osc::int32 d1, d2;
                args >> d1 >> d2 >> osc::EndMessage;
            }  
            else if ( addr ==  A_COMMAND) {
                const char* cmd;
                args >> cmd >> osc::EndMessage;

                LOG_1("received /t3d/command message with argument: " << cmd );
                if (strcmp(cmd, "shutdown") == 0) {
                    LOG_1( "T3D shutdown request" );
                    MecMsg msg;
                    msg.type_=MecMsg::MEC_CONTROL;
                    msg.data_.mec_control_.cmd_=MecMsg::SHUTDOWN;
                    queue_.addToQueue(msg);
                }
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
    UdpListeningReceiveSocket* socket_;
};


////////////////////////////////////////////////
MecOscT3D::MecOscT3D(IMecCallback& cb) :
    active_(false), callback_(cb) {
}

MecOscT3D::~MecOscT3D() {
    deinit();
}


void MecOscT3DListen(MecOscT3D* self) {
    self->listenProc();
}

void MecOscT3D::listenProc() {
    LOG_1( "T3D socket listening on : " << port_);
    socket_->Run();
}

bool MecOscT3D::init(void* arg) {
    MecPreferences prefs(arg);

    if (active_) {
        deinit();
    }
    active_ = false;
    MecOscT3DHandler *pCb = new MecOscT3DHandler(prefs, queue_);

    port_ = prefs.getInt("port", 9000);

    if (pCb->isValid()) {
        active_ = true;
    } else {
        delete pCb;
    }

    LOG_1( "T3D socket on port : " << port_);

    socket_.reset(
        new UdpListeningReceiveSocket(  
            IpEndpointName( IpEndpointName::ANY_ADDRESS, port_ ), 
            pCb )
    );
    
    pCb->setSocket(socket_.get());

    listenThread_= std::thread(MecOscT3DListen, this);


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
        case MecMsg::MEC_CONTROL :
            if(msg.data_.mec_control_.cmd_==MecMsg::SHUTDOWN) {
                LOG_1( "OSC posting shutdown request");
                callback_.mec_control(
                    IMecCallback::SHUTDOWN,
                    nullptr);
            }
            break;
        default:
            LOG_0("MecOscT3D::process unhandled message type");
        }
    }
    return true;
}

void MecOscT3D::deinit() {
    LOG_0("MecOscT3D::deinit");
    if(active_) {
        socket_->AsynchronousBreak();
        listenThread_.join();
        socket_.reset();
        LOG_0("MecOscT3D::deinit done");
    }
    active_ = false;
}

bool MecOscT3D::isActive() {
    return active_;
}




