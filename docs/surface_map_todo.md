see surface map overview for ideas of where this is 'heading'

this docs is more about whats need to get there... and generally tracking things so I dont forget

basic process
----------------

- create surface, touch, musical touch, scaler
- add new callback APIs, which will eventually replace API
- move 1 device (possible OSC, easy to test!) to use it
- update mec-app to use new api
- move other devices to new api (eigenharp, soundplane, mpe)
- move other apps to new api (vst, max, bela)
- test/merge back to master
- delete these docs :) 

notes:
--------------
current ideas is to replace the single callback api with a number, that an app can suscribe too

- MusicalTouch
- Touch
- Service (e.g. shutdown)
- ... (others?)



current notes:
--------------
scaler with scalemanager, loads scales 
rudimentaty join and split surfaces

next: 
surfaces, probably want to move the current surface mapper implemenataiton into a surface type, that allows manual/automatically mapping 
this is probably a R/C type mapping...

other useful surface, could be a 'rotate'

then I need to link the surfaces together in the mec_api, such that the callbacks can be chained... then these need to be linked to the scaler..

a few complications that are coming up
join is a reverse lookup, and it needs access to the surface dimensions, it seems reasonable that surfaces 'know' the dimension , fed from the device...

voices, we dont want to keep recalculating voice id, everytime it goes thru a surface... (but if we dont, voice id are non contiguous (split), or possibly duplicated (join))

relationship between X-C ,  R-Y is there a fixed relationship?
e.g. X,Y are always 0..1 , and C = X * width, R = Y * height
.. this works for soundplane, 
for eigenharps, its means the entires surface becomes X... 
so roll/yaw are calculated... 

this was not 'as intended' where I hoped X/Y were independent of R/C, 
such that it X could be the whole surface (soundplane), or just a relative offset (eigenharp)
but if this is the case, then how can we split, since if you split by X, you dont know how to split Y



