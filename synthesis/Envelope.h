// Envelope.h — ADSR 엔벨로프 (아날로그풍 지수 곡선)
//
// Attack 은 거의 선형, Decay/Release 는 지수 감쇠(아날로그 신스 느낌).
// times 단위는 초(s). 하드웨어 스펙(#8~#11)에서 이미 지수 매핑된 실제 시간이 들어온다.
//   setAttack(0.01); setDecay(0.2); setSustain(0.7); setRelease(0.4);
//   gateOn(); ... process() ...; gateOff();
#pragma once
#include <cmath>
#include <algorithm>

namespace syn {

class Envelope {
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        setAttack(0.01f); setDecay(0.2f); setSustain(0.7f); setRelease(0.3f);
        stage_ = Stage::Idle; level_ = 0.0f;
    }

    void setAttack (float s){ aRate_ = rate(s); }                 // 선형 상승률/샘플
    void setDecay  (float s){ dCoeff_ = coeff(s); }              // 지수 계수
    void setSustain(float l){ sustain_ = std::min(1.0f, std::max(0.0f, l)); }
    void setRelease(float s){ rCoeff_ = coeff(s); }

    // 키 온/오프
    void gateOn()  { stage_ = Stage::Attack; }
    void gateOff() { if (stage_ != Stage::Idle) stage_ = Stage::Release; }
    bool isActive() const { return stage_ != Stage::Idle; }

    // 한 샘플 진행, 현재 레벨(0..1) 반환
    float process() {
        switch (stage_) {
            case Stage::Idle: return 0.0f;
            case Stage::Attack:
                level_ += aRate_;
                if (level_ >= 1.0f) { level_ = 1.0f; stage_ = Stage::Decay; }
                break;
            case Stage::Decay:
                level_ = sustain_ + (level_ - sustain_) * dCoeff_;
                if (level_ <= sustain_ + 1e-4f) { level_ = sustain_; stage_ = Stage::Sustain; }
                break;
            case Stage::Sustain:
                level_ = sustain_;
                break;
            case Stage::Release:
                level_ *= rCoeff_;
                if (level_ <= 1e-4f) { level_ = 0.0f; stage_ = Stage::Idle; }
                break;
        }
        return level_;
    }

private:
    // 선형 상승률: 1.0 까지 t초 → 1/(t*fs)
    float rate(float t) const { return (t > 0.0f) ? 1.0f / (t * sampleRate_) : 1.0f; }
    // 지수 시간상수 계수: t초 동안 약 99% 도달하도록 (~4.6 시정수)
    float coeff(float t) const {
        if (t <= 0.0f) return 0.0f;
        return std::exp(-4.6f / (t * sampleRate_));
    }

    float sampleRate_ = 44100.0f;
    Stage stage_ = Stage::Idle;
    float level_ = 0.0f, sustain_ = 0.7f;
    float aRate_ = 0.01f, dCoeff_ = 0.0f, rCoeff_ = 0.0f;
};

} // namespace syn
