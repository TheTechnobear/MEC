
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "TouchTracker.h"

#include <algorithm>

std::ostream& operator<< (std::ostream& out, const Touch & t)
{
	out << std::setprecision(4);
	out << "[" << t.x << ", " << t.y << ", " << t.zf << " (" << t.age << ")]";
	return out;
}

Touch::Touch() : 
	x(0), y(0), z(0), dz(0), zf(0), zf10(0.), dzf(0.), 
	key(-1), age(0), retrig(0), releaseCtr(0)
{
}

Touch::Touch(float px, float py, float pz, float pdz) : 
	x(px), y(py), z(pz), dz(pdz), zf(0.), zf10(0.), dzf(0.), 
	key(-1), age(0), retrig(0), releaseCtr(0)
{
}

TouchTracker::TouchTracker(int w, int h) :
	mWidth(w),
	mHeight(h),
	mpIn(0),
	mpInputMap(0),
	mNumNewCentroids(0),
	mNumCurrentCentroids(0),
	mNumPreviousCentroids(0),
	mMatchDistance(2.0f),
	mNumPeaks(0),
	mOnThreshold(0.03f),
	mOffThreshold(0.02f),
	mTaxelsThresh(9),
	mQuantizeToKey(false),
	mTemplateThresh(0.003),
	mOverrideThresh(0.01),
	mCount(0),
	mMaxTouchesPerFrame(0),
	mBackgroundFilter(1, 1),
	mNeedsClear(true),
	mKeyboardType(rectangularA),
	mCalibrator(w, h),
	mSampleRate(1000.f),
	mBackgroundFilterFreq(0.125f),
	mPrevTouchForRotate(0),
	mRotate(false),
	mDoNormalize(true)
{
	mTouches.resize(kTrackerMaxTouches);	
	mTouchesToSort.resize(kTrackerMaxTouches);	

	// TODO checks
	if(mpInputMap) 
	{
		delete[] mpInputMap;
		mpInputMap = 0;
	}
	mpInputMap = new e_pixdata[w*h];
	
	mTestSignal.setDims(w, h);
	mCalibratedSignal.setDims(w, h);
	mCookedSignal.setDims(w, h);
	mBackground.setDims(w, h);
	mBackgroundFilterFrequency.setDims(w, h);
	mBackgroundFilterFrequency2.setDims(w, h);
	mFilteredInput.setDims(w, h);
	mSumOfTouches.setDims(w, h);
	mInhibitMask.setDims(w, h);
	mTemplateMask.setDims(w, h);
	mInputMinusBackground.setDims(w, h);
	mCalibrationProgressSignal.setDims(w, h);
	mResidual.setDims(w, h);
	mFilteredResidual.setDims(w, h);
	mTemp.setDims(w, h);
	mTempWithBorder.setDims(w + 2, h + 2);
	mRetrigTimer.setDims(w, h);
	mDzSignal.setDims(w, h);
	mBackgroundFilter.setDims(w, h);
	mBackgroundFilter.setSampleRate(mSampleRate);
	mTemplateScaled.setDims (kTemplateSize, kTemplateSize);

	mNumKeys = 150; // Soundplane A
	mKeyStates.resize(mNumKeys);
	for(int i=0; i<mNumKeys; ++i)
	{
		mKeyStates[i].mKeyCenter = getKeyCenterByIndex(i);
	}
}
		
TouchTracker::~TouchTracker()
{
	if(mpInputMap) delete[] mpInputMap;
}

void TouchTracker::setInputSignal(MLSignal* pIn)
{ 
	mpIn = pIn; 

}

void TouchTracker::setOutputSignal(MLSignal* pOut)
{ 
	mpOut = pOut; 
	int w = pOut->getWidth();
	int h = pOut->getHeight();
	
	if (w < 5)
	{
		debug() << "TouchTracker: output signal too narrow!\n";
		return;
	}
	if (h < mMaxTouchesPerFrame)
	{
		debug() << "error: TouchTracker: output signal too short to contain touches!\n";
		return;
	}
}

void TouchTracker::setMaxTouches(int t)
{
	int newT = clamp(t, 0, kTrackerMaxTouches);
	if(newT != mMaxTouchesPerFrame)
	{
		mMaxTouchesPerFrame = newT;
	}
}

// return the index of the key over the point p.
//
int TouchTracker::getKeyIndexAtPoint(const Vec2 p) 
{
	int k = -1;
	float x = p.x();
	float y = p.y();
	int ix, iy;
	switch (mKeyboardType)
	{
		case(rectangularA):
		default:
			MLRange xRange(3.5f, 59.5f);
			xRange.convertTo(MLRange(1.f, 29.f));
			float kx = xRange(x);
			kx = clamp(kx, 0.f, 29.f);
			ix = kx;
			
			MLRange yRange(1.25, 5.75);  // Soundplane A as measured
			yRange.convertTo(MLRange(1.f, 4.f));
			float ky = yRange(y);
			ky = clamp(ky, 0.f, 4.f);
			iy = ky;

			k = iy*30 + ix;
			break;
	}
	return k;
}


// return the center of the key over the point p.
//
Vec2 TouchTracker::getKeyCenterAtPoint(const Vec2 p) 
{
	float x = p.x();
	float y = p.y();
	int ix, iy;
	float fx, fy;
	switch (mKeyboardType)
	{
		case(rectangularA):
		default:
			MLRange xRange(3.5f, 59.5f);
			xRange.convertTo(MLRange(1.f, 29.f));
			float kx = xRange(x);
			kx = clamp(kx, 0.f, 30.f);
			ix = kx;
			
			MLRange yRange(1.25, 5.75);  // Soundplane A as measured
			yRange.convertTo(MLRange(1.f, 4.f));
			float ky = yRange(y);
			ky = clamp(ky, 0.f, 5.f);
			iy = ky;
			
			MLRange xRangeInv(1.f, 29.f);
			xRangeInv.convertTo(MLRange(3.5f, 59.5f));
			fx = xRangeInv(ix + 0.5f);
			
			MLRange yRangeInv(1.f, 4.f);
			yRangeInv.convertTo(MLRange(1.25, 5.75));
			fy = yRangeInv(iy + 0.5f);

			break;
	}
	return Vec2(fx, fy);
}

Vec2 TouchTracker::getKeyCenterByIndex(int idx) 
{
	// for Soundplane A only
	int iy = idx/30;
	int ix = idx - iy*30;
	
	MLRange xRangeInv(1.f, 29.f);
	xRangeInv.convertTo(MLRange(3.5f, 59.5f));
	float fx = xRangeInv(ix + 0.5f);

	MLRange yRangeInv(1.f, 4.f);
	yRangeInv.convertTo(MLRange(1.25, 5.75));
	float fy = yRangeInv(iy + 0.5f);

	return Vec2(fx, fy);
}

int TouchTracker::touchOccupyingKey(int k)
{
	int r = -1;
	for(int i=0; i<mMaxTouchesPerFrame; ++i)
	{
		Touch& t = mTouches[i];
		if (t.isActive())
		{
			if(t.key == k)
			{
				r = i;
				break;
			}
		}
	}
	return r;
}

