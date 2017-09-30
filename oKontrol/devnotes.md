# Development Notes
This document is a mix of 'todos', brain dumps, general ideas on way forward.
it is **not** definitive, things are still in a state of flux.

# Status
pre-alpha, development, code that is untested, and is likely not working - no **not** use.

# TO DO
stuff that is obviously outstanding
- Parameter callback for local publishing of messages, updating of screens etc.
- input system for encoders/pots
- takeover functionality
- refactor, many things are setup purely for testing concepts
- receiving over osc needs to lock parameter for organelle

# design
layered : general -> pure data layer -> organelle layer (?)

general layer:
- parameter data mapping
- osc transmit/recieve of parameters
- file storage - for presets
- map parameter scaled input (0..1, relative) to actual value
- parameter takeover functionality (relates to active page/potentmeter type)
- midi mapping/learn

pure data layer :
- pure data messages
- send pd messages e.g. s r_mix

device layer - organelle :
- screen
- pots
- encoder reception
  
## open questions
general layer will allows parameters to be changed via osc at any time...
is current page/takeover all part of device layer?
(seems likely, as encoders vs pots for takeover)

## takeover functionality
when page becomes active, mark all parameters that input value != current value, as locked
unlock when 
if pot > param value , unlock when pot <= param value
if pot < param value , unlock when pot >= param value

## changing values
organelle sends 0..1 which we need to map to 'real values', according to range.
encoders send +/- , so need to have some kind of inc/dec scaling
midi sends 0..127, scale to 0..1

seems like layer above, should scale to common (0..1), then set via absoluteChange(), relativeChange()

## midi mapping / learn
allow assignment of parameter to a midi cc
device later could allow learn, e.g. activate learn, twist param, fire cc

## slots
we could potentially have same module in 2 different slots, 
also parameter names cannot be considered unique across module.

preset values: are not 'slot based'?, i.e. if i recall 'hall reverb', it doesnt matter what slot its in.
messages sent out: are slot based, ie. the reverb in slot 1, is different from reverb in slot 4


does this necessitate modules or slot?
e.g.

(module page param displayname min max def)
param rngs gen r_mix pct "mix" 0 100 50 


hmm, issue is the pd file with the definition wil be loaded multiple times, and potentially in different orders/times.

(slot page param displayname min max def)
param slot1 gen r_mix pct "mix" 0 100 50 

do we really want a singleton?
(i think so, just think it needs indexing correctly)


## preset file
lets start with (e.g.) 10 presets, which can be saved/recalled
store presets as param name , value

## one pure data obect or many?
if we want to send in encoders/midi etc, then we dont want this on multiple oKontrol objects, only 1,  preferable in a kind of mother patch. (this lends itself to extension later, to handle slots etc)

oKontrol <- param <- page
oKontrolCentral <-knobs -> screen

or even
oKcontrolDevice/Organelle <- knobs ->screen

send/recieve in PD, need to scope to slots, but probably ok to be unique in patch?
(this would mean, you need to have unique names, if you start sharing bits of patches)
probaby ok for now.


# pure data messages:

page page1 "reverb"  - adds a page called page1, displayed as reverb

param r_mix page1 pct "mix" 0 100 50 - add parameter r_mix to page 1, displayed as mix, 0 to 100 percent, 50 default
param r_active page1 bool "active" on 
param r_time page1 msec "time"  
param r_feedback page1 pct "feedback" on 

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
```
I ======    O ==========
|mix                45%|
|active              on|
|time         1500 mSec|
|feedback           45%|
Aux = blah      [Reverb]
```

twist encoder, display next/prev page as overlay, only display params on encoder up
```
I ======    O ==========
|mix                45%|
|ac------------------on|
|ti|<  Oscs        >|ec|
|fe------------------5%|
Aux = blah      [Reverb]
```



# things to check:
pd - do quoted strings get brought in as single symbols, or do I need to read by looking for quotes? same for output?
pd - special characters in symbols

pd/organelle
- can i open 2nd listen port for encoder/cc etc...
- or have an pure data input for organelle for midi/pots/encoder
- output - direct to mother exec, or go via mother.pd (screen)

# osc interactions
what do message send/recieve look like?

setup - possible similar to pd

/prefix/page page1 reverb
/prefix/param r_mix page1 pct "mix" 0 100 50 

commands
/prefix/paramValue r_mix 1.0
/prefix/requestMeta
