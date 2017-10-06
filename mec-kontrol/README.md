# Important Note #
This is currently being integrated into the MEC project, from being a standalone project.
Once fully integrated the build/dev notes etc will all be part of the normal mec system.


## Windows note ##
as I dont currently target mec at windows (but will do), its likely I'll keep the pd-lib-builder working for windows externals initially.
if you find its missing source files (i.e. doesn't link), check the CMakeFiles.txt for a definitive list

-----

## Technobear's kontrol
kontrol is a system for managing parameters, allowing abstraction of the input and display mechanism, and including navigatonal aids

## Status ##
not released, development only - not working, do not use ;) 

## Contributions ##
Its best for users if there is a single source for these externals, so if you wish to extend/fix/contribute, Im very happy to take Pull Requests for enhancements / bug fixes etc.

## Externals ##
kontrol - parameter control
kontrolhost~ - interface to organelle, note DSP needs to be turned on to work

## Folder Structure ##
api - core functionality, target independent
pd - pure data externals
pd/kontrol - Kontrol pure data external for add params etc
pd/kontrolhost - pure data external for interfacing to Organelle

## Building ##
    cd mec_kontrol/pd/kontrol
    make
    cd mec_kontrol/pd/kontrolhost
    make

Notes:

you will need a copy of pure data installed.
By default, pd-lib-builder will attempt to auto-locate an install of Pure Data, to find the appropriate header files.
If your puredata setup is non-standard, you can use PDINCLUDEDIR and PDLIBDIR

externals will be installed into the library directory, under the 'mec' library.

## Example builds ##
building instaling using defaults

    make install 


a non standard build when PD needs to be specified, for examle Bela you can use

	make PDINCLUDEDIR=~/Bela/include/libpd/ PDLIBDIR=~/Bela/projects/pd-externals install


See `make help` for more details.

This build uses [pd-lib-builder](https://github.com/pure-data/pd-lib-builder/)