int TouchTracker::getNeighborFlags(int key)
{
	// Soundplane A dims
	const int width = 30;
	const int height = 5;
	int ky = key/width;
	int kx = key - ky*width;
	bool n = 0;
	bool s = 0;
	bool e = 0;
	bool w = 0;
	bool ne = 0;
	bool nw = 0;
	bool se = 0;
	bool sw = 0;
	bool nOK = (ky < height - 1);
	bool sOK = (ky > 0);
	bool eOK = (kx < width - 1);
	bool wOK = (kx > 0);
	if(nOK)
	{
		n = keyIsOccupied(key + width);
		if(eOK)
		{
			ne = keyIsOccupied(key + width + 1);
		}
		if(wOK)
		{
			nw = keyIsOccupied(key + width - 1);
		}
	}
	if(sOK)
	{
		s = keyIsOccupied(key - width);
		if(eOK)
		{
			se = keyIsOccupied(key - width + 1);
		}
		if(wOK)
		{
			sw = keyIsOccupied(key - width - 1);
		}
	}
	if(eOK)
	{
		e = keyIsOccupied(key + 1);
	}
	if(wOK)
	{
		w = keyIsOccupied(key - 1);
	}
	
//debug() << " n: " << n << " s: " << s << " e: " << e << " w: " << w << "\n";
//debug() << " ne: " << ne << " se: " << se << " nw: " << nw << " sw: " << sw << "\n";

	return (nw<<7) + (n<<6) + (ne<<5) + (w<<4) + (e<<3) + (sw<<2) + (s<<1) + (se<<0);
}
	
void TouchTracker::setRotate(bool b)
{ 
	mRotate = b; 
	if(!b)
	{
		mPrevTouchForRotate = 0;
	}
}

// add new touch at first free slot.  TODO touch allocation modes including rotate.
// return index of new touch.
//
int TouchTracker::addTouch(const Touch& t)
{
	int newIdx = -1;
	int minIdx = 0;
	float minZ = 1.f;
	int offset = 0;
	
	if(mRotate)
	{
		offset = mPrevTouchForRotate;
		mPrevTouchForRotate++;
		if(mPrevTouchForRotate >= mMaxTouchesPerFrame)
		{
			mPrevTouchForRotate = 0;
		}
	}
	
	for(int jr=offset; jr<mMaxTouchesPerFrame + offset; ++jr)
	{
		int j = jr%mMaxTouchesPerFrame;

		Touch& r = mTouches[j];
		if (!r.isActive())
		{		
			r = t;
			r.age = 1;			
			r.releaseCtr = 0;			
			newIdx = j;
			return newIdx;
		}
		else
		{
			mPrevTouchForRotate++;
			float rz = r.z;
			if (r.z < minZ)
			{
				minIdx = j;
				minZ = rz; 
			}
		}
	}
	
	// if we got here, all touches were active.
	// replace the touch with lowest z if new touch is greater.
	//
	if (t.z > minZ)
	{
		int n = mTouches.size();		
		if (n > 0)
		{
			Touch& r = mTouches[minIdx];
			r = t;
			r.age = 1;	
			r.releaseCtr = 0;		
			newIdx = minIdx;
		}		
	}
	return newIdx;
}

// return index of touch at key k, if any.
//
int TouchTracker::getTouchIndexAtKey(const int k)
{
	int r = -1;
	for(int i=0; i<mMaxTouchesPerFrame; ++i)
	{
		Touch& t = mTouches[i];
		if (t.isActive())
		{		
			if(t.key == k)
			{
				r = i;
				break;
			}
		}
	}
	return r;
}

// remove the touch at index by setting the age to 0.  leave the
// position intact for downstream filters etc.  
//
void TouchTracker::removeTouchAtIndex(int touchIdx)
{
	Touch& t = mTouches[touchIdx];
	t.age = 0;
	t.key = -1;
}

void TouchTracker::clear()
{
	for (int i=0; i<mMaxTouchesPerFrame; i++)	
	{
		mTouches[i] = Touch();	
	}
	mBackgroundFilter.clear();
	mNeedsClear = true;
}
	
void TouchTracker::setThresh(float f) 
{ 
	const float kHysteresis = 0.002f;
	mOffThreshold = f; 
	mOnThreshold = mOffThreshold + kHysteresis; 
	mOverrideThresh = mOnThreshold*5.f;
	mCalibrator.setThreshold(mOnThreshold);
}

// if any neighbors of the input coordinates are higher, move the coordinates there.
//
Vec2 TouchTracker::adjustPeak(const MLSignal& in, int xp, int yp)
{
	int width = in.getWidth();
	int height = in.getHeight();

	int x = clamp(xp, 1, width - 2);
	int y = clamp(yp, 1, height - 2);
	
	float t = 0.0;
	int rx = x;
	int ry = y;
	float a = in(x - 1, y - 1);
	float b = in(x, y - 1);
	float c = in(x + 1, y - 1);
	float d = in(x - 1, y);
	float e = in(x, y);
	float f = in(x + 1, y);
	float g = in(x - 1, y + 1);
	float h = in(x, y + 1);
	float i = in(x + 1, y + 1);
	float fmax = e;
	
	if (y > 0)
	{
		if (a > fmax + t)
		{
//			debug() << "<^ ";
			fmax = a; rx = x - 1; ry = y - 1;		
		}
		if (b > fmax + t)
		{
//			debug() << "^ ";
			fmax = b; rx = x; ry = y - 1;		
		}
		if (c > fmax + t)
		{
//			debug() << ">^ ";
			fmax = c; rx = x + 1; ry = y - 1;		
		}
	}
	if (d > fmax + t)
	{
//		debug() << "<- ";
		fmax = d; rx = x - 1; ry = y;		
	}
	if (f > fmax + t)
	{
//		debug() << "-> ";
		fmax = f; rx = x + 1; ry = y;		
	}
	if (y < in.getHeight() - 1)
	{
		if (g > fmax + t)
		{
//			debug() << "<_ ";
			fmax = g; rx = x - 1; ry = y + 1;		
		}
		if (h > fmax + t)
		{
//			debug() << "_ ";
			fmax = h; rx = x; ry = y + 1;		
		}
		if (i > fmax + t)
		{
//			debug() << ">_ ";
			fmax = i; rx = x + 1; ry = y + 1;		
		}
	}
	return Vec2(rx, ry);
}

// if the template matches better at any neighboring coordinates, move there.
//
Vec2 TouchTracker::adjustPeakToTemplate(const MLSignal& in, int x, int y)
{
	int xMin = x;
	int yMin = y;
	float d;
	float dMin = 100000.f;
	for(int j = -1; j < 2; ++j)
	{
		for(int i = -1; i < 2; ++i)
		{
			d = mCalibrator.differenceFromTemplateTouch(in, Vec2(x + i, y + j));
			if(d < dMin)
			{
				dMin = d;
				xMin = x + i;
				yMin = y + j;
			}
		}
	}
	return Vec2(xMin, yMin);
}

class compareZ 
{
public:
	bool operator() (const Vec3& a, const Vec3& b) 
	{ 
		return (a[2] > b[2]);
	}
};


