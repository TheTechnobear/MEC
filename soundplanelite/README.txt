This directory contains a cutdown collection of files from the madrona libraries.

unlike the 'soundplane' directory, these files may have been modified more extensively to allow streamlining


I'll try to keep track of broad changes here, so that re-porting files might be easier, as there is some idea of what has changed

TODO/Check
- MLVector  
operator / , NEON can it use same methods as SSE2NEON uses? , this method is only used in makeTemplate so not on performance line


notes:
SSE/NEON/NOFPU variations are in separate files, in subdiretories in source.
header files include appropriate, for source files CMAKE file defines which is used

- sse/MLSignal.cpp ,sse/ MLDSP.h are not needed, since their methods are not invoked/used by touchtracker etc
(MLDSP_EXTENDED is not defined so MLDSP.h does not include)


CHANGES:
- move MLApp/DSP/Core/Soundplane etc all into one directory
- MLDSP.h MLVector.h - separate out SSE, so we can have SSE and NEON - generic header includes as appropriate
- MLSignal, MLVector - as above

