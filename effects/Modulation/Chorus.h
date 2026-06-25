// Chorus.h — 코러스 (2-보이스 변조 딜레이 + 피드백 + LPF)
// ───────────────────────────────────────────────────────────────────────────
// 디스플레이 노브 매핑(사용자 스펙):
//   Rate, BpmSync(버튼), Delay1, Delay2, Depth, Feedback, Lpf, Mix
//   Rate 는 BpmSync off=Hz, on=노트분할 인덱스(0..6). 템포는 setTempo()로 주입.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"

namespace fx {

class Chorus : public Effect {
public:
    enum class Param : int {
        InOut    = 0,  // 0/1
        Rate     = 1,  // sync off: 0.05..8 Hz / sync on: 0..6 노트분할
        BpmSync  = 2,  // 0/1
        Delay1Ms = 3,  // 5..30 ms  보이스1 기준 딜레이
        Delay2Ms = 4,  // 5..30 ms  보이스2 기준 딜레이
        Depth    = 5,  // 0..1  변조 깊이(최대 ±~5ms)
        Feedback = 6,  // 0..0.9
        Lpf      = 7,  // 1000..18000 Hz  wet 저역통과
        Mix      = 8,  // 0(dry)..1(wet)
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

    void setTempo(float bpm) { tempo_ = bpm; updateRate(); }

private:
    dsp::DelayLine lineL_, lineR_;
    dsp::Lfo       lfo_;
    dsp::OnePole   lpfL_, lpfR_;

    bool  inOut_    = true;
    float rateRaw_  = 0.5f;
    bool  bpmSync_  = false;
    float delay1Ms_ = 12.0f;
    float delay2Ms_ = 20.0f;
    float depth_    = 0.4f;
    float feedback_ = 0.0f;
    float tempo_    = 120.0f;
    float mix_      = 0.5f;

    float fbStateL_ = 0.0f, fbStateR_ = 0.0f;

    void updateRate();
    float msToSamples(float ms) const { return ms * 0.001f * sampleRate_; }
};

} // namespace fx
