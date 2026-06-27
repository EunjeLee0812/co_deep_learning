// Voice.h — 연주 바 1개가 내는 "연속 피치" 보이스 (Oscillator+Filter+Envelope)
// ───────────────────────────────────────────────────────────────────────────
// [변경] 구버전은 코드 노트(정수 MIDI)에 보이스를 동적 할당했지만, 신버전은
//   "바 1개 = 보이스 1개" 로 고정되고 피치가 바 위치에 따라 연속적으로 변한다.
//
// 피치 = (글라이드된 중심음 center) + (즉각 반영 offset)
//   center : 링(5도권)으로 코드가 바뀌면 변하는 기준음. 짧은 글라이드로 미끄러짐.
//            → "트릴 링을 슬라이드하면 누르고 있던 음이 미끄러지듯 바뀐다" (스펙)
//   offset : 바 위치(가운데 기준 ±반음). 글라이드 없이 즉각 = 바이올린처럼 선형 연속.
//            → "바를 따라 음이 끊김 없이/등간격으로 변한다" (스펙)
//
//   매 블록:  setTargetCenter(기준음) + setOffset(반음)   ← 엔진이 계산해 전달
//   터치 ↑ :  gateOn()  (현재 center 를 목표로 스냅 → 첫 터치 swoop 방지)
//   터치 ↓ :  gateOff()
//   매 샘플:  process() → center 글라이드 → midiToHz → osc → filter → env
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "Oscillator.h"
#include "Filter.h"
#include "Envelope.h"
#include "SynthParams.h"
#include "ChordVoicing.h"   // midiToHz (구 MusicTheory 대체)
#include <cmath>

namespace syn {

// 루트 전환 글라이드 시간[초]. ★ 미끄러짐 정도 조절은 여기 한 줄.
//   작을수록(=0.01) 거의 즉각(클릭만 방지), 클수록(0.05~0.1) 또렷한 포르타멘토.
//   바 위치(offset)는 글라이드 대상이 아니므로 커도 바 추적은 항상 즉각적이다.
constexpr float kGlideSeconds = 0.020f;

class Voice {
public:
    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        osc_.setup(sampleRate);
        oscDetune_.setup(sampleRate);   // Unison 디튠 레이어
        filter_.setup(sampleRate);
        env_.setup(sampleRate);
        setGlideTime(kGlideSeconds);
        active_ = false; hasSub_ = false;
        centerMidi_ = centerTarget_ = 60.0f;
        offset_ = 0.0f;
    }

    // 글라이드(포르타멘토) 시간상수[초]. 1-pole: cur += (tgt-cur)*alpha.
    void setGlideTime(float seconds) {
        glideAlpha_ = (seconds <= 0.0f)
            ? 1.0f
            : 1.0f - std::exp(-1.0f / (seconds * sampleRate_));
    }

    // 이번 블록의 기준음(MIDI, 글라이드 대상) / 바 오프셋(반음, 즉각).
    void setTargetCenter(float centerMidi) { centerTarget_ = centerMidi; }
    void setOffset(float semitones)        { offset_       = semitones; }

    // 게이트 온: 현재 기준음을 목표로 스냅(첫 터치 시 미끄러짐 없이 바로) + ADSR.
    //   isBass=true 면 서브 오실레이터를 섞어 저음을 두껍게(베이스 바 전용).
    void gateOn(float velocity, const SynthParams& p, bool isBass) {
        velocity_   = velocity;
        // [이은제 수정] sub level 노브로 모든 바에서 서브 오실레이터 사용 가능하게.
        //   (구버전은 isBass(=Bass 바)일 때만 sub. 지금은 R5 바만 써서 안 들렸음)
        //   isBass 인자는 호환 위해 남겨두되 사용하지 않음.
        (void)isBass;
        hasSub_     = true;
        centerMidi_ = centerTarget_;   // 스냅
        env_.setAttack(p.envAttack());
        env_.setDecay(p.envDecay());
        env_.setSustain(p.envSustain());
        env_.setRelease(p.envRelease());
        filter_.reset();
        env_.gateOn();
        active_ = true;
    }

