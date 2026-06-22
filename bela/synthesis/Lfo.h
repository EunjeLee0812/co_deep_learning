// Lfo.h — 저주파 오실레이터 (필터/피치 모듈레이션용)
//
// 삼각파 기본. rate 단위 Hz (스펙 #5: 0.1~30Hz). 출력은 -1..1.
// auto-wah(필터 컷오프 흔들기) / 비브라토에 라우팅한다.
#pragma once
#include <cmath>

namespace syn {

class Lfo {
public:
    void setup(float sampleRate) { sampleRate_ = sampleRate; phase_ = 0.0f; setRate(2.0f); }
    void setRate(float hz) { inc_ = hz / sampleRate_; }
    void reset() { phase_ = 0.0f; }

    // -1..1 삼각파
    float process() {
        phase_ += inc_; if (phase_ >= 1.0f) phase_ -= 1.0f;
        // 0..1 위상 -> -1..1 삼각
        return 4.0f * std::fabs(phase_ - 0.5f) - 1.0f;
    }

private:
    float sampleRate_ = 44100.0f;
    float phase_ = 0.0f, inc_ = 0.0f;
};

} // namespace syn
