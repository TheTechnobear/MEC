FROM initial G+ post for ideas....
gives a basic overview of where I want to go...


(note: the terminology may change, e.g. surface, perhaps will be 'zone'... and I dont like row/column, as its a bit confusing) 

the way I current envisage it is:
device , something like a eigenharp/soundplane/osc/mpe, this exposes a number of 'surfaces' (e.g. keys as one, strip 1, strip 2 , breath as others), then mapped into other surfaces, then on to a musical scale (optionally)

something like :

eigenharp 1
|
|- surface 1
     | - surface 10   - >  scaler 1
     |
     | - surface 11   - >  scaler 2
|
|- surface 2 - - --- -- --- ---- --- ---------------
                                                   |
                                                   +------ surf 40 (sliders)
soundplane                                         |
|                                                  |
|- surface 3 - - + - - surface 30 - -------------- |
                 |
                 | - - surface 31 - > scaler 3

midi device (e.g pedal)
|
|- surface 4 -> buttons


a surface is an x/y/z , and is subdivided into row/col, (cells) 
a touch is - touch id, surface id , x, y, z (0..1), r,c 

the touch can be passed thru a number of surface mappers which can 
split : by x/y/ z(!) / r /c  
join: by axis x/y/z
when splitting/joining a new touch is calculated, touch id needs to be unique in this 'surface area', and x/y/z stay in range 0..1, r/c on the joined area will need updating... new surface created (by join/split) have their own unique id.

a raw surface can then pass thru a scaler, to provide a 'musical surface' i.e. one with notes...the scaler is configured with a scale and then the note is is derived from r/c/x/y/z, this results in a 'musical touch'

musical touch is:  touch id, surface id , x, y, z (0..1), r,c, note, scaler id

scaler will deal with scales, tonic, transpose.... i.e. it combines the 'musical mapping' and scaler duties from Eigen

the mappings can't change at runtime, (at least initially) but things like active surface/scales/transpose/tonic/octave can.

an application can subscribe either to a raw surface (without musical mapping ) or a musical surface (with scale) 

the raw surfaces I see being used for things like 'button mapping' and also for controls like breath/strip... 

it may be these need to somehow be 'identifiable' or allow scaling, such that an app can easily turn it into midi, or do something (like change octave) 

thats the basics of my plans
have I missed anything obvious... is this mapping reasonable?

note: 
some limitations/assumptions are going to be needed (at least initially) , to keep it from becoming 'overwhelming' for both coding, and also users to configure.

