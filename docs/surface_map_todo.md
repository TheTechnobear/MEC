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