#include "Chorus.h"

namespace fx {

bool Chorus::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    // TODO: voices_ 개 딜레이라인/LFO 할당
    reset();
    return true;
}

void Chorus::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    // TODO(구현자): 각 보이스 변조 딜레이 read → 합산 → mix/width 적용
    (void)left; (void)right; (void)numFrames;
}

void Chorus::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::InOut:       setBypass(value < 0.5f); break;
        case Param::RateHz:      rateHz_  = value; break;
        case Param::DepthMs:     depthMs_ = value; break;
        case Param::DelayMs:     delayMs_ = value; break;
        case Param::Feedback:    feedback_= value; break;
        case Param::Voices:      voices_  = (int)value; break;
        case Param::Mix:         mix_     = value; break;
        case Param::StereoWidth: width_   = value; break;
        default: break;
    }
}

void Chorus::reset() { /* TODO: 딜레이라인/LFO 위상 0 */ }

} // namespace fx
