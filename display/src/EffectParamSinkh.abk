// EffectParamSink.h
// 디스플레이(터치/노브)에서 일어난 변경을 외부(오디오 엔진/이펙트)로 내보내는 통로.
// 오디오 엔진이 이걸 구현하면, 디스플레이 조작이 곧 이펙트 파라미터 변경이 된다.
//
// effects/ 의 각 이펙트는 setParameter(paramId, value) 를 가지므로,
// 여기서는 (어느 이펙트 객체 + 그 안의 paramId + 실제값) 으로 보낸다.
#pragma once

namespace disp {

// 엔진 측 이펙트 객체 식별자. effects/ 의 묶음 구조에 맞춤:
//  - Modulation : ModulationSet (Flanger/Phaser/Chorus 를 paramId 오프셋으로 구분)
//  - ChannelStrip : Equalizer/Compressor 를 paramId 오프셋으로 구분
enum class EngineTarget : int {
    Reverb = 0,
    Delay,
    Modulation,    // Flanger/Phaser/Chorus
    ChannelStrip,  // EQ/Compressor
    Distortion,
};

class EffectParamSink {
public:
    virtual ~EffectParamSink() = default;

    // 파라미터 1개 변경. value 는 실제 단위(각 이펙트 Param 주석 기준).
    virtual void setEffectParameter(EngineTarget target, int paramId, float value) = 0;

    // 이펙트 선택(하이라이트) 변경 통지(선택). 엔진이 활성 이펙트를 알아야 할 때.
    virtual void onEffectSelected(EngineTarget target) {}
};

} // namespace disp
