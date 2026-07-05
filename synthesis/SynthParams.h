// SynthParams.h — 패널 파라미터 저장소 + 깔끔한 getter
//
// 노브/슬라이더/토글 값은 하드웨어 파트(hw::HardwareInput)가 이미 "실제 단위"로
// 매핑해서 보내준다(LPF cutoff=Hz, EnvA=초, resonance=0..1 …). 엔진은 그 값을
// 여기에 저장하고, 합성 코드는 아래 getter 로 읽는다.
//
//   엔진:   params_.setFromControl(controlId, value);   // 하드웨어/디스플레이가 push
//   합성:   float fc = params_.lpfCutoffHz();            // 읽기
//
// [이은제 재정비 2026] 우리 악기가 실제로 쓰는 15개 컨트롤만 남기고 정리.
//   추가: dcoLfoAmount(비브라토), dcoPwmAmount(LFO→펄스폭), lpfTrackAmount(키트래킹)
//   폐기: pitch→dco / pitch→lfo / mod→lfo (피치휠·모드휠 없음)
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
            // ---- LPF ----
            case ControlId::LpfFreq:  lpfCutoffHz_ = value; break; // 20~20000 Hz
            case ControlId::LpfRes:   lpfRes_      = value; break; // 0..1
            case ControlId::LpfEnv:   lpfEnvAmt_   = value; break; // 0..1
            case ControlId::LpfLfo:   lpfLfoAmt_   = value; break; // 0..1
            case ControlId::LpfTrack: lpfTrackAmt_ = value; break; // 0..1  [이은제 추가] 키트래킹
            // ---- LFO ----
            case ControlId::LfoRate:  lfoRateHz_   = value; break; // 0.05~20 Hz
            case ControlId::LfoDelay: lfoDelaySec_ = value; break; // s     [이은제 추가] 페이드인
            // ---- DCO ----
            case ControlId::DcoLfo:   dcoLfoAmt_   = value; break; // 0..1  [이은제 추가] 비브라토 깊이
            case ControlId::DcoSub:   subLevel_    = value; break; // 0..1
            case ControlId::DcoPwm:   dcoPwmAmt_   = value; break; // 0..1  [이은제 추가] LFO→펄스폭
            // ---- 공통 ----
            case ControlId::Volume:   masterVol_   = value; break; // 0..1
            // ---- ENV (ADSR) ----  [이은제: 페이더 11~14 매핑]
            case ControlId::EnvA:     envA_        = value; break; // s
            case ControlId::EnvD:     envD_        = value; break; // s
            case ControlId::EnvS:     envS_        = value; break; // 0..1
            case ControlId::EnvR:     envR_        = value; break; // s
            // ---- 파형 토글(구: 개별 on/off. 지금은 WaveSelect 스위치가 대체) ----
            case ControlId::DcoWaveSaw:    sawOn_    = (value > 0.5f); break;
            case ControlId::DcoWaveSquare: squareOn_ = (value > 0.5f); break;

            // ---- [이은제 2026] 물리 스위치 5개 ----
            // 파형 선택 1개 스위치: 0=Saw, 1=Square (둘 중 하나만 켜짐)
            case ControlId::WaveSelect:
                if (value > 0.5f) { sawOn_ = false; squareOn_ = true;  } // Square
                else              { sawOn_ = true;  squareOn_ = false; } // Saw
                break;
            case ControlId::QuantizeScale: quantizeOn_ = (value > 0.5f); break; // 음계 퀀타이즈
            case ControlId::LfoEnable:     lfoEnabled_ = (value > 0.5f); break; // LFO on/off
            case ControlId::OctaveUp:      octaveUp_   = (value > 0.5f); break; // 옥타브 +1
            // ControlId::SwitchSpare 는 예비 → 여기서 처리 안 함(default 로 무시)

            // ───── [폐기] 우리 악기에 없는 컨트롤 (피치휠/모드휠/유니즌 등) ─────
            // case ControlId::PitchDco:  ... 폐기 (피치휠 없음)
            // case ControlId::PitchLfo:  ... 폐기
            // case ControlId::ModLfo:    ... 폐기
            // case ControlId::Voicing:   unison_ = ...; 폐기 (해당 토글 없음)
            default: break; // 그 외 풀 Juno 파라미터는 무시
        }
    }

    // ---- 합성 코드가 읽는 getter ----
    float lpfCutoffHz()    const { return lpfCutoffHz_; }
    float lpfResonance()   const { return lpfRes_; }
    float lpfEnvAmount()   const { return lpfEnvAmt_; }
    float lpfLfoAmount()   const { return lpfLfoAmt_; }
    float lpfTrackAmount() const { return lpfTrackAmt_; }   // [이은제 추가]
    float lfoRateHz()      const { return lfoRateHz_; }
    float lfoDelaySec()    const { return lfoDelaySec_; }    // [이은제 추가]
    float dcoLfoAmount()   const { return dcoLfoAmt_; }      // [이은제 추가]
    float subLevel()       const { return subLevel_; }
    float dcoPwmAmount()   const { return dcoPwmAmt_; }      // [이은제 추가]
    float masterVolume()   const { return masterVol_; }
    float envAttack()      const { return envA_; }
    float envDecay()       const { return envD_; }
    float envSustain()     const { return envS_; }
    float envRelease()     const { return envR_; }
    bool  sawOn()          const { return sawOn_; }
    bool  squareOn()       const { return squareOn_; }
    bool  unison()         const { return unison_; }
    // [이은제 2026] 스위치 상태 getter
    bool  quantizeOn()     const { return quantizeOn_; }   // 음계 퀀타이즈
    bool  lfoEnabled()     const { return lfoEnabled_; }   // LFO on/off
    bool  octaveUp()       const { return octaveUp_; }     // 옥타브 +1

private:
    // 합리적 기본값 (전원 켜자마자 소리 나도록)
    float lpfCutoffHz_ = 2000.0f;
    float lpfRes_      = 0.2f;
    float lpfEnvAmt_   = 0.6f;
    float lpfLfoAmt_   = 0.0f;
    float lpfTrackAmt_ = 0.0f;     // [이은제 추가] 0=트래킹 없음
    float lfoRateHz_   = 2.0f;
    float lfoDelaySec_ = 0.0f;     // [이은제 추가] 0=즉시
    float dcoLfoAmt_   = 0.0f;     // [이은제 추가] 0=비브라토 없음
    float subLevel_    = 0.3f;
    float dcoPwmAmt_   = 0.0f;     // [이은제 추가] 0=고정 펄스폭(정사각)
    float masterVol_   = 0.7f;
    float envA_ = 0.01f, envD_ = 0.3f, envS_ = 0.7f, envR_ = 0.4f;
    bool  sawOn_ = true, squareOn_ = false, unison_ = false;
    // [이은제 2026] 스위치 기본값: 퀀타이즈 off, LFO on, 옥타브 시프트 off
    bool  quantizeOn_ = false;
    bool  lfoEnabled_ = true;
    bool  octaveUp_   = false;
};

} // namespace syn
