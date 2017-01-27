#include <xmmintrin.h>

void MLSignal::log2Approx()
{
	MLSample* px1 = getBuffer();
    
	int c = getSize() >> kMLSamplesPerSSEVectorBits;
	__m128 vx1, vy1;
	
	for (int n = 0; n < c; ++n)
	{
		vx1 = _mm_load_ps(px1);
		vy1 = log2Approx4(vx1); 		
		_mm_store_ps(px1, vy1);
		px1 += kSSEVecSize;
	}
}
