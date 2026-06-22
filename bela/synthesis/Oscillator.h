// Oscillator.h — 밴드리미티드 오실레이터 (PolyBLEP)
//
// Juno-106 의 DCO 를 흉내낸다: 톱니(saw) + 사각(square) + 한 옥타브 아래 서브.
// naive 톱니/사각을 그냥 쓰면 고음에서 알리아싱(쇳소리)이 심하므로,
// 불연속 지점에서 PolyBLEP 보정을 넣어 밴드리밋한다.
//
// 사용법:
//   Oscillator o; o.setup(sampleRate); o.setFrequency(440.f);
//   float s = o.processSaw();   // 매 샘플 호출
#pragma once
#include <cmath>

namespace syn {

class Oscillator {
public:
    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        phase_    = 0.0f;
        subPhase_ = 0.0f;
        setFrequency(440.0f);
    }

    // 기준 주파수(Hz). 서브는 자동으로 한 옥타브 아래(f/2).
    void setFrequency(float hz) {
        freq_ = hz;
        inc_    = hz / sampleRate_;          // 0..1 정규화 위상 증가분
        subInc_ = (hz * 0.5f) / sampleRate_; // 서브 = 한 옥타브 아래
    }

    void reset() { phase_ = 0.0f; subPhase_ = 0.0f; }

    // ---- 한 샘플 생성 ----
    // 톱니파: 모든 배음 포함, 가장 풍부 (스펙 #12)
    float processSaw() {
        float v = 2.0f * phase_ - 1.0f;      // naive 톱니
        v -= polyBlep(phase_, inc_);         // 불연속 보정
        advance();
        return v;
    }

    // 사각파: 홀수 배음, 속이 빈 캐릭터 (스펙 #13). pulseWidth 0.5 = 정사각.
    float processSquare(float pulseWidth = 0.5f) {
        float v = (phase_ < pulseWidth) ? 1.0f : -1.0f;
        v += polyBlep(phase_, inc_);                       // 상승 엣지
        float p2 = phase_ + (1.0f - pulseWidth);
        if (p2 >= 1.0f) p2 -= 1.0f;
        v -= polyBlep(p2, inc_);                           // 하강 엣지
        advance();
        return v;
    }

    // 서브 오실레이터: 한 옥타브 아래 사각파 (저음 두껍게, 스펙 #6)
    float processSub() {
        float v = (subPhase_ < 0.5f) ? 1.0f : -1.0f;
        v += polyBlep(subPhase_, subInc_);
        float p2 = subPhase_ + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
        v -= polyBlep(p2, subInc_);
        subPhase_ += subInc_; if (subPhase_ >= 1.0f) subPhase_ -= 1.0f;
        return v;
    }

    float frequency() const { return freq_; }

private:
    void advance() { phase_ += inc_; if (phase_ >= 1.0f) phase_ -= 1.0f; }

    // PolyBLEP: 위상 불연속(0 근처/1 근처)에서 1샘플 폭의 보정 곡선을 더해
    // 알리아싱을 줄인다. dt = 위상 증가분(정규화 주파수).
    static float polyBlep(float t, float dt) {
        if (dt <= 0.0f) return 0.0f;
        if (t < dt) {                 // 막 위상이 0을 지난 직후
            t /= dt;
            return t + t - t * t - 1.0f;
        } else if (t > 1.0f - dt) {   // 위상이 1로 감기 직전
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    float sampleRate_ = 44100.0f;
    float freq_   = 440.0f;
    float phase_  = 0.0f, inc_    = 0.0f;
    float subPhase_ = 0.0f, subInc_ = 0.0f;
};

} // namespace syn
