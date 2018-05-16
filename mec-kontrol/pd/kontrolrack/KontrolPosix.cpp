#ifndef _WIN32

#include "KontrolRack.h"

__attribute__((destructor)) void kontrol_uninit(void)
{
	// Delete the local rack on exiting, if there is one.
	auto localRackId = Kontrol::KontrolModel::model()->localRackId();
	if (!localRackId.empty())
		Kontrol::KontrolModel::model()->deleteRack(Kontrol::CS_LOCAL, localRackId);
}

#endif
