
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "Filters2D.h"

#pragma mark Biquad2D

Biquad2D::Biquad2D(int w, int h) 
{
	setDims(w, h);
}
	
Biquad2D::~Biquad2D()
{
}

void Biquad2D::setDims(int w, int h) 
{
	mX1.setDims(w, h);
	mX2.setDims(w, h);
	mY1.setDims(w, h);
	mY2.setDims(w, h);
	mTemp1.setDims(w, h);
	mAccum.setDims(w, h);
}

void Biquad2D::process(int)
{
	mAccum.copy(*mpIn);
	mAccum.scale(mCoeffs.a0);

	mTemp1.copy(mX1);
	mTemp1.scale(mCoeffs.a1);
	mAccum.add(mTemp1);

	mTemp1.copy(mX2);
	mTemp1.scale(mCoeffs.a2);
	mAccum.add(mTemp1);

	mTemp1.copy(mY1);
	mTemp1.scale(mCoeffs.b1);
	mAccum.subtract(mTemp1);

	mTemp1.copy(mY2);
	mTemp1.scale(mCoeffs.b2);
	mAccum.subtract(mTemp1);

	mX2.copy(mX1);
	mX1.copy(*mpIn);
	mY2.copy(mY1);
	mY1.copy(mAccum);

	mpOut->copy(mAccum);
}

void Biquad2D::dumpCoeffs()
{
	debug() << "{a0: " << mCoeffs.a0 << ", a1: " << mCoeffs.a1 << ", a2: " << mCoeffs.a2 << 
	", b1: " << mCoeffs.b1 << ", b2: " << mCoeffs.b2 << " }";
}


#pragma mark Biquad2DMatrix


Biquad2DMatrix::Biquad2DMatrix(int w, int h)
{
	setDims(w, h);
}

Biquad2DMatrix::~Biquad2DMatrix()
{
}

void Biquad2DMatrix::setDims(int w, int h)
{
	mCoeffsMatrix.setDims(w, h);
	mX1.setDims(w, h);
	mX2.setDims(w, h);
	mY1.setDims(w, h);
	mY2.setDims(w, h);
	mA0.setDims(w, h);
	mA1.setDims(w, h);
	mA2.setDims(w, h);
	mB1.setDims(w, h);
	mB2.setDims(w, h);
	mTemp1.setDims(w, h);
	mAccum.setDims(w, h);
}
void Biquad2DMatrix::setLopass(const MLSignal& fMatrix, float q)
{ 
	float f;
	int w = mCoeffsMatrix.getWidth();
	int h = mCoeffsMatrix.getHeight();
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			f = fMatrix(i, j);
			
			// calc all coefficients using coeffs object for convenience TODO optimize
			mCoeffs.setLopass(f, q); 
			mA0(i, j) = mCoeffs.a0;
			mA1(i, j) = mCoeffs.a1;
			mA2(i, j) = mCoeffs.a2;
			mB1(i, j) = mCoeffs.b1;
			mB2(i, j) = mCoeffs.b2;
		}
	}
}

void Biquad2DMatrix::setHipass(const MLSignal& fMatrix, float q)
{ 
	float f;
	int w = mCoeffsMatrix.getWidth();
	int h = mCoeffsMatrix.getHeight();
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			f = fMatrix(i, j);
			
			// calc all coefficients using coeffs object for convenience TODO optimize
			mCoeffs.setHipass(f, q); 
			mA0(i, j) = mCoeffs.a0;
			mA1(i, j) = mCoeffs.a1;
			mA2(i, j) = mCoeffs.a2;
			mB1(i, j) = mCoeffs.b1;
			mB2(i, j) = mCoeffs.b2;
		}
	}
}

void Biquad2DMatrix::setOnePole(const MLSignal& fMatrix)
{ 
	float f;
	int w = mCoeffsMatrix.getWidth();
	int h = mCoeffsMatrix.getHeight();
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			f = fMatrix(i, j);
			
			// do calc using coeffs object for convenience TODO optimize
			mCoeffs.setOnePole(f); 
			mA0(i, j) = mCoeffs.a0;
			mA1(i, j) = mCoeffs.a1;
			mA2(i, j) = mCoeffs.a2;
			mB1(i, j) = mCoeffs.b1;
			mB2(i, j) = mCoeffs.b2;
		}
	}
}
			
void Biquad2DMatrix::setDifferentiate()
{ 
	int w = mCoeffsMatrix.getWidth();
	int h = mCoeffsMatrix.getHeight();
	mCoeffs.setDifferentiate(); 
	
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			mA0(i, j) = mCoeffs.a0;
			mA1(i, j) = mCoeffs.a1;
			mA2(i, j) = mCoeffs.a2;
			mB1(i, j) = mCoeffs.b1;
			mB2(i, j) = mCoeffs.b2;
		}
	}
}
			
