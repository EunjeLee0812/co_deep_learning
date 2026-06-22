// SynthEngine.cpp — 엔진 구현
#include "SynthEngine.h"
#include "../hardware/ControlIds.h"

bool SynthEngine::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlock_   = maxBlockSize;

    for (auto& v : voices_) v.setup(sampleRate);
    voiceAge_.fill(0);

    lfo_.setup(sampleRate);
    lfo_.setRate(params_.lfoRateHz());

    performer_.reset();

    masterSmooth_.setup(sampleRate, 20.0f);   // 20ms 램프
    masterSmooth_.snap(params_.masterVolume());

    if (!fx_.setup(sampleRate, maxBlockSize)) return false;
    return true;
}

void SynthEngine::applyPerformance(const hw::TrillFrame& frame) {
    syn::Chord chord;
    float velocity = 1.0f;
    switch (performer_.poll(frame, chord, velocity)) {
        case syn::ChordPerformer::Action::Trigger: triggerChord(chord, velocity); break;
        case syn::ChordPerformer::Action::Release: releaseAll();                  break;
        case syn::ChordPerformer::Action::None:    default:                       break;
    }
}

syn::Voice* SynthEngine::allocVoice() {
    // 1) 비활성 보이스 우선
    for (size_t i = 0; i < voices_.size(); ++i)
        if (!voices_[i].isActive()) { voiceAge_[i] = ++age_; return &voices_[i]; }
    // 2) 전부 활성 → 가장 오래된 것 스틸
    size_t oldest = 0;
    for (size_t i = 1; i < voices_.size(); ++i)
        if (voiceAge_[i] < voiceAge_[oldest]) oldest = i;
    voiceAge_[oldest] = ++age_;
    return &voices_[oldest];
}

void SynthEngine::triggerChord(const syn::Chord& c, float velocity) {
    for (int i = 0; i < c.count; ++i) {
        syn::Voice* v = allocVoice();
        const bool isBass = (i == 0); // 최저음에만 서브 오실레이터
        v->noteOn(c.notes[i], velocity, params_, isBass);
    }
}

void SynthEngine::releaseAll() {
    for (auto& v : voices_) v.gateOff();
}

void SynthEngine::render(float* outLeft, float* outRight, unsigned int numFrames) {
    // ── 1) 보이스 합산 (모노 믹스) ──
    for (unsigned int n = 0; n < numFrames; ++n) {
        const float lfoVal = lfo_.process();   // 공유 LFO (-1..1)
        float mix = 0.0f;
        for (auto& v : voices_)
            if (v.isActive()) mix += v.process(params_, lfoVal);

        // 동시 발음 헤드룸(코드라 여러 보이스 합산됨)
        mix *= 0.25f;

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
    // 패널 파라미터 저장(실제 단위). 즉시 반영이 필요한 것만 추가 처리.
    params_.setFromControl(controlId, value);

    using hw::ControlId;
    switch (static_cast<ControlId>(controlId)) {
        case ControlId::LfoRate: lfo_.setRate(params_.lfoRateHz()); break;
        // 컷오프/레조넌스/엔벨로프 등은 보이스가 매 샘플 params_ 에서 직접 읽으므로
        // 여기서 따로 push 할 필요 없음.
        default: break;
    }
}

void SynthEngine::cleanup() {
    fx_.cleanup();
}
