// SoundplaneModelA.cpp
//
// Contains constants that are used by Soundplane drivers.

#include "SoundplaneModelA.h"

#include <math.h>

const char* kSoundplaneAName = ("Soundplane Model A");

// default carriers.  avoiding 32 (always bad)
// in use these should be overridden by the selected carriers.
//
const unsigned char kDefaultCarriers[kSoundplaneSensorWidth] =
{
	0, 0, 4, 5, 6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27,
	28, 29, 30, 31, 33, 34
};


// --------------------------------------------------------------------------------
#pragma mark unpacking data

// combine two surface payloads to a single buffer of floating point pressure values.
//
void K1_unpack_float2(unsigned char *pSrc0, unsigned char *pSrc1, SoundplaneOutputFrame& dest)
{
	float *pDest = dest.data();
	unsigned short a, b;
	float *pDestRow0, *pDestRow1;

	// evey three bytes of payload provide 24 bits,
	// which is two 12-bit magnitude values packed
	// ml Lh HM
	//
	int c = 0;
	for(int i=0; i<kSoundplaneAPickupsPerBoard; ++i)
	{
		pDestRow0 = pDest + kSoundplaneANumCarriers*2*i;
		pDestRow1 = pDestRow0 + kSoundplaneANumCarriers;
		for (int j = 0; j < kSoundplaneANumCarriers; j += 2)
		{
			a = pSrc0[c+1] & 0x0F;  // 000h
			a <<= 8;        // 0h00
			a |= pSrc0[c];      // 0hml
			pDestRow0[j] = a / 4096.f;

			a = pSrc0[c+2];     // 00HM
			a <<= 4;        // 0HM0
			a |= ((pSrc0[c+1] & 0xF0) >> 4);  // 0HML
			pDestRow0[j + 1] = a / 4096.f;

			// flip surface 2

			b = pSrc1[c+1] & 0x0F;  // 000h
			b <<= 8;        // 0h00
			b |= pSrc1[c];      // 0hml
			pDestRow1[kSoundplaneANumCarriers - 1 - j] = b / 4096.f;

			b = pSrc1[c+2];     // 00HM
			b <<= 4;        // 0HM0
			b |= ((pSrc1[c+1] & 0xF0) >> 4);  // 0HML
			pDestRow1[kSoundplaneANumCarriers - 2 - j] = b / 4096.f;

			c += 3;
		}
	}
}

// set data from edge carriers, unused on Soundplane A, to duplicate
// actual data nearby.
void K1_clear_edges(SoundplaneOutputFrame& dest)
{
	float *pDest = dest.data();
	float *pDestRow;
	for(int i=0; i<kSoundplaneAPickupsPerBoard; ++i)
	{
		pDestRow = pDest + kSoundplaneANumCarriers*2*i;
		const float zl = pDestRow[2];
		pDestRow[1] = zl;
		pDestRow[0] = 0;
		const float zr = pDestRow[kSoundplaneANumCarriers*2 - 3];
		pDestRow[kSoundplaneANumCarriers*2 - 2] = zr;
		pDestRow[kSoundplaneANumCarriers*2 - 1] = 0;
	}
}

float frameDiff(const SoundplaneOutputFrame& p0, const SoundplaneOutputFrame& p1)
{
	float sum = 0.f;
	for(int i = 0; i < p0.size(); i++)
	{
		sum += fabs(p1[i] - p0[i]);
	}
	return sum;
}

void dumpFrame(float* frame)
{
	for(int j=0; j<kSoundplaneHeight; ++j)
	{
		printf("row %d: ", j);
		for(int i=0; i<kSoundplaneWidth; ++i)
		{
			printf("%f ", frame[j*kSoundplaneWidth + i]);
		}
		printf("\n");
	}
}
