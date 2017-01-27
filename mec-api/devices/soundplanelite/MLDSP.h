
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_DSP_H
#define _ML_DSP_H

// NOTE should require nothing

#ifdef _WIN32
	#include <memory>
#else
	//#include <tr1/memory>
#endif

#include <math.h>
#ifdef _WIN32
	#define	MAXFLOAT	((float)3.40282346638528860e+38)
#endif

#ifndef MAXFLOAT
	#include <float.h>
	#define MAXFLOAT FLT_MAX
#endif

#define SHUFFLE(a, b, c, d) ((a<<6) | (b<<4) | (c<<2) | (d))
#define SHUFFLE_R(a, b, c, d) ((d<<6) | (c<<4) | (b<<2) | (a))

#include <cassert>
#include <iostream>
#include <stdint.h>
#include <string>
#include <string.h>

#ifndef DEBUG
#define force_inline  inline __attribute__((always_inline))
#else
#define force_inline  inline
#endif

// logs for Windows.   
#ifdef _WIN32
inline double log2( double n )   
{       
	return log( n ) / log( 2. );   
} 
inline float log2f(float n)
{
	return logf(n) / 0.693147180559945309417232121458176568f;
}
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif // _MSC_VER

// ----------------------------------------------------------------
#pragma mark Types
// ----------------------------------------------------------------

typedef float MLSample;
typedef double MLDouble;
typedef int MLSampleRate;
typedef float MLParamValue;

// ----------------------------------------------------------------
#pragma mark Engine Constants
// ----------------------------------------------------------------

const uintptr_t kMLProcessChunkBits = 6;     // signals are always processed in chunks of this size.
const uintptr_t kMLProcessChunkSize = 1 << kMLProcessChunkBits;

const uintptr_t kMLSamplesPerSSEVectorBits = 2;
const uintptr_t kSSEVecSize = 1 << kMLSamplesPerSSEVectorBits;

const int kMLEngineMaxVoices = 8;

const uintptr_t kMLAlignBits = 6; // cache line is 64 bytes
const uintptr_t kMLAlignSize = 1 << kMLAlignBits;
const uintptr_t kMLAlignMask = ~(kMLAlignSize - 1);

const float kMLTwoPi = 6.2831853071795864769252867f;
const float kMLPi = 3.1415926535897932384626433f;
const float kMLOneOverTwoPi = 1.0f / kMLTwoPi;
const float kMLTwelfthRootOfTwo = 1.05946309436f;

const float kMLMinGain = 0.00001f; // 10e-5 = -120dB

const MLSampleRate kMLTimeless = -1;
const MLSampleRate kMLToBeCalculated = 0;

const MLSample kMLMaxSample = MAXFLOAT;
const MLSample kMLMinSample = -MAXFLOAT;

// ----------------------------------------------------------------
#pragma mark utility functions
// ----------------------------------------------------------------

// return bool as float 0. or 1.
inline float boolToFloat(uint32_t b)
{
	uint32_t temp = 0x3F800000 & (!b - 1);
	return *((float*)&temp);
}

// return sign bit of float as float, 1. for positive, 0. for negative.
inline float fSignBit(float f)
{
	uint32_t a = *((uint32_t*)&f);
	a = (((a & 0x80000000) >> 31) - 1) & 0x3F800000;
	return *((float*)&a);
}

MLSample* alignToCacheLine(const MLSample* p);
int bitsToContain(int n);
int ilog2(int n);

inline MLSample lerp(const MLSample a, const MLSample b, const MLSample m)
{
	return(a + m*(b-a));
}

inline MLSample lerpBipolar(const MLSample a, const MLSample b, const MLSample c, const MLSample m)
{
	MLSample absm = fabsf(m);	// TODO fast abs etc
	MLSample pos = m > 0.;
	MLSample neg = m < 0.;
	MLSample q = pos*c + neg*a;
	return (b + (q - b)*absm);
}		

float scaleForRangeTransform(float a, float b, float c, float d); // TODO replace with MLRange object
float offsetForRangeTransform(float a, float b, float c, float d);

MLSample MLRand(void);
void MLRandReset(void);

// ----------------------------------------------------------------
#pragma mark portable numeric checks
// ----------------------------------------------------------------

#include <cmath>

#ifdef __INTEL_COMPILER
#include <mathimf.h>
#endif

int MLisNaN(float x);
int MLisNaN(double x);
int MLisInfinite(float x);
int MLisInfinite(double x);

// ----------------------------------------------------------------
#pragma mark min, max, clamp
// ----------------------------------------------------------------

template <class c>
inline c (min)(const c& a, const c& b)
{
	return (a < b) ? a : b;
}

