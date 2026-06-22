// AnalogControl.h — 아날로그 노브/페이더 1개를 추상화
#pragma once
#include "ControlIds.h"

namespace hw {

class AnalogControl {
public:
    // spec: 이 컨트롤의 범위/커브, controlRateHz: 블록당 1회 읽는 제어 주파수
    // (= audioSampleRate / blockSize). 평활 계수 계산에 사용.
    void configure(const ControlSpec& spec, float controlRateHz);

    // 매 블록 ADC 에서 읽은 raw(0~1) 를 넣는다.
    // 반환: 데드밴드를 넘는 "의미있는 변화"가 있으면 true (이때만 엔진/디스플레이에 통지)
    bool update(float raw);

    float value() const      { return scaledValue_; }   // 실제 단위(Hz/초/0~1 등)
    float normalized() const { return filtered_; }       // 평활된 0~1

    // 디스플레이에서 값을 밀어넣을 때(프리셋 로드, 터치 조작 등).
    // 물리 노브를 강제로 못 움직이므로, 이후 노브가 실제로 움직이면 다시 하드웨어가 우선.
    void setNormalizedFromDisplay(float t);
    void setValueFromDisplay(float realValue); // 실제 단위로 줄 때

private:
    ControlSpec spec_{};
    float onePoleCoeff_ = 0.0f; // raw 저역통과 계수
    float filtered_     = 0.0f; // 평활된 0~1
    float lastEmitted_  = -1.0f;// 마지막으로 통지한 0~1 (데드밴드 기준점)
    float scaledValue_  = 0.0f; // 실제 단위로 변환된 현재값
    bool  initialised_  = false;

    // 데드밴드: ADC 잡음/지터로 인한 떨림 무시. 12bit ~ 0.0005 수준 잡음 → 0.004 권장.
    static constexpr float kDeadband = 0.004f;

    float curveMap(float t) const; // 0~1 -> 실제 단위 (curve/min/max 적용)
};

} // namespace hw
