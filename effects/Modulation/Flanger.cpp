#include "Flanger.h"

namespace fx {

bool Flanger::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    // TODO: 변조 딜레이라인(최대 ~10ms) 할당, LFO 초기화
    reset();
    return true;
}

void Flanger::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    // TODO(구현자): delayMs = manualMs + LFO*sweepDepth; 분수지연 read; feedback; mix
    (void)left; (void)right; (void)numFrames;
}

void Flanger::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::InOut:        setBypass(value < 0.5f); break;
        case Param::ManualMs:     manualMs_    = value; break;
        case Param::SweepDepth:   sweepDepth_  = value; break;
        case Param::SweepRateHz:  sweepRateHz_ = value; break;
        case Param::FeedbackAmt:  feedback_    = value; break;
        case Param::FeedbackFreq: fbFreqHz_    = value; break;
        case Param::Polarity:     polarity_    = (value >= 0.5f); break;
        case Param::MixDryThru:   mix_         = value; break;
        case Param::LfoPhaseDeg:  lfoPhaseDeg_ = (int)value; break;
        case Param::TapeFlange:   tapeFlange_  = (value >= 0.5f); break;
        case Param::BbdType:      bbdType_     = (int)value; break;
        default: break;
    }
}

void Flanger::reset() { /* TODO: 딜레이라인/LFO 위상 0 */ }

} // namespace fx