template <class c>
inline c (max)(const c& a, const c& b)
{
	return (a > b) ? a : b;
}

template <class c>
inline c (clamp)(const c& x, const c& min, const c& max)
{
	return (x < min) ? min : (x > max ? max : x);
}

// within range, including start, excluding end value.
template <class c>
inline bool (within)(const c& x, const c& min, const c& max)
{
	return ((x >= min) && (x < max));
}

template <class c>
inline int (sign)(const c& x)
{
	if (x == 0) return 0;
	return (x > 0) ? 1 : -1;
}

float inMinusPiToPi(float theta);


// amp <-> dB conversions, where ratio of the given amplitude is to 1.

float ampTodB(float a);
float dBToAmp(float d);

#pragma mark smoothstep

inline float smoothstep(float a, float b, float x)
{
	x = clamp((x - a)/(b - a), 0.f, 1.f); 
	return x*x*(3.f - 2.f*x);
}

// ----------------------------------------------------------------
#pragma mark fast trig approximations
// ----------------------------------------------------------------

// fastest and worst.  rough approximation sometimes useful in [-pi/2, pi/2].
inline float fsin1(const float x)
{
    return x - (x*x*x*0.15f);
}

inline float fcos1(const float x)
{
	float xx = x*x;
    return 1.f - xx*0.5f*(1.f - xx*0.08333333f);
}

// ----------------------------------------------------------------
#pragma mark fast exp2 and log2 approx
// Courtesy José Fonseca, http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html

// ----------------------------------------------------------------
// interpolation
// ----------------------------------------------------------------

// ----------------------------------------------------------------
#pragma mark  MLRange
// ----------------------------------------------------------------

class MLRange
{
public:
	MLRange() : mA(0.f), mB(1.f), mScale(1.f), mOffset(0.f), mClip(false), mMinOutput(0), mMaxOutput(0) {}
	MLRange(float a, float b) : mA(a), mB(b), mScale(1.f), mOffset(0.f), mClip(false), mMinOutput(0), mMaxOutput(0) {}
	MLRange(float a, float b, float c, float d, bool clip = false) : 
		mA(a), mB(b), mScale(1.f), mOffset(0.f), mClip(clip), mMinOutput(0), mMaxOutput(0) 
	{
		convertTo(MLRange(c, d));
	}
	~MLRange(){}
	float getA() const {return mA;}
	float getB() const {return mB;}
	void setA(float f){mA = f;} 
	void setB(float f){mB = f;}  
	void set(float a, float b){mA = a; mB = b;}
	void setClip(bool c)
	{
		mClip = c;
	} 
	bool getClip() const { return mClip; } 
	
	void convertFrom(const MLRange& r)
	{
		float a, b, c, d;
		a = r.mA;
		b = r.mB;
		c = mA;
		d = mB;
		mScale = (d - c) / (b - a);
		mOffset = (a*d - b*c) / (a - b);
		mMinOutput = min(c, d);
		mMaxOutput = max(c, d);
	}
	void convertTo(const MLRange& r)
	{
		float a, b, c, d;
		a = mA;
		b = mB;
		c = r.mA;
		d = r.mB;
		mScale = (d - c) / (b - a);
		mOffset = (a*d - b*c) / (a - b);
		mMinOutput = min(c, d);
		mMaxOutput = max(c, d);
	}
	
	float operator()(float f) const 
	{
		float r = f*mScale + mOffset;
		if(mClip) r = clamp(r, mMinOutput, mMaxOutput);
		return r;
	}
	
	inline float convert(float f) const
	{
		return f*mScale + mOffset;
	}
	
	inline float convertAndClip(float f) const
	{
		return clamp((f*mScale + mOffset), mMinOutput, mMaxOutput);
	}

    inline bool contains(float f) const
	{
		return ((f > mMinOutput) && (f < mMaxOutput));
	}

private:
	float mA;
	float mB;
	float mScale;
	float mOffset;
	bool mClip;
	float mMinOutput;
	float mMaxOutput;
};

extern const MLRange UnityRange;

inline char * spaceStr( unsigned int numIndents )
{
	static char * pINDENT=(char *)"                                                   ";
	static int LENGTH= (int)strlen( pINDENT );
	int n = numIndents*4;
	if ( n > LENGTH ) n = LENGTH;
	return &pINDENT[ LENGTH-n ];
}

#ifdef MLDSP_EXTENDED

#if defined(ML_USE_SSE)
    #include "source/sse/MLDSP.h"
#elif defined(ML_USE_NEON)
    #include "source/neon/MLDSP.h"
#else
    #include "source/nofpu/MLDSP.h"
#endif

#endif


#endif // _ML_DSP_H

