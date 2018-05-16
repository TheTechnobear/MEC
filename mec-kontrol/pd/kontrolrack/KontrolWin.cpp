#ifdef _WIN32

#include "KontrolRack.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BOOLEAN WINAPI DllMain(
	IN HINSTANCE dllHandle,
	IN DWORD     reason,
	IN LPVOID    reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_DETACH:
		{
			// Delete the local rack on exiting, if there is one.
			auto localRackId = Kontrol::KontrolModel::model()->localRackId();
			if (!localRackId.empty())
				Kontrol::KontrolModel::model()->deleteRack(Kontrol::CS_LOCAL, localRackId);
			break;
		}
	}

	return TRUE;
}

#endif
