// SynthEngine.h — 소리 생성 총괄 (신버전: 링 5도권 + 연주바 4개 = 연속 보이스 4개)
// ───────────────────────────────────────────────────────────────────────────
// [구조 변경]
//   (구) Trill 4센서 → ChordPerformer → MusicTheory.buildChord → 보이스풀 동적할당
//   (신) Trill Ring(5도권, 이산·래치) → 루트
//        Trill Bar 4개(왼1=Bass, 오른3=5/8/3) → 각 바 = 고정 보이스 1개(연속 피치)
//
// 데이터 흐름(한 오디오 블록):
//   [TrillFrame] → applyPerformance():
//        링 터치 중이면 루트 갱신(떼면 마지막 루트 래치)
//        각 바: 가운데 기준음 + (바위치-0.5)*span  → 보이스 setTargetCenter/setOffset
//                터치 상승엣지=gateOn / 하강엣지=gateOff
//   매 샘플: 보이스 4개 합산 → (블록 끝) 스테레오 복제 → PluginChain(동적 이펙트 체인) → 마스터볼륨
//
// 파라미터 입력: 하드웨어 노브/스위치 + 디스플레이 → setParameter(id, value) 로 합류.
//   (id = hw::ControlId 정수, value = 실제 단위. 매핑은 하드웨어 파트가 끝냄)
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include <array>
#include "../core/Types.h"
#include "../hardware/ParameterSink.h"
#include "../hardware/TrillInput.h"      // hw::TrillFrame, hw::BarTouch
#include "../effects/PluginChain.h"   // [변경] 동적 체인으로 교체 (구 EffectChain 대체)
#include "SynthParams.h"
#include "Voice.h"
#include "Lfo.h"
#include "ChordVoicing.h"               // 5도권/보이싱 매핑 (구 MusicTheory 대체)

class SynthEngine : public hw::ParameterSink {
public:
    bool setup(float sampleRate, unsigned int maxBlockSize);

    // 매 블록: 연주 센서 스냅샷 → 루트 래치 + 4바 게이트/피치 반영.
    void applyPerformance(const hw::TrillFrame& frame);

    // 매 블록: 스테레오 오디오 생성 (보이스 → 이펙터 → 마스터볼륨).
    void render(float* outLeft, float* outRight, unsigned int numFrames);

    // 하드웨어/디스플레이 → 파라미터 (ParameterSink 구현).
    void setParameter(int controlId, float value) override;
    void setParam(int controlId, float value) { setParameter(controlId, value); } // 별칭

    // [변경] 동적 이펙트 체인 접근자. 디스플레이 컨트롤러가 이걸 통해 편집한다.
    //   (구 setEffectParameter(chainParamId, ...) 1000단위 라우팅은 폐기 —
    //    이제 슬롯이 고정이 아니므로 ChainUiController 가 슬롯/파라미터를 직접 지정)
    fx::PluginChain& effects() { return fx_; }

    void cleanup();

private:
    float        sampleRate_ = 44100.0f;
    unsigned int maxBlock_   = 16;

    syn::SynthParams params_;
    // 보이스 4개: [0]=Bass [1]=Fifth [2]=Octave [3]=Third  (syn::BarVoice 인덱스와 동일)
    std::array<syn::Voice, syn::kNumBarVoices> voices_;
    syn::Lfo         lfo_;
    fx::PluginChain  fx_;              // [변경] 동적 체인
    fx::SmoothedValue masterSmooth_;   // 마스터 볼륨 지퍼노이즈 방지

    // ── 연주 상태 ──
    int  rootPc_  = 0;     // 현재 래치된 5도권 루트(피치클래스). 링 터치 중 갱신, 떼면 유지.
    int  ringSeg_ = -1;    // 직전 링 세그먼트(히스테리시스용)
    bool barWasActive_[syn::kNumBarVoices] = { false, false, false, false }; // 엣지 검출
};
