#include "MLVector.h"


#include "arm_neon.h"

MLVec MLVec::getIntPart() const
{
    int32x4_t vi = vcvtq_s32_f32(val.v);    // convert with truncate
    return MLVec(vcvtq_f32_s32(vi));
}

MLVec MLVec::getFracPart() const
{
    int32x4_t vi = vcvtq_s32_f32(val.v);    // convert with truncate
    float32x4_t intPart = vcvtq_f32_s32(vi);
    return MLVec(vsubq_f32(val.v, intPart));
}

void MLVec::getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const
{
    int32x4_t vi = vcvtq_s32_f32(val.v);    // convert with truncate
    intPart = vcvtq_f32_s32(vi);
    fracPart = vsubq_f32(val.v, intPart.val.v);
}

