// Lfo.h — 저주파 오실레이터 (필터/피치 모듈레이션용)
//
// 삼각파 기본. rate 단위 Hz (스펙 #5: 0.1~30Hz). 출력은 -1..1.
// auto-wah(필터 컷오프 흔들기) / 비브라토에 라우팅한다.
//
// [이은제 추가] LFO Delay(페이드인):
//   게이트 온 직후 LFO 효과가 0에서 서서히 차오르게 하는 엔벨로프.
//   delaySeconds 동안 0→1 로 선형 상승. (스펙: LFO Delay 노브)
//   process() 가 (삼각파 × 페이드게인) 을 반환하므로, 라우팅 쪽 코드는 변경 불필요.
#pragma once
#include <cmath>
#include <algorithm>

namespace syn {

class Lfo {
public:
    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        phase_ = 0.0f;
        setRate(2.0f);
        setDelay(0.0f);
        fade_ = 1.0f;
    }
    void setRate(float hz) { inc_ = hz / sampleRate_; }

    // [이은제 추가] 페이드인 시간[초]. 0이면 즉시 풀 깊이.
    void setDelay(float seconds) {
        delaySeconds_ = std::max(0.0f, seconds);
        fadeInc_ = (delaySeconds_ > 0.0f) ? (1.0f / (delaySeconds_ * sampleRate_)) : 1.0f;
    }

    void reset() { phase_ = 0.0f; }

    // [이은제 추가] 새 음 게이트 온 시 호출 → 페이드를 0부터 다시 시작.
    void retrigger() { fade_ = 0.0f; }

    // -1..1 삼각파 × 페이드게인
    float process() {
        phase_ += inc_; if (phase_ >= 1.0f) phase_ -= 1.0f;
        // 0..1 위상 -> -1..1 삼각
        const float tri = 4.0f * std::fabs(phase_ - 0.5f) - 1.0f;

        // [이은제 추가] 페이드인 진행
        if (fade_ < 1.0f) { fade_ += fadeInc_; if (fade_ > 1.0f) fade_ = 1.0f; }
        return tri * fade_;
    }

private:
    float sampleRate_ = 44100.0f;
    float phase_ = 0.0f, inc_ = 0.0f;
    // [이은제 추가] 페이드인 상태
    float delaySeconds_ = 0.0f;
    float fadeInc_ = 1.0f;
    float fade_    = 1.0f;
};

} // namespace syn
