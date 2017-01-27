#include "MLVector.h"

#include <xmmintrin.h>

MLVec MLVec::getIntPart() const
{
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	return MLVec(_mm_cvtepi32_ps(vi));
}

MLVec MLVec::getFracPart() const
{
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	__m128 intPart = _mm_cvtepi32_ps(vi);
	return MLVec(_mm_sub_ps(val.v, intPart));
}

void MLVec::getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const
{
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	intPart = _mm_cvtepi32_ps(vi);
	fracPart = _mm_sub_ps(val.v, intPart.val.v);
}