/*
// find peaks by comparing successive samples in each row and each column,
// and crossing off pixels that are lower than others. The pixels remaining
// are the peaks of their 4-neighborhoods.
//
// new peaks are added to the peak list. 
// 
void TouchTracker::findPeaks(const MLSignal& in)
{
	if (!mpInputMap) return;
	int width = in.getWidth();
	int height = in.getHeight();
	unsigned char *	pMapRow;
	int i, j;
	bool slope, prevSlope;
	memset(mpInputMap, 0, width*height*sizeof(e_pixdata));
	
	// left / right
	for (j=0; j<height; ++j)
	{
		pMapRow = mpInputMap + j*mWidth;
		prevSlope = true;
		for(i=0; i<width - 1; ++i)
		{
			slope = in(i + 1, j) > in(i, j);
			if (!prevSlope || slope)
			{
				pMapRow[i] = 1;
			}
			prevSlope = slope;
		}
		i = width - 1; // last column
		{
			slope = false;
			if (!prevSlope || slope)
			{
				pMapRow[i] = 1;
			}
		}
	}
	
	// up / down
	for (i=0; i<width; ++i)
	{
		prevSlope = true;
		for(j=0; j<height - 1; ++j)
		{
			pMapRow = mpInputMap + j*mWidth;
			slope = in(i, j + 1) > in(i, j);
			if (!prevSlope || slope)
			{
				pMapRow[i] = 1;
			}
			prevSlope = slope;
		}
		j = height-1; // last row
		pMapRow = mpInputMap + j*mWidth;
		{
			slope = false;
			if (!prevSlope || slope)
			{
				pMapRow[i] = 1;
			}
		}
	}
	
	// gather all peaks above fixed height.
	mPeaks.clear();
	for (int j=0; j<mHeight; j++)
	{
		pMapRow = mpInputMap + j*mWidth;
		for (int i=0; i < mWidth; i++)
		{
			// if peak
			if (pMapRow[i] == 0)
			{
				float z = in(i, j);
				if (z > mOnThreshold)
				{
					mPeaks.push_back(Vec3(i, j, z));
					if (mPeaks.size() >= kTouchTrackerMaxPeaks) goto done;
				}
			}
		}
	}
	
done:
	std::sort(mPeaks.begin(), mPeaks.end(), compareZ());
	int p = mPeaks.size();
	if(p > 0)
	{
		debug() << p << " peaks\n";
	}
}
*/

void TouchTracker::setLopass(float k)
{ 
	mLopass = k; 
}
			
// --------------------------------------------------------------------------------
#pragma mark main processing

class compareTouchZ 
{
public:
	bool operator() (const Touch& a, const Touch& b) 
	{ 
		return (a.z > b.z);
	}
};

Vec2 correctTouchPosition(const MLSignal& in, int ix, int iy)
{
	int width = in.getWidth();
	int height = in.getHeight();
	
	int x = clamp(ix, 1, width - 2);
	int y = clamp(iy, 1, height - 2);
	
	if(within(y, 2, height - 2))
	{
		return in.correctPeak(ix, iy, 0.75);
	}
	
	Vec2 pos(x, y);
		
	// Use centered differences to find derivatives.
	float dx = (in(x + 1, y) - in(x - 1, y)) / 2.f;
	float dy = (in(x, y + 1) - in(x, y - 1)) / 2.f;
	float dxx = (in(x + 1, y) + in(x - 1, y) - 2*in(x, y));
	float dyy = (in(x, y + 1) + in(x, y - 1) - 2*in(x, y));
	float dxy = (in(x + 1, y + 1) + in(x - 1, y - 1) - in(x + 1, y - 1) - in(x - 1, y + 1)) / 4.f;

	if((dxx != 0.f)&&(dxx != 0.f)&&(dxx != 0.f))
	{
		float oneOverDiscriminant = 1.f/(dxx*dyy - dxy*dxy);
		float fx = (dyy*dx - dxy*dy) * oneOverDiscriminant;
		float fy = (dxx*dy - dxy*dx) * oneOverDiscriminant;
		
		fx = clamp(fx, -0.5f, 0.5f);
		fy = clamp(fy, -0.5f, 0.5f);
		
		if(y == 1)
		{
			if(fy > 0.)
				fy *= 2.f;
		}
		else if(y == height - 2)
		{
			if(fy < 0.)
				fy *= 2.f;
		}
		
		pos -= Vec2(fx, fy);
	}
	return pos;
}

// with whole input minus background:
// for all touches t from most pressure to least:
//   use taylor correct to get new position
//   if other touches are close, snap position towards nearest unoccupied key center
//   subtract synthetic kernel for t from R

// expire or move existing touches based on new signal input. 
// 
void TouchTracker::updateTouches(const MLSignal& in)
{
	const float e = 2.718281828;

	// copy input signal to border land
	int width = in.getWidth();
	int height = in.getHeight();
	
	mTemp.copy(in);
	mTemplateMask.clear();
	
	// sort active touches by Z
	// copy into sorting container, referring back to unsorted touches
	int activeTouches = 0;
	for(int i = 0; i < mMaxTouchesPerFrame; ++i)
	{
		Touch& t = mTouches[i];
		if (t.isActive())
		{
			t.unsortedIdx = i;
			mTouchesToSort[activeTouches++] = t;
		}
	}
	std::sort(mTouchesToSort.begin(), mTouchesToSort.begin() + activeTouches, compareTouchZ());

	// update active touches in sorted order, referring to existing touches in place
	for(int i = 0; i < activeTouches; ++i)
	{
		int refIdx = mTouchesToSort[i].unsortedIdx;
		Touch& t = mTouches[refIdx];
		Vec2 pos(t.x, t.y); 
		Vec2 newPos = pos;
		float newX = t.x;
		float newY = t.y;
		float newZ = in.getInterpolatedLinear(pos);
		
		// if not preparing to remove, update position.
		if (t.releaseCtr == 0)
		{
			Vec2 minPos(0, 0);
			Vec2 maxPos(width, height);
			int ix = floor(pos.x() + 0.5f);
			int iy = floor(pos.y() + 0.5f);		
			
			// move to any higher neighboring integer value
			Vec2 newPeak;
			newPeak = adjustPeak(mTemp, ix, iy);			
			Vec2 newPeakI, newPeakF;
			newPeak.getIntAndFracParts(newPeakI, newPeakF);
			int newPx = newPeakI.x();
			int newPy = newPeakI.y();
			
			// get exact location and possibly new key. 
			Vec2 correctPos = correctTouchPosition(mTemp, newPx, newPy); 
			
			newPos = correctPos;	
			int newKey = getKeyIndexAtPoint(newPos);												
			
			// move the touch.  
			if((newKey == t.key) || !keyIsOccupied(newKey))
			{			
				// This must be the only place a touch can move from key to key.
				pos = newPos;
				newX = pos.x();
				newY = pos.y();					
				t.key = newKey;
			}							
		}
		
		// look for reasons to release
		newZ = in.getInterpolatedLinear(newPos);
		bool thresholdTest = (newZ > mOffThreshold);			
		float inhibit = getInhibitThreshold(pos);
		bool inhibitTest = (newZ > inhibit);
		t.tDist = mCalibrator.differenceFromTemplateTouchWithMask(mTemp, pos, mTemplateMask);
		bool templateTest = (t.tDist < mTemplateThresh);
		bool overrideTest = (newZ > mOverrideThresh);
						
		t.age++;

		// handle release		
		// TODO get releaseDetect: evidence that touch has been released.
		// from matching release curve over ~ 50 samples. 
		if (!thresholdTest || (!templateTest && !overrideTest) || (!inhibitTest))
		{			
			/*
			if(!thresholdTest && (t.releaseCtr == 0))
			{
				debug() << refIdx << " REL thresholdFail: " << newZ << " at " << pos << "\n";	
			}
			if(!inhibitTest && (t.releaseCtr == 0))
			{
				debug() << refIdx << " REL inhibitFail: " << newZ << " < " << inhibit << "\n";	
			}
			if(!templateTest && (t.releaseCtr == 0))
			{
				debug() << refIdx << " REL templateFail: " << t.tDist << " at " << pos << "\n";	
			}			
			 */
			
			if(t.releaseCtr == 0)
			{
				t.releaseSlope = t.z / (float)kTouchReleaseFrames;
			}
			t.releaseCtr++;
			newZ = t.z - t.releaseSlope;
		}
		else
		{	
			// reset off counter 
			t.releaseCtr = 0;	
		}

		{
			// filter position so that light touches do not wander in pitch much due to noise. 			
			float xyCutoff = newZ*newZ*200000.f;
			xyCutoff = clamp(xyCutoff, 10.f, 100.f);
			
			float x = powf(e, -kMLTwoPi * xyCutoff / (float)mSampleRate);
			float a0 = 1.f - x;
			float b1 = -x;
			t.dz = newZ - t.z;	
			
			t.x1 = t.x;
			t.y1 = t.y;
			t.z1 = t.z;

			// these can't be filtered too much or updateTouches will not work
			// for fast movements.  Revisit when we rewrite the touch tracker.					
			t.x = a0*newX - b1*t.x;
			t.y = a0*newY - b1*t.y;
			t.z = newZ;
		}
		
		// filter z based on user lowpass setting and touch age
		float lp = mLopass;
		lp -= t.age*(mLopass*0.75f/kAttackFrames);
		lp = clamp(lp, mLopass, mLopass*0.25f);	
				
		float xz = powf(e, -kMLTwoPi * lp / (float)mSampleRate);
		float a0z = 1.f - xz;
		float b1z = -xz;
		t.zf = a0z*(newZ - mOnThreshold) - b1z*t.zf;	
				
		// remove touch if filtered z is below threshold
		if(t.zf < 0.)
		{
			removeTouchAtIndex(refIdx);
		}
		
		// subtract updated touch from the input sum.
		// mTemplateScaled is scratch space. 
		mTemplateScaled.clear();
		mTemplateScaled.add2D(mCalibrator.getTemplate(pos), 0, 0);
		mTemplateScaled.scale(-t.z*mCalibrator.getZAdjust(pos)); 
														
		mTemp.add2D(mTemplateScaled, Vec2(pos - Vec2(kTemplateRadius, kTemplateRadius)));
		mTemp.sigMax(0.0);

		// add touch neighborhood to template mask. This allows crowded touches
		// to pass the template test by ignoring areas shared with other touches.
		Vec2 maskPos(t.x, t.y);
		const MLSignal& tmplate = mCalibrator.getTemplate(maskPos);
		mTemplateMask.add2D(tmplate, maskPos - Vec2(kTemplateRadius, kTemplateRadius));
	}
}

