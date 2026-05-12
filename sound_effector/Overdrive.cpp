#include "Overdrive.h"
#include <cmath>
#include <algorithm> // std::min, std::max를 위해 필요

Overdrive::Overdrive() : gGain(10.0f), gMix(0.5f), gMode(OverdriveMode::kSoft) {}

void Overdrive::setGain(float gain) { 
    // std::clamp 대신 std::max 사용
    gGain = (gain > 1.0f) ? gain : 1.0f; 
}

void Overdrive::setMix(float mix) { 
    // std::clamp(mix, 0.0f, 1.0f)와 동일한 동작
    if (mix < 0.0f) gMix = 0.0f;
    else if (mix > 1.0f) gMix = 1.0f;
    else gMix = mix;
}

void Overdrive::setMode(OverdriveMode mode) { gMode = mode; }

float Overdrive::process(float input) {
    float amplified = input * gGain;
    float distorted;

    if (gMode == OverdriveMode::kHard) {
        distorted = hard_clip(amplified);
    } else {
        distorted = soft_clip(amplified);
    }

    return (input * (1.0f - gMix)) + (distorted * gMix);
}

float Overdrive::hard_clip(float in) {
    float in_abs = std::abs(in);
    float sign = (in > 0) ? 1.0f : -1.0f;
    
    constexpr float th = 1.0f / 3.0f;

    if (in_abs < th) {
        return 2.0f * in;
    } else if (in_abs > 2.0f * th) {
        return sign;
    } else {
        float tmp = 2.0f - 3.0f * in_abs;
        return sign * (3.0f - (tmp * tmp)) / 3.0f;
    }
}

float Overdrive::soft_clip(float in) {
    return (in > 0 ? 1.0f : -1.0f) * (1.0f - std::exp(-std::abs(in)));
}