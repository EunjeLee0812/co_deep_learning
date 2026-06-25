// Compressor.h — 채널스트립 컴프 섹션
// ───────────────────────────────────────────────────────────────────────────
// 디스플레이 노브 매핑(사용자 스펙):
//   On/Off 토글, Threshold, Ratio 선택(3:1/5:1/10:1), Dry/Wet Mix.
//   Attack/Release/Makeup 은 패널에 없음 → 내부 기본값(필요 시 노출).
//
// 검출(detection): 외부 사이드체인이 주입되면 그것으로, 아니면 메인 입력으로.
//   EQ 의 HP>SC 라우팅이면 ChannelStrip 이 EQ 사이드체인을 여기로 넣어준다.
// 스테레오 링크: 좌우 max 레벨로 검출해 동일 게인 적용(이미지 흔들림 방지).
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"

namespace fx {

class Compressor : public Effect {
public:
    enum class Param : int {
        Enable       = 0,  // 0/1
        ThresholdDb  = 1,  // -50 .. 0 (dBFS)
        Ratio        = 2,  // 3 / 5 / 10 (직접 비율값)
        AttackMs     = 3,  // 0.1 .. 100  (패널엔 없음)
        ReleaseMs    = 4,  // 10 .. 1000  (패널엔 없음)
        DryWetMix    = 5,  // 0 .. 1
        MakeupDb     = 6,  // 0 .. 24
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

    // 외부 사이드체인 입력(EQ HP>SC 등). 이 블록에 한해 검출 입력으로 사용.
    void setSidechain(const float* scL, const float* scR, unsigned int n);
    // 디스플레이 게인리덕션 미터용(dB, 음수)
    float gainReductionDb() const { return gainReductionDb_; }

private:
    bool  enabled_     = true;
    float thresholdDb_ = -20.0f;
    float ratio_       = 4.0f;
    float attackMs_    = 10.0f;
    float releaseMs_   = 120.0f;
    float dryWet_      = 1.0f;
    float makeupDb_    = 0.0f;
    float kneeDb_      = 6.0f;     // 소프트니 폭(고정)

    float envDb_       = -90.0f;   // 게인리덕션 평활 상태(dB)
    float atkCoeff_    = 0.0f;
    float relCoeff_    = 0.0f;
    float gainReductionDb_ = 0.0f; // 미터용

    const float* scL_ = nullptr;
    const float* scR_ = nullptr;
    bool         hasSidechain_ = false;

    void recalcBallistics();
    inline float computeTargetGRdb(float levelDb) const;  // 정적 곡선
};

} // namespace fx
