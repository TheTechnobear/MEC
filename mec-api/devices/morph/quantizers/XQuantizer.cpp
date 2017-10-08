#include <cmath>
#include "XQuantizer.h"
#include "../../../mec_log.h"

namespace mec {
namespace morph {

const float XQuantizer::PITCH_CORRECTION_STEP = 0.5;
const float XQuantizer::NO_MOVEMENT_THRESHOLD = 0.05;

XQuantizer::XQuantizer() : Quantizer() {}

void XQuantizer::quantizeNewTouch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch, const Rectangle &boundaries) {
    float centerX = boundaries.x + boundaries.width / 2.0;
    originalTouch.quantizing_offset_x_ = centerX - originalTouch.x_;
    quantizedTouch = originalTouch;
    quantizedTouch.x_ = centerX;
    quantizedTouch.c_= centerX;
    LOG_2("mec::XQuantizer::quantizeNewTouch: quantized (x:" << quantizedTouch.x_ << ",y:" << quantizedTouch.y_ << ")"
                                                             << " original (x:" << originalTouch.x_ << ",y:" << originalTouch.y_ << ")"
                                                             << " x-offset:" << originalTouch.quantizing_offset_x_);
}

void XQuantizer::quantizeContinuedTouch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch, const Rectangle &boundaries,
                                        bool roundNotePitchWhenNotMoving) {
    float centerX = boundaries.x + boundaries.width / 2.0;
    if(roundNotePitchWhenNotMoving && fabs(originalTouch.delta_x_) < NO_MOVEMENT_THRESHOLD) {
        float currentDistance = centerX - originalTouch.x_ - quantizedTouch.quantizing_offset_x_;
        if(fabs(currentDistance) > PITCH_CORRECTION_STEP) {
            if(originalTouch.x_ + originalTouch.quantizing_offset_x_ > centerX) {
                originalTouch.quantizing_offset_x_ -= PITCH_CORRECTION_STEP;
            } else {
                originalTouch.quantizing_offset_x_ += PITCH_CORRECTION_STEP;
            }
        } else {
            originalTouch.quantizing_offset_x_ = currentDistance;
        }
    }
    quantizedTouch = originalTouch;
    quantizedTouch.x_ += originalTouch.quantizing_offset_x_;
    quantizedTouch.c_ = quantizedTouch.x_;
    LOG_2("mec::XQuantizer::quantizeContinuedTouch: quantized (x:" << quantizedTouch.x_ << ",y:" << quantizedTouch.y_ << ")"
                  << " original (x:" << originalTouch.x_ << ",y:" << originalTouch.y_ << ")"
                  << " x-offset:" << originalTouch.quantizing_offset_x_);
}

void XQuantizer::retuneToOriginalPitch(TouchWithDeltas &quantizedTouch, TouchWithDeltas &originalTouch) {
    if(fabs(originalTouch.quantizing_offset_x_) > PITCH_CORRECTION_STEP) {
        if(originalTouch.x_ > originalTouch.quantizing_offset_x_) {
            originalTouch.quantizing_offset_x_ -= PITCH_CORRECTION_STEP;
        } else {
            originalTouch.quantizing_offset_x_ += PITCH_CORRECTION_STEP;
        }
    } else {
        originalTouch.quantizing_offset_x_ = 0;
    }
    quantizedTouch = originalTouch;
    quantizedTouch.x_ += originalTouch.quantizing_offset_x_;
    quantizedTouch.c_ = quantizedTouch.x_;
    LOG_2("mec::XQuantizer::retuneToOriginalPitch: quantized (x:" << quantizedTouch.x_ << ",y:" << quantizedTouch.y_ << ")"
                                                                   << " original (x:" << originalTouch.x_ << ",y:" << originalTouch.y_ << ")"
                                                                   << " x-offset:" << originalTouch.quantizing_offset_x_);
}

}
}