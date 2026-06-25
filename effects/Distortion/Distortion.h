// Distortion.h — 디스토션 (tube/soft/hard + 모핑 SVF EQ + drive/mix)
// ───────────────────────────────────────────────────────────────────────────
// 디스플레이 노브 매핑(사용자 스펙):
//   공통   : Gain(입력), Out(출력)
//   Type   : Tube / SoftClip / HardClip 선택
//   EqPos  : Off / Pre / Post (EQ 위치 버튼)
//   EqMorph: 0=LowCut(HP) ↔ 0.5=Band(BP) ↔ 1=HighCut(LP) 연속 블렌딩(이산 아님)
//   EqFreq : 모핑 EQ 코너 주파수(1개)
//   EqReso : 모핑 EQ 레조넌스
//   Drive  : 왜곡량(셰이퍼 입력 게인)
//   Mix    : dry/wet
//
// 신호 흐름:
//   in → ×Gain → dry 분기
//      wet: [EQ(Pre)] → ×Drive → shaper(Type) → [EQ(Post)] → DC차단
//      out = (dry*(1-Mix) + wet*Mix) × Out
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"

namespace fx {

class Distortion : public Effect {
public:
    enum class Type   : int { Tube = 0, SoftClip = 1, HardClip = 2 };
    enum class EqPos  : int { Off = 0, Pre = 1, Post = 2 };

    enum class Param : int {
        Gain    = 0,  // -inf..+12 dB
        Out     = 1,  // -inf..+12 dB
        Type    = 2,  // 0=Tube,1=SoftClip,2=HardClip
        EqPos   = 3,  // 0=Off,1=Pre,2=Post
        EqMorph = 4,  // 0(HP)..0.5(BP)..1(LP) 연속
        EqFreq  = 5,  // 50..12000 Hz
        EqReso  = 6,  // 0..1
        Drive   = 7,  // 0..1
        Mix     = 8,  // 0(dry)..1(wet)
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

private:
    dsp::StateVariableFilter svfL_, svfR_;  // 모핑 EQ
    dsp::OnePole dcL_, dcR_;                 // tube DC 차단

    SmoothedValue inGain_, outGain_, mix_, drive_;
    Type   type_    = Type::SoftClip;
    EqPos  eqPos_   = EqPos::Off;
    float  eqMorph_ = 0.5f;

    inline float shape(float x) const;
    inline float morphFilter(dsp::StateVariableFilter& svf, float x) const;
};

} // namespace fx
