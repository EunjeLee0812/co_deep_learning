// SynthEngine.h — 소리 생성 총괄: 연주로직 + 보이스풀 + LFO + 이펙터 체인 + 믹스
//
// 데이터 흐름(한 오디오 블록):
//   [Trill 센서] → ChordPerformer → (Trigger/Release) → 보이스 풀 할당
//   매 샘플: 보이스들 합산 → (블록 끝) 스테레오 복제 → EffectChain → 마스터 볼륨
//
// 파라미터 입력:
//   하드웨어 노브/스위치, 디스플레이 둘 다 ParameterSink::setParameter(id, value) 로 합류.
//   (id = hw::ControlId 정수값, value = 실제 단위. 매핑은 하드웨어 파트가 이미 끝냄)
#pragma once
#include <array>
#include "../core/Types.h"
#include "../hardware/ParameterSink.h"
#include "../hardware/TrillInput.h"      // hw::TrillFrame
#include "../effects/EffectChain.h"
#include "SynthParams.h"
#include "Voice.h"
#include "Lfo.h"
#include "ChordPerformer.h"

class SynthEngine : public hw::ParameterSink {
public:
    bool setup(float sampleRate, unsigned int maxBlockSize);

    // 매 블록: 연주 센서 스냅샷을 받아 코드 트리거/해제를 반영.
    void applyPerformance(const hw::TrillFrame& frame);

    // 매 블록: 스테레오 오디오 생성 (보이스 → 이펙터 → 마스터볼륨).
    void render(float* outLeft, float* outRight, unsigned int numFrames);

    // 하드웨어/디스플레이 → 파라미터 (ParameterSink 구현).
    void setParameter(int controlId, float value) override;
    // CommHandler 가 부르던 이름 호환용 별칭.
    void setParam(int controlId, float value) { setParameter(controlId, value); }

    // 이펙트 체인 파라미터(체인 ID 규약, EffectChain.h 참고) 전달용.
    void setEffectParameter(int chainParamId, float value) {
        fx_.setParameter(chainParamId, value);
    }
    fx::EffectChain& effects() { return fx_; }

    void cleanup();

private:
    // 코드 노트들에 보이스를 배정(부족하면 가장 오래된 것 스틸).
    void triggerChord(const syn::Chord& c, float velocity);
    void releaseAll();
    syn::Voice* allocVoice();

    float sampleRate_   = 44100.0f;
    unsigned int maxBlock_ = 16;

    syn::SynthParams params_;
    std::array<syn::Voice, core::kMaxVoices> voices_;
    syn::Lfo            lfo_;
    syn::ChordPerformer performer_;
    fx::EffectChain     fx_;

    fx::SmoothedValue   masterSmooth_;  // 마스터 볼륨 지퍼노이즈 방지
    unsigned long       age_ = 0;       // 보이스 스틸용 타임스탬프
    std::array<unsigned long, core::kMaxVoices> voiceAge_{};
};
