#include "Equalizer.h"

namespace fx {

bool Equalizer::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    // TODO: 바이쿼드 계수 초기 계산
    reset();
    return true;
}

void Equalizer::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_ || !enabled_) return;
    // TODO(구현자): HP → loShelf → midPeak → hiShelf 순으로 좌우 바이쿼드 통과
    (void)left; (void)right; (void)numFrames;
}

void Equalizer::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::Enable:        enabled_ = (value >= 0.5f); break;
        case Param::HpFreqHz:      hpFreq_  = value; break;
        case Param::HpRouting:     hpRoute_ = static_cast<HpRoute>((int)value); break;
        case Param::LoShelfFreqHz: loFreq_  = value; break;
        case Param::LoShelfGainDb: loGain_  = value; break;
        case Param::MidFreqHz:     midFreq_ = value; break;
        case Param::MidGainDb:     midGain_ = value; break;
        case Param::MidBell:       midBell_ = (value >= 0.5f); break;
        case Param::HiShelfFreqHz: hiFreq_  = value; break;
        case Param::HiShelfGainDb: hiGain_  = value; break;
        default: break;
    }
    // TODO: 변경된 밴드의 바이쿼드 계수 재계산
}

void Equalizer::reset() { /* TODO: 바이쿼드 상태(z 지연) 0 */ }

} // namespace fx