Vec3 TouchTracker::closestTouch(Vec2 pos)
{
	float minDist = MAXFLOAT;
	int minIdx = 0;
	for(int i = 0; i < mMaxTouchesPerFrame; ++i)
	{
		Touch& t = mTouches[i];
		if (t.isActive())
		{
			Vec2 p(t.x, t.y);
			p -= pos;
			float d = p.magnitude();
			if(d < minDist)
			{
				minDist = d;
				minIdx = i;
			}
		}
	}
	Touch& t = mTouches[minIdx];
	return(Vec3(t.x, t.y, t.z));
}

const float kInhibitRange = 8;
// currently, touches inhibit other new touches near them, below
// a conical slope away from the touch position. 
// return the greatest inhibit threshold of any other touch at the given position.		
float TouchTracker::getInhibitThreshold(Vec2 a)
{
	float maxInhibit = 0.f;
	for(int i = 0; i < mMaxTouchesPerFrame; ++i)
	{
		const Touch& u = mTouches[i];
		if (u.isActive())
		{
			Vec2 b(u.x1, u.y1);		// use previous position		
			float dab = (a - b).magnitude();			
			// don't check against touch at input position (same touch)
			if(dab > 0.1f)
			{
				if(dab < kInhibitRange)
				{					
					float dru = 1.f - (dab / kInhibitRange);	
					dru = clamp(dru, 0.f, 1.f);
					float inhibitZ = u.z1*0.9*dru;
					maxInhibit = max(maxInhibit, inhibitZ);
				}
			}
		}
	}
	return maxInhibit;
}

void TouchTracker::addPeakToKeyState(const MLSignal& in)
{
	mTemp.copy(in);
	for(int i=0; i<kMaxPeaksPerFrame; ++i)
	{
		// get highest peak from temp.  
		Vec3 peak = mTemp.findPeak();	
		float z = peak.z();
		
		// add peak to key state, or bail
		if (z > mOnThreshold)
		{			
			Vec2 pos = in.correctPeak(peak.x(), peak.y(), 1.0f);	
			int key = getKeyIndexAtPoint(pos);
			if(within(key, 0, mNumKeys))
			{
				// send peak energy to key under peak.
				KeyState& keyState = mKeyStates[key];
				MLRange kdzRange(mOffThreshold, mOnThreshold*2., 0.001f, 1.f);
				float iirCoeff = kdzRange.convertAndClip(z);	
				float dt = mCalibrator.differenceFromTemplateTouch(in, pos);
                
				keyState.mK = iirCoeff;
				keyState.zIn = z;
				keyState.dtIn = dt;
				if(mQuantizeToKey)
				{
					keyState.posIn = keyState.mKeyCenter;
				}
				else
				{
					keyState.posIn = pos;
				}
			}
		}
		else
		{
			break;
		}
	}
}							

// this is where all new touches are born.
void TouchTracker::findTouches()
{
	for(int i=0; i<mNumKeys; ++i)
	{
		float z = mKeyStates[i].zOut;	
		bool threshTest = (z > mOnThreshold);
		if(threshTest)
		{			
			float kdt = mKeyStates[i].dtOut;
			float kdz = mKeyStates[i].dzOut * 50.f;
			kdz = clamp(kdz, 0.f, 1.f);
			kdz = sqrtf(kdz);
			float kCoeff = mKeyStates[i].mK;
			Vec2 pos = mKeyStates[i].posOut;
			float inhibit = getInhibitThreshold(pos);			
			bool inhibitTest = (z > inhibit);
			bool templateTest = (kdt < mTemplateThresh);
			bool overrideTest = (z > mOverrideThresh);
			bool kCoeffTest = (kCoeff > 0.001f);
			bool ageTest = mKeyStates[i].age > 10;
			bool dzTest = kdz > 0.001f;
            
			if (ageTest && inhibitTest && kCoeffTest && dzTest)
			{	
				// if difference of peak neighborhood from template at subpixel position 
				// is under threshold, we may have a new touch.		
				if (templateTest || overrideTest) 
				{
					if(!keyIsOccupied(i))
					{
                        //	Touch t(pos.x(), pos.y(), z, kCoeff);
						Touch t(pos.x(), pos.y(), z, kdz);
						t.key = i;
						t.tDist = kdt;
						int newIdx = addTouch(t); 	
						if(newIdx >= 0)
						{		
							mKeyStates[i].age = 0;										
																			
//debug() << newIdx << " ON z:" << z << " kdt:" << kdt << " at " << pos << "\n";	
//debug() << "          template: " << templateTest << " overrideTest: " << overrideTest << "\n";
//			debug() << "********\n";
//			debug() << "KC " << kCoeff << "\n";
//			debug() << "KDZ " << kdz << "\n";
//			debug() << "********\n";

			
			// set age for key state to inhibit retrigger
			
							
						}
					}

				}
			}
		}
	}
}

void TouchTracker::KeyState::tick()
{	
	// filter
	dzIn = (zIn - zOut);
	zOut += dzIn*mK;
	dtOut += (dtIn - dtOut)*mK;
	posOut += (posIn - posOut)*mK;
	
	// collect dz with constant IIR filter
	dzOut += (dzIn - dzOut)*mK;
	
	age++;

	// set inputs back to defaults 
	zIn = 0.f;
	dzIn = 0.f;
	dtIn = 1.0f;
	posIn = mKeyCenter; 
}

