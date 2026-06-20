// SynthEngine.h — 소리 생성 파트: 악기 + 이펙터 체인 + 믹스를 총괄
#pragma once
#include "../core/Types.h"

class SynthEngine {
public:
    bool  setup(float sampleRate);
    float process();                              // instruments -> effects -> out
    void  setParam(core::ParamId id, float value); // comm/hardware 가 호출
    void  cleanup();
};
