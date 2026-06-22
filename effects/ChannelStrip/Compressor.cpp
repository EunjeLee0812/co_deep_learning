#include "Compressor.h"

namespace fx {

bool Compressor::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    // TODO: 검출기 계수(attack/release), 엔벨로프 초기화
    reset();
    return true;
}

void Compressor::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_ || !enabled_) return;
    // TODO(구현자): 채널 결합 검출 → GR 계산 → 좌우 게인 적용 → makeup/drywet
    //   gainReductionDb_ 갱신해 디스플레이 미터로 보낼 수 있게 할 것
    (void)left; (void)right; (void)numFrames;
}

void Compressor::setSidechain(const float* scL, const float* scR, unsigned int n) {
    // TODO: 사이드체인 버퍼 저장 (다음 process 에서 검출 입력으로 사용)
    (void)scL; (void)scR; (void)n;
}

void Compressor::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::Enable:      enabled_     = (value >= 0.5f); break;
        case Param::ThresholdDb: thresholdDb_ = value; break;
        case Param::Ratio:       ratio_       = value; break;
        case Param::AttackMs:    attackMs_    = value; break;
        case Param::ReleaseMs:   releaseMs_   = value; break;
        case Param::DryWetMix:   dryWet_      = value; break;
        case Param::MakeupDb:    makeupDb_    = value; break;
        default: break;
    }
}

void Compressor::reset() { gainReductionDb_ = 0.0f; /* TODO: 엔벨로프 0 */ }

} // namespace fx
