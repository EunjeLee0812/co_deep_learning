// Flanger.cpp — 변조 딜레이 플랜저 구현
#include "Flanger.h"

namespace fx {

// 기준/변조 딜레이 범위(ms). 플랜저는 1ms 안팎의 짧은 딜레이.
static constexpr float kBaseMs  = 1.0f;   // 최소 딜레이
static constexpr float kSweepMs = 4.0f;   // 변조로 더해지는 최대 범위

bool Flanger::setup(float sampleRate, unsigned int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    int maxS = (int)msToSamples(kBaseMs + kSweepMs + 1.0f) + 8;
    lineL_.setup(maxS);
    lineR_.setup(maxS);
    lfo_.setSampleRate(sampleRate);
    lfo_.setShape(dsp::Lfo::Shape::Triangle);  // 플랜저는 삼각이 고전적
    updateRate();
    reset();
    return true;
}

void Flanger::updateRate() {
    float hz = bpmSync_ ? (1.0f / dsp::noteDivSeconds((int)rateRaw_, tempo_)) : rateRaw_;
    lfo_.setRate(dsp::clampf(hz, 0.01f, 20.0f));
}

void Flanger::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_ || !inOut_) return;
    const float baseS  = msToSamples(kBaseMs);
    const float sweepS = msToSamples(kSweepMs) * depth_;

    for (unsigned int n = 0; n < numFrames; ++n) {
        // LFO 0..1 로 변환해 딜레이 변조(항상 양수 딜레이 유지)
        float mL = (lfo_.next() * 0.5f + 0.5f);
        float mR = (lfo_.tap(phase_) * 0.5f + 0.5f);
        float dL = baseS + mL * sweepS;
        float dR = baseS + mR * sweepS;

        float wetL = lineL_.read(dL);
        float wetR = lineR_.read(dR);

        lineL_.write(left[n]  + wetL * feedback_);
        lineR_.write(right[n] + wetR * feedback_);

        left[n]  = left[n]  * (1.0f - mix_) + wetL * mix_;
        right[n] = right[n] * (1.0f - mix_) + wetR * mix_;
    }
}

void Flanger::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::InOut:    inOut_ = (value >= 0.5f); break;
        case Param::Rate:     rateRaw_ = value; updateRate(); break;
        case Param::BpmSync:  bpmSync_ = (value >= 0.5f); updateRate(); break;
        case Param::Depth:    depth_ = dsp::clampf(value, 0.0f, 1.0f); break;
        case Param::Feedback: feedback_ = dsp::clampf(value, 0.0f, 0.95f); break;
        case Param::Phase:    phase_ = dsp::clampf(value, 0.0f, 1.0f); break;
        case Param::Mix:      mix_ = dsp::clampf(value, 0.0f, 1.0f); break;
        default: break;
    }
}

void Flanger::reset() {
    lineL_.reset(); lineR_.reset();
    fbL_ = fbR_ = 0.0f;
    lfo_.reset(0.0f);
}

} // namespace fx
