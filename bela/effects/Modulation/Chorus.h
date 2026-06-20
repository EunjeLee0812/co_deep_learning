// Chorus.h — 코러스 (레퍼런스 이미지 없음 → 표준 파라미터로 구성. 필요 시 사용자가 조정)
#pragma once
#include "Effect.h"

namespace fx {

class Chorus : public Effect {
public:
    enum class Param : int {
        InOut       = 0,  // 0/1
        RateHz      = 1,  // 0.05 .. 8 (Hz)
        DepthMs     = 2,  // 0 .. 20 (ms) 변조 폭
        DelayMs     = 3,  // 5 .. 30 (ms) 기준 딜레이
        Feedback    = 4,  // 0 .. 0.9
        Voices      = 5,  // 1 .. 4 (보이스 수)
        Mix         = 6,  // 0(dry) .. 1(wet)
        StereoWidth = 7,  // 0 .. 1
        NumParams
        // TODO(사용자): 코러스에 원하는 파라미터 있으면 알려줘 — 여기에 추가/수정할게
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

private:
    float rateHz_  = 0.5f;
    float depthMs_ = 5.0f;
    float delayMs_ = 15.0f;
    float feedback_= 0.0f;
    int   voices_  = 2;
    float mix_     = 0.5f;
    float width_   = 0.5f;
    // TODO(구현자): 보이스 수만큼 변조 딜레이라인 + 위상 분산된 LFO + 스테레오 분배.
};

} // namespace fx
