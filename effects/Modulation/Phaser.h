// Phaser.h — 페이저 (N단 1차 allpass 체인 + LFO 스윕 + 피드백)
// ───────────────────────────────────────────────────────────────────────────
// 디스플레이 노브 매핑(사용자 스펙):
//   Rate, BpmSync(버튼), Depth, Freq(중심주파수), Feedback, Phase(좌우 위상차), Mix
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"

namespace fx {

class Phaser : public Effect {
public:
    enum class Param : int {
        InOut    = 0,  // 0/1
        Rate     = 1,  // sync off: 0.05..8 Hz / sync on: 0..6 노트분할
        BpmSync  = 2,  // 0/1
        Depth    = 3,  // 0..1  스윕 폭(옥타브)
        Freq     = 4,  // 100..5000 Hz  스윕 중심
        Feedback = 5,  // 0..0.95
        Phase    = 6,  // 0..1  좌우 LFO 위상차(0=모노,0.5=180도)
        Mix      = 7,  // 0(dry)..1(wet)
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

    void setTempo(float bpm) { tempo_ = bpm; updateRate(); }

private:
    static constexpr int kStages = 6;   // allpass 단수(짝수=노치 3개)

    struct Ap1 {   // 1차 allpass
        float a = 0.0f, xz = 0.0f, yz = 0.0f;
        inline float process(float x) { float y = a * x + xz - a * yz; xz = x; yz = y; return y; }
        void reset() { xz = yz = 0.0f; }
    };
    Ap1 apL_[kStages], apR_[kStages];
    dsp::Lfo lfo_;

    bool  inOut_    = true;
    float rateRaw_  = 0.3f;
    bool  bpmSync_  = false;
    float depth_    = 0.5f;
    float centerHz_ = 800.0f;
    float feedback_ = 0.3f;
    float phase_    = 0.5f;
    float mix_      = 0.5f;
    float tempo_    = 120.0f;

    float fbL_ = 0.0f, fbR_ = 0.0f;

    void updateRate();
    inline float coeffForFreq(float fc) const {
        float t = std::tan(dsp::kPi * dsp::clampf(fc, 20.0f, sampleRate_ * 0.49f) / sampleRate_);
        return (t - 1.0f) / (t + 1.0f);
    }
};

} // namespace fx