    void gateOff() { env_.gateOff(); }
    bool isActive() const { return active_ && env_.isActive(); }

    // 한 샘플 생성. lfoVal 은 엔진의 공유 LFO(-1..1).
    float process(const SynthParams& p, float lfoVal) {
        if (!active_) return 0.0f;
        if (!env_.isActive()) { active_ = false; return 0.0f; }

        // ── 피치: center 만 글라이드, offset 은 즉각 ──
        centerMidi_ += (centerTarget_ - centerMidi_) * glideAlpha_;
        const float noteMidi = centerMidi_ + offset_;    // 현재 실제 음높이(MIDI)

        // [이은제 추가] DCO LFO: LFO 로 피치를 흔드는 비브라토.
        //   깊이 1.0 에서 약 ±0.5반음(50센트). lfoVal 은 -1..1.
        const float kVibratoSemis = 0.5f;
        const float vibrato = p.dcoLfoAmount() * lfoVal * kVibratoSemis;
        const float hz = midiToHz(noteMidi + vibrato);
        osc_.setFrequency(hz);
        oscDetune_.setFrequency(hz * 1.004f);   // 약 +7센트 디튠(두께)

        // [이은제 추가] DCO PWM: LFO 로 사각파 펄스폭을 흔든다.
        //   기본 0.5(정사각) 기준 깊이 1.0 에서 약 ±0.4 까지. (0.1~0.9 클램프)
        float pw = 0.5f + p.dcoPwmAmount() * lfoVal * 0.4f;
        if (pw < 0.1f) pw = 0.1f; else if (pw > 0.9f) pw = 0.9f;

        // ── 오실레이터 믹스 (saw/square 토글, 둘 다 꺼지면 saw 폴백) ──
        float osc = 0.0f; bool any = false;
        if (p.sawOn())    { osc += osc_.processSaw();      any = true; }
        if (p.squareOn()) { osc += osc_.processSquare(pw); any = true; }  // [이은제 수정] pw 적용
        if (!any)         { osc += osc_.processSaw(); }
        else              { osc *= 0.5f; }               // 동시 출력 헤드룸
        if (p.unison())   osc = 0.7f * osc + 0.5f * oscDetune_.processSaw();
        if (hasSub_)      osc += p.subLevel() * osc_.processSub();   // 저음 보강(sub level 노브)

        // ── 필터 컷오프 모듈레이션 (옥타브/로그 도메인) ──
        const float env = env_.process();                // 0..1
        const float kEnvOct = 4.0f, kLfoOct = 2.0f;

        // [이은제 추가] LPF Track(키트래킹): 음이 높을수록 컷오프도 따라 올라간다.
        //   기준음 C3(MIDI 48) 대비 한 옥타브(12반음)마다 track 비율만큼 컷오프 1옥타브 이동.
        //   amount 1.0 = 완전 추종(음과 동일하게), 0 = 추종 없음.
        const float kTrackRefMidi = 48.0f;               // C3 기준
        const float trackOct = p.lpfTrackAmount() * (noteMidi - kTrackRefMidi) / 12.0f;

        const float mod = p.lpfEnvAmount() * env * kEnvOct
                        + p.lpfLfoAmount() * lfoVal * kLfoOct
                        + trackOct;                      // [이은제 추가] 키트래킹 합산
        filter_.setCutoff(p.lpfCutoffHz() * std::pow(2.0f, mod));
        filter_.setResonance(p.lpfResonance());
        const float y = filter_.processLP(osc);

        return y * env * velocity_;
    }

private:
    float sampleRate_ = 44100.0f;
    Oscillator osc_, oscDetune_;
    Filter     filter_;
    Envelope   env_;

    float centerMidi_  = 60.0f;   // 현재(글라이드 중) 기준음
    float centerTarget_= 60.0f;   // 목표 기준음
    float offset_      = 0.0f;    // 바 위치 오프셋(반음, 즉각)
    float glideAlpha_  = 1.0f;    // 1-pole 글라이드 계수
    float velocity_    = 1.0f;
    bool  active_      = false;
    bool  hasSub_      = false;
};

} // namespace syn
