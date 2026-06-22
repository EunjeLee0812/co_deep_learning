// Phaser.h — 페이저 (레퍼런스: Clearmountain's Phases 우측 모듈)
#pragma once
#include "../Effect.h"

namespace fx {

class Phaser : public Effect {
public:
    enum class Param : int {
        InOut        = 0,  // 0/1
        ManualKHz    = 1,  // 0.1 .. 10 (KHz) 중심 주파수
        Stages       = 2,  // 4 .. 12 (allpass 단수, 보통 짝수)
        SweepDepth   = 3,  // 0 .. 100 (%)
        SweepRateHz  = 4,  // LFO 속도 또는 노트분할
        FeedbackAmt  = 5,  // 0 .. 10
        FeedbackTap  = 6,  // 2 .. 12 (피드백 탭 위치)
        Polarity     = 7,  // 0/1
        LfoPhaseDeg  = 8,  // 0/90/180/270
        Analog       = 9,  // 0/1 (아날로그 모델링 on)
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

    void setLfoSync(bool on)      { lfoSync_ = on; }
    void setSweepRate(float rate) { sweepRateHz_ = rate; }

private:
    float manualKHz_  = 1.0f;
    int   stages_     = 6;
    float sweepDepth_ = 0.0f;
    float sweepRateHz_= 0.5f;
    float feedback_   = 0.0f;
    int   feedbackTap_= 4;
    bool  polarity_   = false;
    int   lfoPhaseDeg_= 0;
    bool  analog_     = false;
    bool  lfoSync_    = false;
    // TODO(구현자): N단 allpass 체인 + LFO 로 중심주파수 스윕 + 피드백.
    //  Analog 면 비선형/온도드리프트 모델링 추가.
};

} // namespace fx
