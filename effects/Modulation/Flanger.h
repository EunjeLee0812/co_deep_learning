// Flanger.h — 플랜저 (짧은 변조 딜레이 + 강한 피드백 = 제트 사운드)
// ───────────────────────────────────────────────────────────────────────────
// 디스플레이 노브 매핑(사용자 스펙):
//   Rate, BpmSync(버튼), Depth, Feedback, Phase(좌우 위상차), Mix
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"

namespace fx {

class Flanger : public Effect {
public:
    enum class Param : int {
        InOut    = 0,  // 0/1
        Rate     = 1,  // sync off: 0.05..8 Hz / sync on: 0..6 노트분할
        BpmSync  = 2,  // 0/1
        Depth    = 3,  // 0..1  변조 깊이
        Feedback = 4,  // 0..0.95 (음수 위상은 내부 폭 조절)
        Phase    = 5,  // 0..1  좌우 LFO 위상차
        Mix      = 6,  // 0(dry)..1(wet)
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

    bool  inOut_    = true;
    float rateRaw_  = 0.3f;
    bool  bpmSync_  = false;
    float depth_    = 0.6f;
    float feedback_ = 0.5f;
    float phase_    = 0.0f;
    float mix_      = 0.5f;
    float tempo_    = 120.0f;

    float fbL_ = 0.0f, fbR_ = 0.0f;

    void  updateRate();
    float msToSamples(float ms) const { return ms * 0.001f * sampleRate_; }
};

} // namespace fx