void TouchTracker::process(int)
{	
	if (!mpIn) return;
	const MLSignal& in(*mpIn);
	
	mFilteredInput.copy(in);
	
	// clear edges (should do earlier! TODO)
	int w = in.getWidth();
	int h = in.getHeight();
	for(int j=0; j<h; ++j)
	{
		mFilteredInput(0, j) = 0;
		mFilteredInput(w - 1, j) = 0;
	}
	
	if (mNeedsClear)
	{
		mBackground.copy(mFilteredInput);
		mBackgroundFilter.clear();
		mNeedsClear = false;
		return;
	}
		
	// filter out any negative values, this shows up from capacitive coupling near front edge
	mFilteredInput.sigMax(0.);		
	
	if (mCalibrator.isCalibrating())
	{		
		int done = mCalibrator.addSample(mFilteredInput);
		
		if(done == 1)
		{
			// Tell the listener we have a new calibration. We still do the calibration here in the Tracker, 
			// but the Model will be responsible for saving and restoring the calibration maps.
			if(mpListener)
			{
				mpListener->hasNewCalibration(mCalibrator.mCalibrateSignal, mCalibrator.mNormalizeMap, mCalibrator.mAvgDistance);
			}
		}
	}
	else
	{
		if(mDoNormalize)
		{
			mCalibrator.normalizeInput(mFilteredInput);
		}
		
		if(mMaxTouchesPerFrame > 0)
		{
			// smooth input	
			float kc, ke, kk;
			kc = 4.f/16.f; ke = 2.f/16.f; kk=1.f/16.f;
			mFilteredInput.convolve3x3r(kc, ke, kk);

			// build sum of currently tracked touches	
			//
			mSumOfTouches.clear();
			int numActiveTouches = 0;
			for(int i = 0; i < mMaxTouchesPerFrame; ++i)
			{
				const Touch& t(mTouches[i]);
				if(t.isActive())
				{
					Vec2 touchPos(t.x, t.y);
					mTemplateScaled.clear();
					mTemplateScaled.add2D(mCalibrator.getTemplate(touchPos), 0, 0);
					mTemplateScaled.scale(t.z*mCalibrator.getZAdjust(touchPos));
					mSumOfTouches.add2D(mTemplateScaled, touchPos - Vec2(kTemplateRadius, kTemplateRadius));
					numActiveTouches++;
				}
			}	
			
			// to make sum of touches a bit bigger 
			mSumOfTouches.scale(2.0f);
			mSumOfTouches.convolve3x3r(kc, ke, kk);
			mSumOfTouches.convolve3x3r(kc, ke, kk);
			mSumOfTouches.convolve3x3r(kc, ke, kk);

			// TODO lots of optimization here in onepole, 2D filter
			//
			// TODO the mean of lowpass background can be its own control source that will 
			// act like an accelerometer!  tilt controls even. 
			
			mBackgroundFilterFrequency.fill(mBackgroundFilterFreq);
			
			// build background: lowpass filter rest state.  Filter freq.
			// is nonzero where there are no touches, 0 where there are touches.
			mTemp.copy(mSumOfTouches);
			mTemp.scale(100.f); 
			mBackgroundFilterFrequency.subtract(mTemp);
			mBackgroundFilterFrequency.sigMax(0.);		
			
			// TODO allow filter to move a little if touch template distance is near threshold
			// this will fix most stuck touches

			// filter background in up direction 
			mBackgroundFilterFrequency2.fill(mBackgroundFilterFreq);
			mBackgroundFilter.setInputSignal(&mFilteredInput);
			mBackgroundFilter.setOutputSignal(&mBackground);

			// set asymmetric filter coeffs and get background
			mBackgroundFilter.setCoeffs(mBackgroundFilterFrequency, mBackgroundFilterFrequency2);
			mBackgroundFilter.process(1);	
 		}
		
		// subtract background from input 
		//
		mInputMinusBackground.copy(mFilteredInput);
		mInputMinusBackground.subtract(mBackground);
		
		// move or remove and filter existing touches
		//
		updateTouches(mInputMinusBackground);	
		
		// TODO can negative values be used to inhibit nearby touches?  
		// this might prevent sticking touches when a lot of force is
		// applied then quickly released.
		
		// TODO look for retriggers here, touches not fallen to 0 but where 
		// dz warrants a new note-on. The way to do this is keep a second,
		// separate set of key states that do not get cleared by current
		// touches.  These can be used to get the dz values and using the exact
		// same math, velocities will match other note-ons.
        //
        // we can also look for a nearby release just beforehand when
        // retriggering. this will increase confidence in a retrigger as
        // opposed to simply moving the touch.

		// after update Touches, subtract sum of touches to get residual R
		// R = input - T.
		// This represents any pressure data not currently part of a touch.
		if(mMaxTouchesPerFrame > 0)
		{
			mInputMinusBackground.sigMax(0.);	
			mResidual.copy(mInputMinusBackground);
			mResidual.subtract(mSumOfTouches);
			mResidual.sigMax(0.);
		}

		// get signals for viewer
		// TODO optimize: we only have to copy these each time a view is needed
		mCalibratedSignal.copy(mInputMinusBackground);
		mCookedSignal.copy(mSumOfTouches);		
		mTestSignal.copy(mResidual);		
		
		// get subpixel xyz peak from residual
		addPeakToKeyState(mResidual);
		
		// update key states 
		for(int i=0; i<mNumKeys; ++i)
		{
			mKeyStates[i].tick();
		}
		
		findTouches();

		// filter touches
		// filter x and y for output
		// filter touches and write touch data to one frame of output signal.
		//
		MLSignal& out = *mpOut;
		for(int i = 0; i < mMaxTouchesPerFrame; ++i)
		{
			Touch& t = mTouches[i];			
			if(t.age > 1)
			{
				float xyc = 1.0f - powf(2.71828f, -kMLTwoPi * mLopass*0.1f / (float)mSampleRate);
				t.xf += (t.x - t.xf)*xyc;
				t.yf += (t.y - t.yf)*xyc;
			}
			else if(t.age == 1)
			{
				t.xf = t.x;
				t.yf = t.y;
			}
			out(xColumn, i) = t.xf;
			out(yColumn, i) = t.yf;
			out(zColumn, i) = (t.age > 0) ? t.zf : 0.;			
			out(dzColumn, i) = t.dz;
			out(ageColumn, i) = t.age;
			out(dtColumn, i) = t.tDist;
		}
	}
	
#if DEBUG	
	// TEMP	
	if (mCount++ > 1000) 
	{
		mCount = 0;
		//dumpTouches();
	}
#endif	
}

void TouchTracker::setDefaultNormalizeMap()
{
	mCalibrator.setDefaultNormalizeMap();
	mpListener->hasNewCalibration(mNullSig, mNullSig, -1.f);
}

// --------------------------------------------------------------------------------
#pragma mark calibration


TouchTracker::Calibrator::Calibrator(int w, int h) :
	mActive(false),
	mHasCalibration(false),
	mHasNormalizeMap(false),
	mCollectingNormalizeMap(false),
	mSrcWidth(w),
	mSrcHeight(h),
	mWidth(w),
	mHeight(h),
	mAutoThresh(0.05f)
{
	// resize sums vector and signals in it
	mData.resize(mWidth*mHeight);//*kCalibrateResolution*kCalibrateResolution);
	mDataSum.resize(mWidth*mHeight);//*kCalibrateResolution*kCalibrateResolution);
	mSampleCount.resize(mWidth*mHeight);
	mPassesCount.resize(mWidth*mHeight);
	for(int i=0; i<mWidth*mHeight; ++i)
	{
		mData[i].setDims(kTemplateSize, kTemplateSize);
		mData[i].clear();
		mDataSum[i].setDims(kTemplateSize, kTemplateSize);
		mDataSum[i].clear();
		mSampleCount[i] = 0;
		mPassesCount[i] = 0;
	}
	mIncomingSample.setDims(kTemplateSize, kTemplateSize);
	mVisSignal.setDims(mWidth, mHeight);
	mNormalizeMap.setDims(mSrcWidth, mSrcHeight);
	
	mNormalizeCount.setDims(mSrcWidth, mSrcHeight);
	mFilteredInput.setDims(mSrcWidth, mSrcHeight);
	mTemp.setDims(mSrcWidth, mSrcHeight);
	mTemp2.setDims(mSrcWidth, mSrcHeight);

	makeDefaultTemplate();
}

