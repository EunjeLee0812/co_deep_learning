// Flanger.h — 플렌저 (레퍼런스: Clearmountain's Phases 좌측 모듈)
// ModulationSet 안의 서브 모듈. 단독으로도 Effect 로 동작 가능.
#pragma once
#include "Effect.h"

namespace fx {

class Flanger : public Effect {
public:
    enum class Param : int {
        InOut        = 0,  // 0/1  모듈 on/off (패널 IN/OUT)
        ManualMs     = 1,  // 0.2 .. 4.0  기준 딜레이(ms)
        SweepDepth   = 2,  // 0 .. 100 (%)
        SweepRateHz  = 3,  // LFO 속도(Hz) 또는 sync 시 노트분할 인덱스
        FeedbackAmt  = 4,  // 0 .. 10
        FeedbackFreq = 5,  // 20 .. 1000 (Hz) 피드백 필터
        Polarity     = 6,  // 0/1  피드백 위상
        MixDryThru   = 7,  // 0(dry) .. 1(through-delay)
        LfoPhaseDeg  = 8,  // 0/90/180/270
        TapeFlange   = 9,  // 0/1
        BbdType      = 10, // 0..5 (BBD/SAD/NTE/MN/TCA/WD 등 모델 선택)
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

    // 세트(ModulationSet)에서 글로벌 LFO sync / sweep rate 를 주입할 때 사용
    void setLfoSync(bool on)      { lfoSync_ = on; }
    void setSweepRate(float rate) { sweepRateHz_ = rate; }

private:
    float manualMs_   = 1.0f;
    float sweepDepth_ = 0.0f;
    float sweepRateHz_= 0.5f;
    float feedback_   = 0.0f;
    float fbFreqHz_   = 500.0f;
    bool  polarity_   = false;
    float mix_        = 0.5f;
    int   lfoPhaseDeg_= 0;
    bool  tapeFlange_ = false;
    int   bbdType_    = 0;
    bool  lfoSync_    = false;
    // TODO(구현자): 짧은 변조 딜레이라인 + LFO(삼각/사인) + 피드백 + 피드백 1-pole 필터.
    //  TapeFlange/BbdType 에 따라 보간/대역제한 특성 변경.
};

} // namespace fx
