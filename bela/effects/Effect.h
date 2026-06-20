// Effect.h — 음향 이펙터 공통 인터페이스
#pragma once

class Effect {
public:
    virtual ~Effect() = default;
    virtual float process(float in) = 0;         // 입력 샘플 -> 처리된 샘플
    virtual void  setParam(int paramId, float value) = 0;
    virtual void  setBypass(bool b) = 0;
};
