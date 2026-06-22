#include "Phaser.h"

namespace fx {

bool Phaser::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    // TODO: allpass 단(stages_개) 할당, LFO 초기화
    reset();
    return true;
}

void Phaser::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    // TODO(구현자): 각 샘플마다 allpass 계수를 LFO 로 변조, 체인 통과, 피드백 합성
    (void)left; (void)right; (void)numFrames;
}

void Phaser::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::InOut:       setBypass(value < 0.5f); break;
        case Param::ManualKHz:   manualKHz_   = value; break;
        case Param::Stages:      stages_      = (int)value; break;
        case Param::SweepDepth:  sweepDepth_  = value; break;
        case Param::SweepRateHz: sweepRateHz_ = value; break;
        case Param::FeedbackAmt: feedback_    = value; break;
        case Param::FeedbackTap: feedbackTap_ = (int)value; break;
        case Param::Polarity:    polarity_    = (value >= 0.5f); break;
        case Param::LfoPhaseDeg: lfoPhaseDeg_ = (int)value; break;
        case Param::Analog:      analog_      = (value >= 0.5f); break;
        default: break;
    }
}

void Phaser::reset() { /* TODO: allpass 상태/LFO 위상 0 */ }

} // namespace fx
