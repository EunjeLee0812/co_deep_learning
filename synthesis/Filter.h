// Filter.h — 레조넌스 있는 저역통과 필터 (TPT State-Variable Filter)
//
// Zavalishin 의 TPT(Topology-Preserving Transform) SVF. 컷오프를 빠르게 흔들어도
// 안정적이라 LFO/엔벨로프 모듈레이션에 적합하다.
//   cutoff : Hz
//   res    : 0..1 (0.9 이상에서 self-oscillation 영역 — 스펙 #2)
// 매 샘플 setCutoff() 로 변조된 컷오프를 넣고 processLP() 를 호출하면 된다.
#pragma once
#include <cmath>
#include <algorithm>

namespace syn {

class Filter {
public:
    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
        setCutoff(1000.0f);
        setResonance(0.2f);
    }

    void reset() { ic1eq_ = 0.0f; ic2eq_ = 0.0f; }

    // 컷오프(Hz). 매 샘플 모듈레이션해도 됨. 20Hz~Nyquist*0.45 로 클램프.
    void setCutoff(float hz) {
        const float maxF = sampleRate_ * 0.45f;
        hz = std::min(maxF, std::max(20.0f, hz));
        g_ = std::tan(float(M_PI) * hz / sampleRate_);
        updateCoeffs();
    }

    // 레조넌스 0..1. 내부적으로 k(=1/Q)로 변환. res→1 일수록 k→0(강한 공진).
    void setResonance(float res) {
        res = std::min(1.0f, std::max(0.0f, res));
        // res 0 -> k≈2(Q 0.5, 거의 평탄), res 1 -> k≈0.05(self-osc 직전)
        k_ = 2.0f - 1.95f * res;
        updateCoeffs();
    }

    // 저역통과 출력 1샘플.
    float processLP(float x) {
        const float v3 = x - ic2eq_;
        const float v1 = a1_ * ic1eq_ + a2_ * v3;
        const float v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
        ic1eq_ = 2.0f * v1 - ic1eq_;
        ic2eq_ = 2.0f * v2 - ic2eq_;
        return v2; // lowpass
    }

private:
    void updateCoeffs() {
        a1_ = 1.0f / (1.0f + g_ * (g_ + k_));
        a2_ = g_ * a1_;
        a3_ = g_ * a2_;
    }

    float sampleRate_ = 44100.0f;
    float g_ = 0.0f, k_ = 1.0f;
    float a1_ = 0.0f, a2_ = 0.0f, a3_ = 0.0f;
    float ic1eq_ = 0.0f, ic2eq_ = 0.0f;
};

} // namespace syn