TouchTracker::Calibrator::~Calibrator()
{
}

void TouchTracker::Calibrator::begin()
{
	MLConsole() << "\n****************************************************************\n\n";
	MLConsole() << "Hello and welcome to tracker calibration. \n";
	MLConsole() << "Collecting silence, please don't touch.";
	
	mFilteredInput.clear();
	mSampleCount.clear();
	mPassesCount.clear();
	mVisSignal.clear();
	mNormalizeMap.clear();
	mNormalizeCount.clear();
	mTotalSamples = 0;
	mStartupSum = 0.;
	for(int i=0; i<mWidth*mHeight; ++i)
	{
		float maxSample = 1.f;
		mData[i].fill(maxSample);
		mDataSum[i].clear(); 
		mSampleCount[i] = 0;
		mPassesCount[i] = 0;
	}
	mPeak = Vec2();
	age = 0;
	mActive = true;
	mHasCalibration = false;
	mHasNormalizeMap = false;
	mCollectingNormalizeMap = false;
}

void TouchTracker::Calibrator::cancel()
{
	if(isCalibrating())
	{
		mActive = false;
		MLConsole() << "\nCalibration cancelled.\n";
	}
}

void TouchTracker::Calibrator::setDefaultNormalizeMap()
{
	mActive = false;
	mHasCalibration = false;
	mHasNormalizeMap = false;
}

void TouchTracker::Calibrator::makeDefaultTemplate()
{
	mDefaultTemplate.setDims (kTemplateSize, kTemplateSize);
		
	int w = mDefaultTemplate.getWidth();
	int h = mDefaultTemplate.getHeight();
	Vec2 vcenter(h/2.f, w/2.f);
	
	// default scale -- not important because we want to calibrate.
	Vec2 vscale(3.5, 3.);	 
	
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			Vec2 vdistance = Vec2(i + 0.5f, j + 0.5f) - vcenter;
			vdistance /= vscale;
			float d = clamp(vdistance.magnitude(), 0.f, 1.f);
			mDefaultTemplate(i, j) = 1.0*(1.0f - d);
		}
	}
}

// get the template touch at the point p by bilinear interpolation
// from the four surrounding templates.
const MLSignal& TouchTracker::Calibrator::getTemplate(Vec2 p) const
{
	static MLSignal temp1(kTemplateSize, kTemplateSize);
	static MLSignal temp2(kTemplateSize, kTemplateSize);
	static MLSignal d00(kTemplateSize, kTemplateSize);
	static MLSignal d10(kTemplateSize, kTemplateSize);
	static MLSignal d01(kTemplateSize, kTemplateSize);
	static MLSignal d11(kTemplateSize, kTemplateSize);
	if(mHasCalibration)
	{
		Vec2 pos = getBinPosition(p);
		Vec2 iPos, fPos;
		pos.getIntAndFracParts(iPos, fPos);	
		int idx00 = iPos.y()*mWidth + iPos.x();


		d00.copy(mCalibrateSignal.getFrame(idx00));
        if(iPos.x() < mWidth - 3)
        {
            d10.copy(mCalibrateSignal.getFrame(idx00 + 1));
        }
        else
        {
            d10.copy(mDefaultTemplate);
        }
        
        if(iPos.y() < mHeight - 1)
        {
            d01.copy(mCalibrateSignal.getFrame(idx00 + mWidth));
        }
        else
        {
            d01.copy(mDefaultTemplate);
        }
        
        if ((iPos.x() < mWidth - 3) && (iPos.y() < mHeight - 1))
        {
            d11.copy(mCalibrateSignal.getFrame(idx00 + mWidth + 1));
        }
        else
        {
            d11.copy(mDefaultTemplate);
        }
		temp1.copy(d00);
        
        temp1.sigLerp(d10, fPos.x());
		temp2.copy(d01);
		temp2.sigLerp(d11, fPos.x());
		temp1.sigLerp(temp2, fPos.y());

		return temp1;
	}
	else
	{
		return mDefaultTemplate;
	}
}

Vec2 TouchTracker::Calibrator::getBinPosition(Vec2 pIn) const
{
	// Soundplane A
	static MLRange binRangeX(2.0, 61.0, 0., mWidth);
	static MLRange binRangeY(0.5, 6.5, 0., mHeight);
	Vec2 minPos(2.5, 0.5);
	Vec2 maxPos(mWidth - 2.5f, mHeight - 0.5f);
	Vec2 pos(binRangeX(pIn.x()), binRangeY(pIn.y()));
	return vclamp(pos, minPos, maxPos);
}

void TouchTracker::Calibrator::normalizeInput(MLSignal& in)
{
	if(mHasNormalizeMap)
	{
		in.multiply(mNormalizeMap);
	}
}

// return true if the current cell (i, j) is used by the current stage of calibration
bool TouchTracker::Calibrator::isWithinCalibrateArea(int i, int j)
{
	if(mCollectingNormalizeMap)
	{
		return(within(i, 1, mWidth - 1) && within(j, 0, mHeight));
	}
	else
	{
		return(within(i, 2, mWidth - 2) && within(j, 0, mHeight));
	}
}

float TouchTracker::Calibrator::makeNormalizeMap()
{			
	int samples = 0;
	float sum = 0.f;
	for(int j=0; j<mSrcHeight; ++j)
	{
		for(int i=0; i<mSrcWidth; ++i)
		{			
			if(isWithinCalibrateArea(i, j))
			{
				float sampleSum = mNormalizeMap(i, j);
				float sampleCount = mNormalizeCount(i, j);
				float sampleAvg = sampleSum / sampleCount;
				mNormalizeMap(i, j) = 1.f / sampleAvg;
				sum += sampleAvg;
				samples++;
			}
			else
			{
				mNormalizeMap(i, j) = 0.f;
			}
		}
	}
    
	float mean = sum/(float)samples;
	mNormalizeMap.scale(mean);	

	// constrain output values
	mNormalizeMap.sigMin(3.f);
	mNormalizeMap.sigMax(0.125f);	
	
	// return maximum
	Vec3 vmax = mNormalizeMap.findPeak();
	float rmax = vmax.z();
	
	mHasNormalizeMap = true;
	return rmax;
}

void TouchTracker::Calibrator::getAverageTemplateDistance()
{
	MLSignal temp(mWidth, mHeight);
	MLSignal tempSample(kTemplateSize, kTemplateSize);
	float sum = 0.f;
	int samples = 0;
	for(int j=0; j<mHeight; ++j)
	{
		for(int i=0; i<mWidth; ++i)
		{						
			int idx = j*mWidth + i;
			
			// put mean of input samples into temp signal at i, j
			temp.clear();
			tempSample.copy(mDataSum[idx]);
			tempSample.scale(1.f / (float)mSampleCount[idx]);
			temp.add2D(tempSample, i - kTemplateRadius, j - kTemplateRadius);
			
			float diff = differenceFromTemplateTouch(temp, Vec2(i, j));
			sum += diff;
			samples++;
			// debug() << "template diff [" << i << ", " << j << "] : " << diff << "\n";	
		}
	}
	mAvgDistance = sum / (float)samples;
}

