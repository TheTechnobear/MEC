#include <iostream>
#include <unistd.h>
#include <string.h>

#include <pthread.h>

#include <osc/OscOutboundPacketStream.h>
#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>

#include "mec.h"
#include <mec_prefs.h>


#define IN_PORT 9000


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

                LOG_2(std::cout   << "received /tb/mec/command message with argument: " << cmd << std::endl;)
                if (strcmp(cmd, "stop") == 0) {
                    pthread_mutex_lock(&waitMtx);
                    LOG_1(std::cout << " OscCommandListener stopping " << std::endl;)
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
            LOG_1(std::cerr << "error while parsing message: " << m.AddressPattern() << ": " << e.what() << std::endl;)
        }
    }
private:
    UdpListeningReceiveSocket* pSocket_;
};


void *osc_command_proc(void *arg)
{
    LOG_0(std::cout  << "osc_command_proc start" << std::endl;)

    MecPreferences prefs(arg);

    OscCommandListener listener;
    UdpListeningReceiveSocket s(
        IpEndpointName( IpEndpointName::ANY_ADDRESS, IN_PORT ),
        &listener );
    listener.setSocket(&s);
    s.RunUntilSigInt();
    listener.setSocket(NULL);
    LOG_0(std::cout  << "osc_command_proc stop" << std::endl;)
    pthread_exit(NULL);
}


