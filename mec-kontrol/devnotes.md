# Development Notes
This document is a mix of 'todos', brain dumps, general ideas on way forward.
it is **not** definitive, things are still in a state of flux.

# Status
pre-alpha, development, code that is untested, and is likely not working - no **not** use.

# TO DO
- Push 2, update to new mode model, and allow selection of rack/module, also > 8 params etc
- resolve Rack entity id ... what to use? description? 
- invert connection model , i.e. racks connect to 'parent'
- meta data, we should only push to requesting client
- ping keep-alive
- version osc mesages 

# design
overall design is to allow for a peer to peer parameter system, essentially Kontrol is responsible for 'syncronising' parameters across a transport.

software layers : general -> pure data layer -> organelle layer (?)
general layer:
- parameter data mapping
- osc transmit/recieve of parameters 
- file storage - for presets
- map parameter scaled input (0..1, relative) to actual value
- parameter takeover functionality (relates to active page/potentmeter type)
- midi mapping/learn

overall design is to allow for a peer to peer parameter system, essentially Kontrol is responsible for 'syncronising' parameters across a transport.

pure data layer :
- pure data messages
- send pd messages e.g. s r_mix

device layer - organelle :
- screen
- pots
- encoder reception
  
## parameter definitions
(done)
currently, we define parameters in PD via messages to the external, however, there are advantages to moving to a parameter file, so the pd patch is only really bound by the usage of the external and the 'receive name'

## midi mapping / learn
(done)
allow assignment of parameter to a midi cc
device later could allow learn, e.g. activate learn, twist param, fire cc

## heirarcy for parameters
current implementation is page->params
however, we need to also allow at least for multiple clients 
e.g. 
/device/page/parameter
also do we need to even sub group these? 
e.g. 
/device/module/page/parameter
this might be a bit much? but it would be nice to move to a less rigid heirachy

to stop loops, do we need to allow only the owner to make the change? but if so how do we get fast local feedback?
(currently, we do the change locally, then its overwritten, we are using parameter source)

the issue with forming a heirarchy is parameters are forced to be on one page... rather having arbitary grouping of parameters
the current model, has parameters as a top level entity where, they pages are a container.

however... what we need to do, is to have parameter ids unique for a particular device/patch....



## osc messaging format
(done, except need to rationalise rack, versioning) 

move to a message heirarcy 
version osc messages (or clients?)

example:
````
/kontrol/rack organelle
/kontrol/module organelle wrpsbrds
/kontrol/param organelle wrpsbrds o_transpose "pitch" -24 24 0
/kontrol/param organelle wrpsbrds o_transpose "pitch" -24 24 0
/kontrol/page organelle wrpsbrds o_transpose "oscillator" o_transpose 
/kontrol/changed organelle wrpsbrds o_transpose  12
````




## json parameter file 
(pseudo only ;) )

````
patch : {
    name : wrpsbrds,
    parameters : {
        o_transpose {
            type : pitch, 
            displayname : transpose,
            min : -24,
            max : 24,
            default : 0
        }
        ...
    }, 
    pages : {
        page_1 {
            displayname: oscillator
            parameters : [
                o_transpose, 
            ]
        }
    }
}
````



## json patch file (?)
lets start with (e.g.) 10 presets, which can be saved/recalled
store presets as param name , value
````
preset {
    0 : {
        o_transpose : 12,
        r_mix : 50
    }
}, 
midi_mapping : {
    cc : {
        63 : o_transpose
    }
}
````

# pure data messages:


currently we have:
page page1 "reverb"  - adds a page called page1, displayed as reverb

param r_mix page1 pct "mix" 0 100 50 - add parameter r_mix to page 1, displayed as mix, 0 to 100 percent, 50 default
param r_active page1 bool "active" on 
param r_time page1 msec "time"  
param r_feedback page1 pct "feedback" on 

connect port

organelle messages... rawknob1-4 encoder, this is because we can't directly listen to mother host, since mother.pd is blocking us. 
(this could change in teh future if we replace mother.pd)

the idea is to move away from messages, and try to move to using a json configuration file

## notes
types/range change depending on type.


# types
pct     - 54 %  (allow decimals?)
linHz   - 1330 hz (linear hz)
expHz   - 1330 hz (exponential hz)
semis   - 1 sem
octave  - 1 oct
bool    - on/off
second  - 15 s (allow dec?)
msec    - 100 mSec (milliseconds)

advanced types , later on
enum  -  e.g. hall|room|drum 

# display 
need to right justify text, variable space in middle.
page name in bottom right?
Aux function test? (all pages, or per page?)
e.g
````
I ======    O ==========
|mix                45%|
|active              on|
|time         1500 mSec|
|feedback           45%|
Aux = blah      [Reverb]
````

twist encoder, display next/prev page as overlay, only display params on encoder up
````
I ======    O ==========
|mix                45%|
|ac------------------on|
|ti|<  Oscs        >|ec|
|fe------------------5%|
Aux = blah      [Reverb]
````



## TO DO more details/thoughts

### Push 2, update to new mode model, and allow selection of rack/module, also > 8 params etc
similar model to organelle, each mode has a handler... to resolve all inputs.

### Resolve Rack entity id ... what to use? description? 
original plan was host:port , but how do we get the hostname (possible) and is it really useful, whne you have multiple racks
e.g. having organelle:9000 organelle 9001, what about descriptive name?


 
### invert connection model , i.e. racks connect to 'parent'
currently, PD is listening, and MEC connects and requests meta data
new idea is:
MEC is running and listening
PD connects to MEC, and listens, and when it connects sends meta data



### meta data, we should only push to requesting client
pushing to other clients...
which clients?  
eg. does a PD client really want to know about other clients?


### version osc mesages 
put in main messages topic, so we dont have issues?
Im not likely to support multiple versions in short term
so
```` /kontrol/v1/param ````


### keep-alive connection model

ping messages contains host/port, port is the port the client is listening on.

client connects, and starts sending pings to server... so server knows it exists.
(perhaps back off if doesnt reply?)
server maintains a client list, 
a new client is added (with an OSCBroadcaster) when a ping arrives from somewhere it doesn't know

server polls client list, at the ping frequency ( 5 seconds)
server pings to active clients

OSCBroadcaster is registered to model, to recieve pings... it then:
 - records the last ping time, 
 - if a slave, if was previous not connected, sends meta data

additionally, OSCBroadcast will not transmit any OSC messages, to a client if the ping tiemout has been exceeded (10 seconds) 


important concepts are:
- client is sending pings out frequently, master responds to this ping. (RTT/latency could be tracked here)
- lack of receiving pings = not active
- OSC broadcasters are connected via KontrolModel callback, to get the ping receipt
- when client sends inital ping,  server can add it to list of clients
- both side assume disconnect if not active (server could removed from client list too)

Meta Data... we need to either
- clear meta data if client goes inactive... 
- clear meta data due to 'meta data clear message'
- 'overrwrite data' if we get meta data messages.. 
( I think currently I overwrite but we need to check)