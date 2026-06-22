// Effect.h — 모든 음향 이펙터의 공통 base 인터페이스
//
// 설계 개념
//  - 디스플레이 → Bela 통신은 (paramId:int, value:float) 한 쌍으로 들어온다.
//    각 이펙트는 자신의 enum class Param 을 정의하고 setParameter() 로 그 값을 받는다.
//  - 실제 오디오 처리는 render() 안에서 블록 단위로 process() 를 호출한다 (스테레오 in-place).
//  - 세부 DSP 는 구현자가 .cpp 의 TODO 를 채운다. 헤더의 시그니처/Param 은 계약(contract)이다.
//
//  value 의 단위/범위는 각 이펙트 헤더의 Param 주석 참고 (ms, dB, Hz, 0~1 등).
#pragma once
#include <cmath>

namespace fx {

class Effect {
public:
    virtual ~Effect() = default;

    // 오디오 스트림 시작 전 1회. 샘플레이트와 최대 블록 크기로 내부 버퍼를 할당한다.
    // 성공 시 true.
    virtual bool setup(float sampleRate, unsigned int maxBlockSize) = 0;

    // 스테레오 in-place 처리. left/right 길이는 numFrames.
    // (모노 이펙트도 동일 시그니처로 두 채널을 동일 처리하거나 합쳐서 처리)
    virtual void process(float* left, float* right, unsigned int numFrames) = 0;

    // 디스플레이에서 들어온 파라미터 1개 적용. paramId 는 각 이펙트의 Param enum 값.
    virtual void setParameter(int paramId, float value) = 0;

    // 우회: 켜지면 원음을 그대로 통과시킨다.
    virtual void setBypass(bool bypass) { bypassed_ = bypass; }
    bool isBypassed() const { return bypassed_; }

    // 딜레이 라인/필터 상태 등 내부 버퍼 초기화.
    virtual void reset() = 0;

    // 자원 해제(필요 시).
    virtual void cleanup() {}

protected:
    float sampleRate_ = 44100.0f;
    bool  bypassed_   = false;
};

// ---------------------------------------------------------------------------
// SmoothedValue — 파라미터 급변 시 클릭/지퍼 노이즈 방지용 1차 스무더.
// 디스플레이에서 값이 툭툭 바뀌어도 오디오 쪽에서 부드럽게 따라가도록 쓴다.
// (이 유틸은 실제 동작하도록 구현해 둠. 그대로 써도 되고 교체해도 됨)
// ---------------------------------------------------------------------------
class SmoothedValue {
public:
    void setup(float sampleRate, float rampTimeMs) {
        const float n = (rampTimeMs * 0.001f) * sampleRate;
        coeff_ = (n > 0.0f) ? std::exp(-1.0f / n) : 0.0f;
    }
    void  setTarget(float t) { target_ = t; }
    void  snap(float v)      { current_ = target_ = v; } // 즉시 점프(초기화용)
    float next() {                                        // 매 샘플 호출
        current_ = target_ + (current_ - target_) * coeff_;
        return current_;
    }
    float current() const { return current_; }

private:
    float current_ = 0.0f;
    float target_  = 0.0f;
    float coeff_   = 0.0f;
};

} // namespace fx
