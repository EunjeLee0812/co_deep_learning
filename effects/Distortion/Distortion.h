// Distortion.h — 디스토션 (스펙/레퍼런스 미정 → 표준 파라미터로 구성)
// TODO(사용자): 원하는 디스토션 타입/파라미터 알려주면 여기에 맞춰 수정할게.
#pragma once
#include "../Effect.h"

namespace fx {

class Distortion : public Effect {
public:
    enum class Type : int { SoftClip = 0, HardClip = 1, Tube = 2, Fuzz = 3, BitCrush = 4 };

    enum class Param : int {
        Drive       = 0,  // 0 .. 1 (또는 dB) 게인/왜곡량
        ToneHz      = 1,  // 출력 톤(로우패스/틸트) 컷오프
        OutputDb    = 2,  // -24 .. +6
        Mix         = 3,  // 0(dry) .. 1(wet)
        Mode        = 4,  // Type enum 값
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

private:
    float drive_   = 0.0f;
    float toneHz_  = 8000.0f;
    float outDb_   = 0.0f;
    float mix_     = 1.0f;
    Type  mode_    = Type::SoftClip;
    // TODO(구현자): mode_ 별 파형 정형 함수 + 톤 필터 + dry/wet.
    //   업샘플링(오버샘플) 고려 — 디스토션은 앨리어싱 잘 생김.
};

} // namespace fx