// input: the pressure data, after static calibration (tare) but otherwise raw.
// input feeds a state machine that first collects a normalization map, then
// collects a touch shape, or kernel, at each point. 
int TouchTracker::Calibrator::addSample(const MLSignal& m)
{
	int r = 0;
	static Vec2 intPeak1;
	
	static MLSignal f2(mSrcWidth, mSrcHeight);
	static MLSignal input(mSrcWidth, mSrcHeight);
	static MLSignal tare(mSrcWidth, mSrcHeight);
	static MLSignal normTemp(mSrcWidth, mSrcHeight);
    
    // decreasing this will collect a wider area during normalization,
    // smoothing the results.
    const float kNormalizeThreshold = 0.125f;
	
	float kc, ke, kk;
	kc = 4./16.; ke = 2./16.; kk=1./16.;
	
	// simple lopass time filter for calibration
	f2 = m;
	f2.subtract(mFilteredInput);
	f2.scale(0.1f);
	mFilteredInput.add(f2);
	input = mFilteredInput;
	input.sigMax(0.);		
		
	// get peak of sample data
	Vec3 testPeak = input.findPeak();	
	float peakZ = testPeak.z();
	
	const int startupSamples = 1000;
	const int waitAfterNormalize = 2000;
	if (mTotalSamples < startupSamples)
	{
		age = 0;
		mStartupSum += peakZ;
		if(mTotalSamples % 100 == 0)
		{
			MLConsole() << ".";
		}
	}
	else if (mTotalSamples == startupSamples)
	{
		age = 0;
		//mAutoThresh = kNormalizeThresh;
		mAutoThresh = mStartupSum / (float)startupSamples * 10.f;	
		MLConsole() << "\n****************************************************************\n\n";
		MLConsole() << "OK, done collecting silence (auto threshold: " << mAutoThresh << "). \n";
		MLConsole() << "Now please slide your palm across the surface,  \n";
		MLConsole() << "applying a firm and even pressure, until all the rectangles \n";
		MLConsole() << "at left turn blue.  \n\n";
		
		mNormalizeMap.clear();
		mNormalizeCount.clear();
		mCollectingNormalizeMap = true;
	}		
	else if (mCollectingNormalizeMap)
	{
		// smooth temp signal, duplicating values at border
		normTemp.copy(input);		
		normTemp.convolve3x3rb(kc, ke, kk);
		normTemp.convolve3x3rb(kc, ke, kk);
		normTemp.convolve3x3rb(kc, ke, kk);
	
		if(peakZ > mAutoThresh)
		{
			// collect additions in temp signals
			mTemp.clear(); // adds to map
			mTemp2.clear(); // adds to sample count
			
			// where input > thresh and input is near max current input, add data. 		
			for(int j=0; j<mHeight; ++j)
			{
				for(int i=0; i<mWidth; ++i)
				{
					// test threshold with smoothed data
					float zSmooth = normTemp(i, j);
					// but add actual samples from unsmoothed input
					float z = input(i, j);
					if(zSmooth > peakZ * kNormalizeThreshold)
					{
						// map must = count * z/peakZ
						mTemp(i, j) = z / peakZ;
						mTemp2(i, j) = 1.0f;	
					}
				}
			}
			
			// add temp signals to data						
			mNormalizeMap.add(mTemp);
			mNormalizeCount.add(mTemp2);
			mVisSignal.copy(mNormalizeCount);
			mVisSignal.scale(1.f / (float)kNormMapSamples);
		}
		
		if(doneCollectingNormalizeMap())
		{				
			float mapMaximum = makeNormalizeMap();	
						
			MLConsole() << "\n****************************************************************\n\n";
			MLConsole() << "\n\nOK, done collecting normalize map. (max = " << mapMaximum << ").\n";
			MLConsole() << "Please lift your hands.";
			mCollectingNormalizeMap = false;
			mWaitSamplesAfterNormalize = 0;
			mVisSignal.clear();
			mStartupSum = 0;
		}
	}
	else
	{
		if(mWaitSamplesAfterNormalize < waitAfterNormalize)
		{
			mStartupSum += peakZ;
			mWaitSamplesAfterNormalize++;
			if(mTotalSamples % 100 == 0)
			{
				MLConsole() << ".";
			}
		}
		else if(mWaitSamplesAfterNormalize == waitAfterNormalize)
		{
			mWaitSamplesAfterNormalize++;
			mAutoThresh *= 1.5f;
			MLConsole() << "\nOK, done collecting silence again (auto threshold: " << mAutoThresh << "). \n";

			MLConsole() << "\n****************************************************************\n\n";
			MLConsole() << "Now please slide a single finger over the  \n";
			MLConsole() << "Soundplane surface, visiting each area twice \n";
			MLConsole() << "until all the areas are colored green at left.  \n";
			MLConsole() << "Sliding over a key the first time will turn it gray.  \n";
			MLConsole() << "Sliding over a key the second time will turn it green.\n";
			MLConsole() << "\n";
		}
		else if(peakZ > mAutoThresh)
		{
			// normalize input
			mTemp.copy(input);
			mTemp.multiply(mNormalizeMap);
		
			// smooth input	
			mTemp.convolve3x3r(kc, ke, kk);
			mTemp.convolve3x3r(kc, ke, kk);
			mTemp.convolve3x3r(kc, ke, kk);
			
			// get corrected peak
			mPeak = mTemp.findPeak();
			mPeak = mTemp.correctPeak(mPeak.x(), mPeak.y(), 1.0f);							
			Vec2 minPos(2.0, 0.);
			Vec2 maxPos(mWidth - 2., mHeight - 1.);
			mPeak = vclamp(mPeak, minPos, maxPos);
		
			age++; // continue touch
			// get sample from input around peak and normalize
			mIncomingSample.clear();
			mIncomingSample.add2D(m, -mPeak + Vec2(kTemplateRadius, kTemplateRadius)); 
			mIncomingSample.sigMax(0.f);
			mIncomingSample.scale(1.f / mIncomingSample(kTemplateRadius, kTemplateRadius));

			// get integer bin	
			Vec2 binPeak = getBinPosition(mPeak);
			mVisPeak = binPeak - Vec2(0.5, 0.5);		
			int bix = binPeak.x();			
			int biy = binPeak.y();
			// clamp to calibratable area
			bix = clamp(bix, 2, mWidth - 2);
			biy = clamp(biy, 0, mHeight - 1);
			Vec2 bIntPeak(bix, biy);

			// count sum and minimum of all kernel samples for the bin
			int dataIdx = biy*mWidth + bix;
			mDataSum[dataIdx].add(mIncomingSample); 
			mData[dataIdx].sigMin(mIncomingSample); 
			mSampleCount[dataIdx]++;
            
			if(bIntPeak != intPeak1)
			{
				// entering new bin.
				intPeak1 = bIntPeak;
				if(mPassesCount[dataIdx] < kPassesToCalibrate)
				{
					mPassesCount[dataIdx]++;
					mVisSignal(bix, biy) = (float)mPassesCount[dataIdx] / (float)kPassesToCalibrate;
				}
			}			
			
			// check for done
			if(isDone())
			{
				mCalibrateSignal.setDims(kTemplateSize, kTemplateSize, mWidth*mHeight);
				
				// get result for each junction
				for(int j=0; j<mHeight; ++j)
				{
					for(int i=0; i<mWidth; ++i)
					{						
						int idx = j*mWidth + i;
						// copy to 3d signal
						mCalibrateSignal.setFrame(idx, mData[idx]);
					}
				}
				
				getAverageTemplateDistance();
				mHasCalibration = true;
				mActive = false;	
				r = 1;	
							
				MLConsole() << "\n****************************************************************\n\n";
				MLConsole() << "Calibration is now complete and will be auto-saved in the file \n";
				MLConsole() << "SoundplaneAppState.txt. \n";
				MLConsole() << "\n****************************************************************\n\n";
			}	
		}
		else
		{
			age = 0;
			intPeak1 = Vec2(-1, -1);
			mVisPeak = Vec2(-1, -1);
		}
	}
	mTotalSamples++;
	return r;
}

