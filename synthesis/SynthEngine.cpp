// SynthEngine.cpp — 엔진 구현 (신버전)
#include "SynthEngine.h"
#include "../hardware/ControlIds.h"

bool SynthEngine::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlock_   = maxBlockSize;

    for (auto& v : voices_) v.setup(sampleRate);

    lfo_.setup(sampleRate);
    lfo_.setRate(params_.lfoRateHz());
    lfo_.setDelay(params_.lfoDelaySec());   // [이은제 추가] LFO 페이드인

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

    // [이은제 2026] OctaveUp 스위치: 켜지면 전 보이스 기준음을 +12(한 옥타브) 올림.
    //   center 에 더하므로 글라이드가 걸려 부드럽게 올라간다. offset(바 위치)은 그대로.
    const int octaveShift = params_.octaveUp() ? 12 : 0;
    // [이은제 2026] QuantizeScale 스위치 상태를 현재 루트와 함께 각 보이스에 전달.
    const bool quantize = params_.quantizeOn();

    for (int i = 0; i < syn::kNumBarVoices; ++i) {
        const hw::BarTouch& b = *bars[i];

        // 기준음(center)은 항상 갱신 → 터치 중 루트가 바뀌면 글라이드로 미끄러진다.
        // 바 오프셋(offset)은 즉각 → 바를 따라 바이올린처럼 연속/등간격으로 변한다.
        voices_[i].setTargetCenter(static_cast<float>(syn::voiceCenterMidi(rootPc_, i) + octaveShift));
        voices_[i].setOffset(syn::barPosToOffsetSemis(b.pos));
        voices_[i].setQuantize(quantize, rootPc_);   // [이은제 2026] 음계 퀀타이즈

        if (b.active && !barWasActive_[i]) {
            // 상승엣지: 게이트 온 (베이스 바에만 서브 오실레이터)
            // strength(터치 면적)가 들어오면 벨로시티로, 없으면(0) 풀 볼륨.
            const float vel = (b.strength > 0.01f) ? (0.3f + 0.7f * b.strength) : 1.0f;
            voices_[i].gateOn(vel, params_, /*isBass=*/(i == syn::Bass));
            lfo_.retrigger();   // [이은제 추가] 새 음마다 LFO 페이드인 0부터 다시 시작
        } else if (!b.active && barWasActive_[i]) {
            voices_[i].gateOff();   // 하강엣지: 릴리즈
        }
        barWasActive_[i] = b.active;
    }
}

void SynthEngine::render(float* outLeft, float* outRight, unsigned int numFrames) {
    // ── 1) 보이스 합산 (모노 믹스) ──
    // [이은제 2026] LfoEnable 스위치: off 면 lfoVal 을 0으로 게이팅한다.
    //   → 비브라토·PWM·필터LFO 가 한 번에 모두 정지. phase 는 계속 돌려(process 호출)
    //     다시 켤 때 위상 점프 없이 매끄럽게 이어지도록 한다.
    const bool lfoOn = params_.lfoEnabled();
    for (unsigned int n = 0; n < numFrames; ++n) {
        float lfoVal = lfo_.process();                // 공유 LFO (-1..1)
        if (!lfoOn) lfoVal = 0.0f;                     // LFO off → 모듈레이션 정지
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
        case ControlId::LfoRate:  lfo_.setRate(params_.lfoRateHz());   break;
        case ControlId::LfoDelay: lfo_.setDelay(params_.lfoDelaySec()); break; // [이은제 추가]
        // 컷오프/레조넌스/엔벨로프/파형 등은 보이스가 매 샘플 params_ 에서 직접 읽으므로
        // 여기서 별도 push 불필요.
        default: break;
    }
}

void SynthEngine::cleanup() {
    fx_.cleanup();
}