void Biquad2DMatrix::process(int)
{
	mAccum.copy(*mpIn);
	mAccum.multiply(mA0);

	mTemp1.copy(mX1);
	mTemp1.multiply(mA1);
	mAccum.add(mTemp1);

	mTemp1.copy(mX2);
	mTemp1.multiply(mA2);
	mAccum.add(mTemp1);

	mTemp1.copy(mY1);
	mTemp1.multiply(mB1);
	mAccum.subtract(mTemp1);

	mTemp1.copy(mY2);
	mTemp1.multiply(mB2);
	mAccum.subtract(mTemp1);

	mX2.copy(mX1);
	mX1.copy(*mpIn);
	mY2.copy(mY1);
	mY1.copy(mAccum);

	mpOut->copy(mAccum);
}

#pragma mark AsymmetricOnepoleMatrix


AsymmetricOnepoleMatrix::AsymmetricOnepoleMatrix(int w, int h)
{
	setDims(w, h);
}

AsymmetricOnepoleMatrix::~AsymmetricOnepoleMatrix()
{
}

void AsymmetricOnepoleMatrix::setDims(int w, int h)
{
	mY1.setDims(w, h);
	mDx.setDims(w, h);
	mSign.setDims(w, h);
	mK.setDims(w, h);
	mA.setDims(w, h);
	mB.setDims(w, h);
}

void AsymmetricOnepoleMatrix::setCoeffs(const MLSignal& fa, const MLSignal& fb)
{
	float sr = mSampleRate;
	float isr = 1.f / sr;
	float k;
	for(unsigned i=0; i<mA.getSize(); ++i)
	{	
		k = kMLTwoPi * fa[i] * isr;
		mA[i] = clamp(k, 0.f, 0.25f);
	}

	for(unsigned i=0; i<mB.getSize(); ++i)
	{	
		k = kMLTwoPi * fb[i] * isr;
		mB[i] = clamp(k, 0.f, 0.25f);
	}
}
			
void AsymmetricOnepoleMatrix::process(int)
{
	//	dxdt = x[n] - mY1;	
	mDx.copy(*mpIn);
	mDx.subtract(mY1);
	
	//	s = (dxdt < 0.f ? -1.f : 1.f);	
	mSign.copy(mDx);
	mSign.ssign();
	
	// rising: time constant is A
	// falling: time constant is B
	// k = ((1.f - s)*mB + (1.f + s)*mA)*0.5f;
	for(unsigned i=0; i<mY1.getSize(); ++i)
	{	
		MLSample s, a, b;
		s = mSign[i];
		a = mA[i];
		b = mB[i];
		mK[i] = ((1.f - s)*b + (1.f + s)*a)*0.5f;
	}
		
	// one pole 
	// mY1 += k*dxdt;
	// y[n] = mY1;
	mDx.multiply(mK);
	mY1.add(mDx);
	mpOut->copy(mY1);
}


#pragma mark OnepoleMatrix


OnepoleMatrix::OnepoleMatrix(int w, int h)
{
	setDims(w, h);
}

OnepoleMatrix::~OnepoleMatrix()
{
}

void OnepoleMatrix::setDims(int w, int h)
{
	mY1.setDims(w, h);
	mDx.setDims(w, h);
	mA.setDims(w, h);
}

void OnepoleMatrix::setCoeffs(const MLSignal& fa)
{
	float sr = mSampleRate;
	float isr = 1.f / sr;
	float k;
	for(unsigned i=0; i<mA.getSize(); ++i)
	{	
		k = kMLTwoPi * fa[i] * isr;
		mA[i] = clamp(k, 0.f, 0.25f);
	}
}
			
void OnepoleMatrix::process(int)
{
	//	dxdt = x[n] - mY1;	
	mDx.copy(*mpIn);
	mDx.subtract(mY1);
	
	// one pole 
	// mY1 += a*dxdt;
	// y[n] = mY1;
	mDx.multiply(mA);
	mY1.add(mDx);
	mpOut->copy(mY1);
}

#pragma mark BoxFilter2D

const int BoxFilter2D::kMaxN = 50;

BoxFilter2D::BoxFilter2D(int w, int h) 
{
	mDelay.resize(kMaxN);
	setDims(w, h);
	mDelayIdx = 0;
}

BoxFilter2D::~BoxFilter2D()
{
}

void BoxFilter2D::clear() 
{
	for(int i=0; i<mDelay.size(); ++i)
	{
		mDelay[i].clear();
	}
	mAccum.clear(); 
	mDelayIdx = 0;
}

void BoxFilter2D::setDims(int w, int h) 
{
	for(int i=0; i<mDelay.size(); ++i)
	{
		mDelay[i] = MLSignal(w, h);
	}
	
	mAccum.setDims(w, h);
	mDelayIdx = 0;
}

void BoxFilter2D::setN(int n)
{ 
	mN = clamp(n, 1, kMaxN); 
	mScale = 1.0f / static_cast<float>(n); 
}

void BoxFilter2D::process(int)
{
	mDelayIdx++;
	if(mDelayIdx >= mN)
	{
		mDelayIdx = 0;
	}
	mDelay[mDelayIdx].copy(*mpIn);
	
	mAccum.clear();
	for(int i=0; i<mN; ++i)
	{
		mAccum.add(mDelay[i]);
	}
	
	mAccum.scale(mScale);
	
	mpOut->copy(mAccum);
}
