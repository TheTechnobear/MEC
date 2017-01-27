#include "arm_neon.h"

void MLSignal::log2Approx()
{
	MLSample* px1 = getBuffer();
    
	int c = getSize() >> kMLSamplesPerSSEVectorBits;
	float32x4_t vx1, vy1;
	
	for (int n = 0; n < c; ++n)
	{
		vx1 = vld1q_f32(px1);
		vy1 = log2Approx4(vx1); 		
		vst1q_f32(px1, vy1);
		px1 += kSSEVecSize;
	}
}
