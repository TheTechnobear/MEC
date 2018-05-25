# Kontrol

    oscdump 9001

## protocol 

    /Kontrol/ping i 9001 0

(0 = keep alive time (seconds) , if > 0 server will drop connection if next ping is not recieved in 2 x keep alive time)

## data model changes
    /Kontrol/rack ssi "127.0.0.1:9000" "127.0.0.1" 9000
    /Kontrol/module ssss "127.0.0.1:9000" "braids" "Braids" "brds"
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "int" "r_amt" "Amount" 0.000000 47.000000 0.000000
    /Kontrol/page ssssssss "127.0.0.1:9000" "braids" "pg_rvb" "Reverb" "r_amt" "r_time" "r_diff" "r_cutoff"
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "r_amt" 20
    
    /Kontrol/deleteRack s "127.0.0.1:9000" 
    

## loadmodule 
    /Kontrol/loadModule s "127.0.0.1:9000" "m0" "brds"


## save and recall settings
    /Kontrol/applyPreset ss "127.0.0.1:9000" "Default"
    /Kontrol/updatePreset ss "127.0.0.1:9000" "Default"
    /Kontrol/saveSettings s "127.0.0.1:9000"

## assigning midi to parameters
    /Kontrol/assignMidiCC sssi "127.0.0.1:9000" "braids" "r_amt" 1
    /Kontrol/unassignMidiCC sssi "127.0.0.1:9000" "braids" "r_amt" 1
    
## assigning parameter to modulation bus
    /Kontrol/assignModulation sssi "127.0.0.1:9000" "braids" "r_amt" 1
    /Kontrol/unassignModulation sssi "127.0.0.1:9000" "braids" "r_amt" 1
    
## moduation/ midi learn activate/deactivate
    /Kontrol/midiLearn 1
    /Kontrol/modulationLearn 1

note: assignments/learn are help in the current preset, and persisted with save settings

## resources
    /Kontrol/resource sss "127.0.0.1:9000" "m0" "module" "brds"


## 'dumb' client example

assuming kontrol patch and MEC running 

    oscdump 9001
    oscsend localhost 8000 /Kontrol/ping i 9001 0

this initial ping, as KA = 0 , so is not expected to keep pinging
(when you do ping with KA=0 you will get the current meta data)

you seem something like

    /Kontrol/rack ssi "127.0.0.1:9000" "127.0.0.1" 9000
    /Kontrol/module ssss "127.0.0.1:9000" "braids" "Braids" "brds"
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "int" "o_shape" "Shape" 0.000000 47.000000 0.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "pitch" "o_transpose" "Transpose" -24.000000 24.000000 0.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "pct" "r_time" "Time" 0.000000 100.000000 50.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "pct" "e_sustain" "Sustain" 0.000000 100.000000 50.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "pct" "o_colour" "Colour" 0.000000 100.000000 50.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "time" "e_release" "Release" 0.000000 2000.000000 50.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "time" "e_decay" "Decay" 0.000000 2000.000000 500.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "freq" "f_cutoff" "Cutoff" 0.000000 15000.000000 10000.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "pct" "f_reson" "Resonance" 0.000000 100.000000 0.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "pct" "o_timbre" "Timbre" 0.000000 100.000000 50.000000
    /Kontrol/param sssssfff "127.0.0.1:9000" "braids" "pct" "f_env_amt" "E.Amount" 0.000000 100.000000 0.000000
    /Kontrol/page ssssssss "127.0.0.1:9000" "braids" "pg_osc" "Oscillator" "o_shape" "o_colour" "o_timbre" "o_transpose"
    /Kontrol/page ssssssss "127.0.0.1:9000" "braids" "pg_vca" "Vca" "e_attack" "e_decay" "e_sustain" "e_release"
    /Kontrol/page sssssss "127.0.0.1:9000" "braids" "pg_vcf" "Vcf" "f_cutoff" "f_reson" "f_env_amt"
    /Kontrol/page ssssssss "127.0.0.1:9000" "braids" "pg_fenv" "F.Env" "f_attack" "f_decay" "f_sustain" "f_release"
    /Kontrol/page ssssssss "127.0.0.1:9000" "braids" "pg_rvb" "Reverb" "r_amt" "r_time" "r_diff" "r_cutoff"
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "o_shape" 45.000000
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "o_transpose" 23.000000
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "r_time" 100.000000
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "e_sustain" 50.000000
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "o_colour" 63.281250
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "e_release" 50.000000
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "e_decay" 500.000000
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "f_cutoff" 10000.000000
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "f_reson" 0.000000
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "o_timbre" 70.312500
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "f_env_amt" 0.000000

these messages are provided so that a client knows what parameters are available and what their current values are

you will then get changed values, if another 'client' alters the values

    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "o_timbre" 20.1
    /Kontrol/changed sssf "127.0.0.1:9000" "braids" "o_timbre" 20.2

clients are expected to track changes so that if a user changes values via midi or from another client, they are reflectected throughout.
the parameters meta data provide minimum and maximum, anything outside of this will be clipped (3rd value is default)
parameters ids, should not be hard coded , since its possible these can change
(pages are used to give logical user groupings and order)



### change a value 

    oscsend localhost 8000 /Kontrol/changed sssf "127.0.0.1:9000" "braids" "o_shape" 20.0


### apply and update presets

    oscsend localhost 8000 /Kontrol/applyPreset ss "127.0.0.1:9000" "Default"
    oscsend localhost 8000 /Kontrol/updatePreset ss "127.0.0.1:9000" "Default"

(currently "Default" is always loaded at patch startup)

### save settings (for when patch is restarted)

    oscsend localhost 8000 /Kontrol/saveSettings s "127.0.0.1:9000"

### assign midi cc for midi devices connected to patch

    oscsend localhost 8000 /Kontrol/assignMidiCC sssi "127.0.0.1:9000" "braids" "r_amt" 1
    oscsend localhost 8000 /Kontrol/unassignMidiCC sssi "127.0.0.1:9000" "braids" "r_amt" 1


