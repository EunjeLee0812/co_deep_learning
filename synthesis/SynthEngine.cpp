// SynthEngine.cpp — 엔진 구현 (신버전)
#include "SynthEngine.h"
#include "../hardware/ControlIds.h"

bool SynthEngine::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlock_   = maxBlockSize;

    for (auto& v : voices_) v.setup(sampleRate);

    lfo_.setup(sampleRate);
    lfo_.setRate(params_.lfoRateHz());

    masterSmooth_.setup(sampleRate, 20.0f);    // 20ms 램프
    masterSmooth_.snap(params_.masterVolume());

    if (!fx_.setup(sampleRate, maxBlockSize)) return false;
    return true;
}

void SynthEngine::applyPerformance(const hw::TrillFrame& f) {
    // ── 1) 링(5도권): 터치 중이면 루트 갱신, 떼면 마지막 값 유지(래치) ──
    //   "터치하고 떼면 그 코드를 계속 저장" + "슬라이드하면 즉시 바뀜" 동시 충족.
    if (f.ringActive) {
        ringSeg_ = syn::ringToSegment(f.ringPos, ringSeg_);  // 이산 + 히스테리시스
        rootPc_  = syn::segmentToRootPc(ringSeg_);
    }
    // f.ringActive == false → rootPc_ 그대로 유지 (래치)

    // ── 2) 4개 바 → 4개 보이스 (연속 피치 + 게이트) ──
    const hw::BarTouch* bars[syn::kNumBarVoices] = { &f.bass, &f.r5, &f.r8, &f.r3 };
    for (int i = 0; i < syn::kNumBarVoices; ++i) {
        const hw::BarTouch& b = *bars[i];

        // 기준음(center)은 항상 갱신 → 터치 중 루트가 바뀌면 글라이드로 미끄러진다.
        // 바 오프셋(offset)은 즉각 → 바를 따라 바이올린처럼 연속/등간격으로 변한다.
        voices_[i].setTargetCenter(static_cast<float>(syn::voiceCenterMidi(rootPc_, i)));
        voices_[i].setOffset(syn::barPosToOffsetSemis(b.pos));

        if (b.active && !barWasActive_[i]) {
            // 상승엣지: 게이트 온 (베이스 바에만 서브 오실레이터)
            // strength(터치 면적)가 들어오면 벨로시티로, 없으면(0) 풀 볼륨.
            const float vel = (b.strength > 0.01f) ? (0.3f + 0.7f * b.strength) : 1.0f;
            voices_[i].gateOn(vel, params_, /*isBass=*/(i == syn::Bass));
        } else if (!b.active && barWasActive_[i]) {
            voices_[i].gateOff();   // 하강엣지: 릴리즈
        }
        barWasActive_[i] = b.active;
    }
}

void SynthEngine::render(float* outLeft, float* outRight, unsigned int numFrames) {
    // ── 1) 보이스 합산 (모노 믹스) ──
    for (unsigned int n = 0; n < numFrames; ++n) {
        const float lfoVal = lfo_.process();          // 공유 LFO (-1..1)
        float mix = 0.0f;
        for (auto& v : voices_)
            if (v.isActive()) mix += v.process(params_, lfoVal);

        mix *= 0.30f;                                  // 4보이스 합산 헤드룸

        outLeft[n]  = mix;
        outRight[n] = mix;
    }

    // ── 2) 이펙터 체인 (스테레오, 블록 단위) ──
    fx_.process(outLeft, outRight, numFrames);

    // ── 3) 마스터 볼륨 (FX 뒤, 지퍼노이즈 방지 스무딩) ──
    masterSmooth_.setTarget(params_.masterVolume());
    for (unsigned int n = 0; n < numFrames; ++n) {
        const float g = masterSmooth_.next();
        outLeft[n]  *= g;
        outRight[n] *= g;
    }
}

void SynthEngine::setParameter(int controlId, float value) {
    params_.setFromControl(controlId, value);   // 실제 단위 저장

    using hw::ControlId;
    switch (static_cast<ControlId>(controlId)) {
        case ControlId::LfoRate: lfo_.setRate(params_.lfoRateHz()); break;
        // 컷오프/레조넌스/엔벨로프/파형 등은 보이스가 매 샘플 params_ 에서 직접 읽으므로
        // 여기서 별도 push 불필요.
        default: break;
    }
}

void SynthEngine::cleanup() {
    fx_.cleanup();
}
