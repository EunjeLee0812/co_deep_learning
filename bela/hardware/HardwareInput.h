// HardwareInput.h — 하드웨어 입력 파트: 센서/ADC/GPIO/MIDI 등을 읽어 엔진에 전달
#pragma once
#include <Bela.h>
class SynthEngine;

class HardwareInput {
public:
    bool setup(BelaContext* context);
    void poll(BelaContext* context, SynthEngine& engine);  // 입력 읽어 engine 파라미터/노트 갱신
    void cleanup();
};
