#include "AnalogControl.h"
#include <cmath>
#include <algorithm>

namespace hw {

void AnalogControl::configure(const ControlSpec& spec, float controlRateHz) {
    spec_ = spec;
    // 1-pole 저역통과 계수. smoothingMs 동안 목표의 약 63% 도달.
    // y[n] = y[n-1] + (1-a)*(x - y[n-1]),  a = exp(-1 / (t*fs))
    const float n = (spec.smoothingMs * 0.001f) * controlRateHz;
    onePoleCoeff_ = (n > 0.0f) ? std::exp(-1.0f / n) : 0.0f;
    initialised_ = false;
}

bool AnalogControl::update(float raw) {
    raw = std::min(1.0f, std::max(0.0f, raw)); // 안전 클램프

    if (!initialised_) {                 // 첫 블록: 필터를 현재값으로 채워 튀는 것 방지
        filtered_ = raw;
        lastEmitted_ = raw;
        scaledValue_ = curveMap(filtered_);
        initialised_ = true;
        return true;                     // 초기값 1회 통지
    }

    // 1) 노이즈 저역통과
    filtered_ = raw + (filtered_ - raw) * onePoleCoeff_;

    // 2) 데드밴드 — 마지막 통지값에서 충분히 벗어났을 때만 "움직였다"로 본다.
    //    (가만히 있는 노브가 디스플레이 설정을 계속 덮어쓰지 않도록 하는 핵심)
    if (std::fabs(filtered_ - lastEmitted_) < kDeadband)
        return false;

    lastEmitted_ = filtered_;
    scaledValue_ = curveMap(filtered_);  // 3) 실제 단위로 변환
    return true;
}

void AnalogControl::setNormalizedFromDisplay(float t) {
    t = std::min(1.0f, std::max(0.0f, t));
    // 물리 노브 위치(filtered_)는 건드리지 않는다. 값만 디스플레이 기준으로 갱신.
    // lastEmitted_ 를 t 로 옮겨두면, 노브가 t 근처를 지날 때 불필요한 재통지를 피함.
    lastEmitted_ = t;
    scaledValue_ = curveMap(t);
    // TODO(선택): "soft pickup" 모드 — 노브 물리위치가 t 를 통과하기 전까지
    //   하드웨어 변화를 무시해 값 점프를 없앨 수 있음. 지금은 즉시(jump) 방식.
}

void AnalogControl::setValueFromDisplay(float realValue) {
    // 실제 단위 -> 0~1 역변환 후 위 함수 재사용
    float t;
    if (spec_.curve == Curve::Exp && spec_.min > 0.0f)
        t = std::log(realValue / spec_.min) / std::log(spec_.max / spec_.min);
    else
        t = (realValue - spec_.min) / (spec_.max - spec_.min);
    setNormalizedFromDisplay(t);
}

float AnalogControl::curveMap(float t) const {
    if (spec_.curve == Curve::Exp && spec_.min > 0.0f) {
        // 기하 보간: 주파수/시간처럼 지수적인 양에 자연스러움
        return spec_.min * std::pow(spec_.max / spec_.min, t);
    }
    return spec_.min + t * (spec_.max - spec_.min); // 선형
}

} // namespace hw