bool TouchTracker::Calibrator::isCalibrating()
{
	return mActive;
}

bool TouchTracker::Calibrator::hasCalibration()
{
	return mHasCalibration;
}

bool TouchTracker::Calibrator::isDone()
{
	// If we have enough samples at each location, we are done.
	bool calDone = true;
	for(int j=0; j<mHeight; ++j)
	{
		for(int i=0; i<mWidth; ++i)
		{
			if(isWithinCalibrateArea(i, j))
			{
				int dataIdx = j*mWidth + i;
				if (mPassesCount[dataIdx] < kPassesToCalibrate)
				{
					calDone = false;
					goto done;
				}
			}
		}
	}
done:	
	return calDone;
}

bool TouchTracker::Calibrator::doneCollectingNormalizeMap()
{
	// If we have enough samples at each location, we are done.
	bool calDone = true;
	for(int j=0; j<mHeight; ++j)
	{
		for(int i=0; i<mWidth; ++i)
		{
			if(isWithinCalibrateArea(i, j))
			{
				if (mNormalizeCount(i, j) < kNormMapSamples)
				{
					calDone = false;
					goto done;
				}
			}
		}
	}
done:	
	return calDone;
}
	
void TouchTracker::Calibrator::setCalibration(const MLSignal& v)
{
	if((v.getHeight() == kTemplateSize) && (v.getWidth() == kTemplateSize))
	{
        mCalibrateSignal = v;
        mHasCalibration = true;
    }
    else
    {
		MLConsole() << "TouchTracker::Calibrator::setCalibration: bad size, restoring default.\n";
        mHasCalibration = false;
    }
}

void TouchTracker::Calibrator::setNormalizeMap(const MLSignal& v)
{
	if((v.getHeight() == mSrcHeight) && (v.getWidth() == mSrcWidth))
	{
		mNormalizeMap = v;
		mHasNormalizeMap = true;
	}
	else
	{
		MLConsole() << "TouchTracker::Calibrator::setNormalizeMap: restoring default.\n";
		mNormalizeMap.fill(1.f);
		mHasNormalizeMap = false;
	}
}


float TouchTracker::Calibrator::getZAdjust(const Vec2 p)
{
	// first adjust z for interpolation based on xy position within unit square
	//
	Vec2 vInt, vFrac;
	p.getIntAndFracParts(vInt, vFrac);
	
	Vec2 vd = (vFrac - Vec2(0.5f, 0.5f));
	return 1.414f - vd.magnitude()*0.5f;
}

float TouchTracker::Calibrator::differenceFromTemplateTouch(const MLSignal& in, Vec2 pos)
{
	static MLSignal a2(kTemplateSize, kTemplateSize);
	static MLSignal b(kTemplateSize, kTemplateSize);
	static MLSignal b2(kTemplateSize, kTemplateSize);

	float r = 1.0;
	int height = in.getHeight();
	int width = in.getWidth();
	MLRect boundsRect(0, 0, width, height);
	
	// use linear interpolated z value from input
	float linearZ = in.getInterpolatedLinear(pos)*getZAdjust(pos);
	linearZ = clamp(linearZ, 0.00001f, 1.f);
	float z1 = 1./linearZ;	
	const MLSignal& a = getTemplate(pos);
	
	// get normalized input values surrounding touch
	int tr = kTemplateRadius;
	b.clear();
	for(int j=0; j < kTemplateSize; ++j)
	{
		for(int i=0; i < kTemplateSize; ++i)
		{
			Vec2 vInPos = pos + Vec2((float)i - tr,(float)j - tr);			
			if (boundsRect.contains(vInPos))
			{
				float inVal = in.getInterpolatedLinear(vInPos);
				inVal *= z1;
				b(i, j) = inVal;
			}
		}
	}
	
	int tests = 0;
	float sum = 0.;

	// add differences in z from template
	a2.copy(a);
	b2.copy(b);
    
	for(int j=0; j < kTemplateSize; ++j)
	{
		for(int i=0; i < kTemplateSize; ++i)
		{
			if(b(i, j) > 0.)
			{
				float d = a2(i, j) - b2(i, j);
				sum += d*d;
				tests++;
			}
		}
	}

	// get RMS difference
	if(tests > 0)
	{
		r = sqrtf(sum / tests);
	}	
	return r;
} 

float TouchTracker::Calibrator::differenceFromTemplateTouchWithMask(const MLSignal& in, Vec2 pos, const MLSignal& mask)
{
	static float maskThresh = 0.001f;
	static MLSignal a2(kTemplateSize, kTemplateSize);
	static MLSignal b(kTemplateSize, kTemplateSize);
	static MLSignal b2(kTemplateSize, kTemplateSize);

	float r = 0.f;
	int height = in.getHeight();
	int width = in.getWidth();
	MLRect boundsRect(0, 0, width, height);
	
	// use linear interpolated z value from input
	float linearZ = in.getInterpolatedLinear(pos)*getZAdjust(pos);
	linearZ = clamp(linearZ, 0.00001f, 1.f);
	float z1 = 1./linearZ;	
	const MLSignal& a = getTemplate(pos);
	
	// get normalized input values surrounding touch
	int tr = kTemplateRadius;
	b.clear();
	for(int j=0; j < kTemplateSize; ++j)
	{
		for(int i=0; i < kTemplateSize; ++i)
		{
			Vec2 vInPos = pos + Vec2((float)i - tr,(float)j - tr);			
			if (boundsRect.contains(vInPos) && (mask.getInterpolatedLinear(vInPos) < maskThresh))
			{
				float inVal = in.getInterpolatedLinear(vInPos);
				inVal *= z1;
				b(i, j) = inVal;
			}
		}
	}
	
	int tests = 0;
	float sum = 0.;

	// add differences in z from template
	a2.copy(a);
	b2.copy(b);
	a2.partialDiffX();
	b2.partialDiffX();
	for(int j=0; j < kTemplateSize; ++j)
	{
		for(int i=0; i < kTemplateSize; ++i)
		{
			if(b(i, j) > 0.)
			{
				float d = a2(i, j) - b2(i, j);
				sum += d*d;
				tests++;
			}
		}
	}

	// get RMS difference
	if(tests > 0)
	{
		r = sqrtf(sum / tests);
	}	
	return r;
}



// --------------------------------------------------------------------------------
#pragma mark utilities

void TouchTracker::dumpTouches()
{
	int c = 0;
	for(int i = 0; i < mMaxTouchesPerFrame; ++i)
	{
		const Touch& t = mTouches[i];
		if(t.isActive())
		{
			debug() << "t" << i << ":" << t << ", key#" << t.key << ", td " << t.tDist << ", dz " << t.dz << "\n";		
			c++;
		}
	}
	if(c > 0)
	{
		debug() << "\n";
	}
}

int TouchTracker::countActiveTouches()
{
	int c = 0;
	for(int i = 0; i < mMaxTouchesPerFrame; ++i)
	{
		const Touch& t = mTouches[i];
		if( t.isActive())
		{
			c++;
		}
	}
	return c;
}


