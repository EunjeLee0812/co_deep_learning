// Compressor.h — 채널스트립 컴프 섹션 (레퍼런스: Apogee Symphony ECS)
#pragma once
#include "Effect.h"

namespace fx {

class Compressor : public Effect {
public:
    enum class Param : int {
        Enable       = 0,  // 0/1
        ThresholdDb  = 1,  // -50 .. 0 (dBFS)
        Ratio        = 2,  // 1 .. 20 (예: 3,5,10)
        AttackMs     = 3,  // 0.1 .. 100  (패널엔 없음 → 기본값/추가 노출 선택)
        ReleaseMs    = 4,  // 10 .. 1000  (패널엔 없음)
        DryWetMix    = 5,  // 0 .. 1  (패널 DRY/WET)
        MakeupDb     = 6,  // 0 .. 24 자동/수동
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

    // 외부 사이드체인 입력(EQ HP>SC 등) 사용 시
    void setSidechain(const float* scL, const float* scR, unsigned int n);
    // 디스플레이 게인리덕션 미터로 돌려보낼 현재 GR(dB, 음수)
    float gainReductionDb() const { return gainReductionDb_; }

private:
    bool  enabled_     = true;
    float thresholdDb_ = -20.0f;
    float ratio_       = 4.0f;
    float attackMs_    = 10.0f;
    float releaseMs_   = 100.0f;
    float dryWet_      = 1.0f;
    float makeupDb_    = 0.0f;
    float gainReductionDb_ = 0.0f; // 미터용 (process 중 갱신)
    // TODO(구현자): 피크/RMS 검출 → 정적 곡선(threshold/ratio/knee) → attack/release 평활 →
    //   게인 적용 + makeup + dry/wet. 사이드체인 있으면 검출 입력으로 사용. GR 미터 갱신.
};

} // namespace fx
