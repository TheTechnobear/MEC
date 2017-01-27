
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_VECTOR_SSE__
#define __ML_VECTOR_SSE__

#include <xmmintrin.h>

typedef union 
{
    __m128 v;
	float f[4];
} MLV4;


class MLVec
{
public:
	MLVec() { clear(); }
	MLVec(const float f) { set(f); }
	
	virtual ~MLVec(){};

	MLVec(const MLVec& b) : val(b.val) {}
	MLVec(const float a, const float b, const float c, const float d)
		{ val.f[0] = a; val.f[1] = b; val.f[2] = c; val.f[3] = d; }
	MLVec(const float* fb) 
		{ val.f[0] = fb[0]; val.f[1] = fb[1]; val.f[2] = fb[2]; val.f[3] = fb[3]; }
	inline MLVec(__m128 vk) { val.v = vk; }
	inline void clear() { val.v = _mm_setzero_ps(); }
	inline void set(float f) { val.v = _mm_set1_ps(f); }
	inline MLVec & operator+=(const MLVec& b) { val.v = _mm_add_ps(val.v, b.val.v); return *this; }
	inline MLVec & operator-=(const MLVec& b) { val.v = _mm_sub_ps(val.v, b.val.v); return *this; }	
	inline MLVec & operator*=(const MLVec& b) { val.v = _mm_mul_ps(val.v, b.val.v); return *this; }
	inline MLVec & operator/=(const MLVec& b) { val.v = _mm_div_ps(val.v, b.val.v); return *this; }
	inline const MLVec operator-() const { return MLVec(_mm_sub_ps(_mm_setzero_ps(), val.v)); }

	// inspector, return by value
	inline float operator[] (int i) const { return val.f[i]; }
	// mutator, return by reference
	inline float& operator[] (int i) { return val.f[i]; }

	bool operator==(const MLVec& b) const;
	bool operator!=(const MLVec& b) const;
 
	inline const MLVec operator+ (const MLVec& b) const { return MLVec(*this) += b; }
	inline const MLVec operator- (const MLVec& b) const { return MLVec(*this) -= b; }
	inline const MLVec operator* (const MLVec& b) const { return MLVec(*this) *= b; }
	inline const MLVec operator/ (const MLVec& b) const { return MLVec(*this) /= b; }

	inline MLVec & operator*=(const float f) { (*this) *= MLVec(f); return *this; }
	inline const MLVec operator* (const float f) const { return MLVec(*this) *= f; }

	virtual float magnitude() const;
	void normalize();
	MLVec getIntPart() const;
	MLVec getFracPart() const;
	void getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const;

	MLV4 val;
};


inline const MLVec vmin(const MLVec&a, const MLVec&b) { return MLVec(_mm_min_ps(a.val.v, b.val.v)); }
inline const MLVec vmax(const MLVec&a, const MLVec&b) { return MLVec(_mm_max_ps(a.val.v, b.val.v)); }
inline const MLVec vclamp(const MLVec&a, const MLVec&b, const MLVec&c) { return vmin(c, vmax(a, b)); }
inline const MLVec vsqrt(const MLVec& a) { return MLVec(_mm_sqrt_ps(a.val.v)); }


#endif 

