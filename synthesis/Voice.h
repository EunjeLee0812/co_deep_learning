// Voice.h — 한 음을 내는 보이스 (Oscillator + Filter + Envelope 조립)
//
// 코드의 노트 하나당 Voice 하나가 할당된다. SynthEngine 이 보이스 풀을 관리하고,
// 트리거 시 setNote()/gateOn(), 릴리즈 시 gateOff() 를 호출한다.
//
// 모듈레이션:
//   - 필터 컷오프 = 기준컷오프 * 2^(envAmt*env*EnvOct + lfoAmt*lfo*LfoOct)
//     (로그=옥타브 도메인에서 변조해야 청감상 자연스러움 — PDF DSP 노트와 동일 정신)
//   - 진폭 = ADSR 엔벨로프 * 벨로시티
//   - 서브 오실레이터는 "베이스 보이스"에만 섞어 저음을 두껍게 (스펙 #6)
#pragma once
#include "Oscillator.h"
#include "Filter.h"
#include "Envelope.h"
#include "SynthParams.h"
#include "MusicTheory.h"

namespace syn {

class Voice {
public:
    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        osc_.setup(sampleRate);
        oscDetune_.setup(sampleRate);  // Unison 용 디튠 레이어
        filter_.setup(sampleRate);
        env_.setup(sampleRate);
        active_ = false;
        hasSub_ = false;
    }

    // 노트 시작: MIDI 노트로 주파수 설정 + ADSR 적용 + 게이트 온.
    // isBass=true 면 서브 오실레이터를 섞는다(코드 최저음).
    void noteOn(int midi, float velocity, const SynthParams& p, bool isBass) {
        midi_     = midi;
        velocity_ = velocity;
        hasSub_   = isBass;
        float hz  = midiToHz((float)midi);
        osc_.setFrequency(hz);
        oscDetune_.setFrequency(hz * 1.004f); // 약 +7센트 디튠(두께)
        env_.setAttack(p.envAttack());
        env_.setDecay(p.envDecay());
        env_.setSustain(p.envSustain());
        env_.setRelease(p.envRelease());
        filter_.reset();
        env_.gateOn();
        active_ = true;
    }

    void gateOff() { env_.gateOff(); }
    void kill()    { env_.gateOff(); active_ = false; }

    bool isActive()  const { return active_ && env_.isActive(); }
    int  note()      const { return midi_; }

    // 한 샘플 생성. lfoVal 은 엔진이 만든 공유 LFO(-1..1).
    float process(const SynthParams& p, float lfoVal) {
        if (!active_) return 0.0f;
        if (!env_.isActive()) { active_ = false; return 0.0f; }

        // ── 오실레이터 믹스 (saw/square 토글, 둘 다 꺼지면 saw 로 폴백) ──
        float osc = 0.0f;
        bool any = false;
        if (p.sawOn())    { osc += osc_.processSaw();           any = true; }
        if (p.squareOn()) { osc += osc_.processSquare(0.5f);    any = true; }
        if (!any)         { osc += osc_.processSaw();                       }
        else              { osc *= 0.5f; } // 두 파형 동시 시 클리핑 여유

        // Unison: 살짝 디튠된 saw 레이어 추가 → 두꺼운 사운드
        // (원래 Juno Unison 은 "모든 보이스 같은 음"이지만, 본 악기는 코드 기반이라
        //  보이스별 디튠 레이어로 해석. 바꾸려면 여기만 수정.)
        if (p.unison()) osc = 0.7f * osc + 0.5f * oscDetune_.processSaw();

        // 서브 오실레이터(베이스 보이스만): 한 옥타브 아래로 저음 보강
        if (hasSub_) osc += p.subLevel() * osc_.processSub();

        // ── 필터 컷오프 모듈레이션 (옥타브/로그 도메인) ──
        float env = env_.process();                       // 0..1
        const float kEnvOct = 4.0f;   // 엔벨로프가 컷오프를 흔드는 최대 옥타브
        const float kLfoOct = 2.0f;   // LFO 가 컷오프를 흔드는 최대 옥타브
        float mod = p.lpfEnvAmount() * env * kEnvOct
                  + p.lpfLfoAmount() * lfoVal * kLfoOct;
        float cutoff = p.lpfCutoffHz() * std::pow(2.0f, mod);
        filter_.setCutoff(cutoff);
        filter_.setResonance(p.lpfResonance());

        float y = filter_.processLP(osc);

        // 진폭: 엔벨로프 * 벨로시티
        return y * env * velocity_;
    }

private:
    float sampleRate_ = 44100.0f;
    Oscillator osc_;
    Oscillator oscDetune_;
    Filter     filter_;
    Envelope   env_;

    int   midi_     = 60;
    float velocity_ = 1.0f;
    bool  active_   = false;
    bool  hasSub_   = false;
};

} // namespace syn
