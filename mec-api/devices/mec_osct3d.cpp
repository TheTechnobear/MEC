#include "mec_osct3d.h"


#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>


#include "mec_log.h"
#include "mec_prefs.h"
#include "../mec_voice.h"

////////////////////////////////////////////////
// TODO
////////////////////////////////////////////////

namespace mec {

class OscT3DHandler: public  osc::OscPacketListener
{
public:
    OscT3DHandler(Preferences& p, MsgQueue& q)
        :   prefs_(p),
            queue_(q),
            valid_(true),
            socket_(nullptr)
    {
        if (valid_) {
            LOG_0("OscT3DHandler enabling for mecapi");
        }
        for (int i = 0; i < sizeof(activeTouches_); i++) {
            activeTouches_[i] = false;
        }
        stealVoices_ = false;
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
            static const std::string A_FRM = "/t3d/frm";


            osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
            std::string addr = m.AddressPattern();
            if ( addr.length() > 8 && addr.find(A_TOUCH) == 0) {
                std::string touch = addr.substr(8);
                int tId = std::stoi(touch);
                float x = 0.0f, y = 0.0f, z = 0.0f, note = 0.0f;
                args >> x >> y >> z >> note >>  osc::EndMessage;
                queue_touch(tId, note, x, (y * 2.0f) - 1.0f, z);
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
                    msg.type_ = MecMsg::MEC_CONTROL;
                    msg.data_.mec_control_.cmd_ = MecMsg::SHUTDOWN;
                    queue_.addToQueue(msg);
                }
            }
        } catch ( osc::Exception& e ) {
            // any parsing errors such as unexpected argument types, or
            // missing arguments get thrown as exceptions.
            LOG_0("error while parsing message: " << m.AddressPattern() << ": " << e.what() );
        }
    }

    virtual void queue_touch(int tId, float mn, float mx, float my, float mz)
    {
        Voices::Voice* voice = voices_.voiceId(tId);
        if (mz > 0.0)
        {
            if (!voice) {
                voice = voices_.startVoice(tId);
                // LOG_1("start voice for " << tId << " ch " << voice->i_);

                if (!voice && stealVoices_) {
                    // no available voices, steal?
                    Voices::Voice* stolen = voices_.oldestActiveVoice();
                    MecMsg msg;
                    msg.data_.touch_.touchId_ = stolen->i_;
                    msg.data_.touch_.note_ = stolen->note_;
                    msg.data_.touch_.x_ = stolen->x_;
                    msg.data_.touch_.y_ = stolen->y_;
                    msg.data_.touch_.z_ = 0.0f;
                    msg.type_ = MecMsg::TOUCH_OFF;
                    queue_.addToQueue(msg);
                    voices_.stopVoice(stolen);
                    voice = voices_.startVoice(tId);
                }
            }

            if (voice) {
                if (voice->state_ == Voices::Voice::PENDING) {
                    // LOG_1("pressure voice for " << tId << " ch " << voice->i_ << " mz " << mz);
                    voices_.addPressure(voice, mz);
                    if (voice->state_ == Voices::Voice::ACTIVE) {
                        MecMsg msg;
                        msg.data_.touch_.touchId_ = voice->i_;
                        msg.data_.touch_.note_ = mn;
                        msg.data_.touch_.x_ = mx;
                        msg.data_.touch_.y_ = my;
                        msg.data_.touch_.z_ = voice->v_;
                        msg.type_ = MecMsg::TOUCH_ON;
                        queue_.addToQueue(msg);
                    }
                    // dont send to callbacks until we have the minimum pressures for velocity
                }
                else {
                    MecMsg msg;
                    msg.data_.touch_.touchId_ = voice->i_;
                    msg.data_.touch_.note_ = mn;
                    msg.data_.touch_.x_ = mx;
                    msg.data_.touch_.y_ = my;
                    msg.data_.touch_.z_ = mz;
                    msg.type_ = MecMsg::TOUCH_CONTINUE;
                    queue_.addToQueue(msg);
                }
                voice->note_ = mn;
                voice->x_ = mx;
                voice->y_ = my;
                voice->z_ = mz;
                voice->t_ = 0;
            }
            // else no voice available

        } else {
            
            if (voice) {
                // LOG_1("stop voice for " << tId << " ch " << voice->i_);
                MecMsg msg;
                msg.data_.touch_.touchId_ = voice->i_;
                msg.data_.touch_.note_ = mn;
                msg.data_.touch_.x_ = mx;
                msg.data_.touch_.y_ = my;
                msg.data_.touch_.z_ = mz;
                msg.type_ = MecMsg::TOUCH_OFF;
                queue_.addToQueue(msg);
                voices_.stopVoice(voice);
            }
        }
    }
private:
    inline  float clamp(float v, float mn, float mx) {return (std::max(std::min(v, mx), mn));}
    float   note(float n) { return n; }

    Preferences prefs_;
    MsgQueue& queue_;
    bool valid_;
    bool activeTouches_[16];
    UdpListeningReceiveSocket* socket_;
    bool stealVoices_;
    Voices voices_;

};


////////////////////////////////////////////////
OscT3D::OscT3D(ICallback& cb) :
    active_(false), callback_(cb) {
}

OscT3D::~OscT3D() {
    deinit();
}


void OscT3DListen(OscT3D* self) {
    self->listenProc();
}

void OscT3D::listenProc() {
    LOG_1( "T3D socket listening on : " << port_);
    socket_->Run();
}

bool OscT3D::init(void* arg) {
    Preferences prefs(arg);

    if (active_) {
        deinit();
    }
    active_ = false;
    OscT3DHandler *pCb = new OscT3DHandler(prefs, queue_);

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

    listenThread_ = std::thread(OscT3DListen, this);


    return active_;
}

bool OscT3D::process() {
    return queue_.process(callback_);
}

void OscT3D::deinit() {
    LOG_0("OscT3D::deinit");
    if (active_) {
        socket_->AsynchronousBreak();
        listenThread_.join();
        socket_.reset();
        LOG_0("OscT3D::deinit done");
    }
    active_ = false;
}

bool OscT3D::isActive() {
    return active_;
}


}

