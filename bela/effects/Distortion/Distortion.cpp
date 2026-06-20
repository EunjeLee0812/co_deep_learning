#include "Distortion.h"

namespace fx {

bool Distortion::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    // TODO: 톤 필터 계수 초기화, (선택) 오버샘플러 준비
    reset();
    return true;
}

void Distortion::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    // TODO(구현자): in*driveGain → shaper(mode_) → toneFilter → out*outGain → dry/wet
    (void)left; (void)right; (void)numFrames;
}

void Distortion::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::Drive:    drive_  = value; break;
        case Param::ToneHz:   toneHz_ = value; break;
        case Param::OutputDb: outDb_  = value; break;
        case Param::Mix:      mix_    = value; break;
        case Param::Mode:     mode_   = static_cast<Type>((int)value); break;
        default: break;
    }
}

void Distortion::reset() { /* TODO: 필터 상태 0 */ }

} // namespace fx
