#ifdef _WIN32

#include "KontrolRack.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BOOLEAN WINAPI DllMain(
    IN HINSTANCE dllHandle,
    IN DWORD     reason,
    IN LPVOID    reserved)
{
    if (reason == DLL_PROCESS_DETACH)
    {
        KontrolRack_cleanup();
    }

    return TRUE;
}

#endif
