
#include "MLVector.h"

MLVec MLVec::getIntPart() const
{
	return MLVec((int)val.f[0],(int)val.f[1],(int)val.f[2],(int)val.f[3]);
}

MLVec MLVec::getFracPart() const
{
	return *this - getIntPart();
}

void MLVec::getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const
{
	MLVec ip = getIntPart();
	intPart = ip;
	fracPart = *this - ip;
}
