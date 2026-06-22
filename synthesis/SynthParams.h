// SynthParams.h — 패널 파라미터 저장소 + 깔끔한 getter
//
// 노브/슬라이더/토글 값은 하드웨어 파트(hw::HardwareInput)가 이미 "실제 단위"로
// 매핑해서 보내준다(LPF cutoff=Hz, EnvA=초, resonance=0..1 …). 엔진은 그 값을
// 여기에 저장하고, 합성 코드는 아래 getter 로 읽는다.
//
//   엔진:   params_.setFromControl(controlId, value);   // 하드웨어/디스플레이가 push
//   합성:   float fc = params_.lpfCutoffHz();            // 읽기
//
// 즉 네가 말한 get_LPF_Cutoff() 스타일 API 는 "읽는 쪽(합성)"에 이렇게 둔다.
// (전기신호 -> 실제단위 매핑은 hw 파트의 ControlIds.cpp 가 담당. 이중 매핑 방지)
#pragma once
#include "../hardware/ControlIds.h"
#include <atomic>

namespace syn {

class SynthParams {
public:
    // 하드웨어/디스플레이가 보낸 (controlId, 실제단위 value) 반영.
    // controlId 는 hw::ControlId 의 정수값.
    void setFromControl(int controlId, float value) {
        using hw::ControlId;
        switch (static_cast<ControlId>(controlId)) {
            // ---- 우리 패널이 실제로 쓰는 것들 (PDF 스펙 기준) ----
            case ControlId::LpfFreq:  lpfCutoffHz_ = value; break; // 20~20000 Hz
            case ControlId::LpfRes:   lpfRes_      = value; break; // 0..1
            case ControlId::LpfEnv:   lpfEnvAmt_   = value; break; // 0..1
            case ControlId::LpfLfo:   lpfLfoAmt_   = value; break; // 0..1
            case ControlId::LfoRate:  lfoRateHz_   = value; break; // 0.1~30 Hz
            case ControlId::DcoSub:   subLevel_    = value; break; // 0..1
            case ControlId::Volume:   masterVol_   = value; break; // 0..1 (이미 x^2 테이퍼 적용)
            case ControlId::EnvA:     envA_        = value; break; // s
            case ControlId::EnvD:     envD_        = value; break; // s
            case ControlId::EnvS:     envS_        = value; break; // 0..1
            case ControlId::EnvR:     envR_        = value; break; // s
            case ControlId::DcoWaveSaw:    sawOn_    = (value > 0.5f); break;
            case ControlId::DcoWaveSquare: squareOn_ = (value > 0.5f); break;
            // 패널 Unison 토글은 hw 쪽에서 Voicing(3포지션)의 UNISON(=2)으로 들어옴.
            case ControlId::Voicing:  unison_ = (static_cast<int>(value + 0.5f) == 2); break;
            default: break; // 우리가 안 쓰는 풀 Juno 파라미터는 무시
        }
    }

    // ---- 합성 코드가 읽는 getter ----
    float lpfCutoffHz() const { return lpfCutoffHz_; }
    float lpfResonance() const { return lpfRes_; }
    float lpfEnvAmount() const { return lpfEnvAmt_; }
    float lpfLfoAmount() const { return lpfLfoAmt_; }
    float lfoRateHz()   const { return lfoRateHz_; }
    float subLevel()    const { return subLevel_; }
    float masterVolume() const { return masterVol_; }
    float envAttack()   const { return envA_; }
    float envDecay()    const { return envD_; }
    float envSustain()  const { return envS_; }
    float envRelease()  const { return envR_; }
    bool  sawOn()       const { return sawOn_; }
    bool  squareOn()    const { return squareOn_; }
    bool  unison()      const { return unison_; }

private:
    // 합리적 기본값 (전원 켜자마자 소리 나도록)
    float lpfCutoffHz_ = 2000.0f;
    float lpfRes_      = 0.2f;
    float lpfEnvAmt_   = 0.6f;
    float lpfLfoAmt_   = 0.0f;
    float lfoRateHz_   = 2.0f;
    float subLevel_    = 0.3f;
    float masterVol_   = 0.7f;
    float envA_ = 0.01f, envD_ = 0.3f, envS_ = 0.7f, envR_ = 0.4f;
    bool  sawOn_ = true, squareOn_ = false, unison_ = false;
};

} // namespace syn
