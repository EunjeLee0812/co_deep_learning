// Phaser.cpp — N단 allpass 페이저 구현
#include "Modulation/Phaser.h"

namespace fx {

bool Phaser::setup(float sampleRate, unsigned int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    lfo_.setSampleRate(sampleRate);
    lfo_.setShape(dsp::Lfo::Shape::Sine);
    updateRate();
    reset();
    return true;
}

void Phaser::updateRate() {
    float hz = bpmSync_ ? (1.0f / dsp::noteDivSeconds((int)rateRaw_, tempo_)) : rateRaw_;
    lfo_.setRate(dsp::clampf(hz, 0.01f, 20.0f));
}

void Phaser::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_ || !inOut_) return;
    for (unsigned int n = 0; n < numFrames; ++n) {
        float lfoL = lfo_.next();           // -1..1
        float lfoR = lfo_.tap(phase_);      // 좌우 위상차

        // 중심주파수에서 ±depth 옥타브 스윕 → allpass 계수.
        float fcL = centerHz_ * std::pow(2.0f, lfoL * depth_ * 2.0f);
        float fcR = centerHz_ * std::pow(2.0f, lfoR * depth_ * 2.0f);
        float aL = coeffForFreq(fcL);
        float aR = coeffForFreq(fcR);
        for (int s = 0; s < kStages; ++s) { apL_[s].a = aL; apR_[s].a = aR; }

        // 입력 + 피드백 → allpass 체인
        float xL = left[n]  + fbL_ * feedback_;
        float xR = right[n] + fbR_ * feedback_;
        for (int s = 0; s < kStages; ++s) xL = apL_[s].process(xL);
        for (int s = 0; s < kStages; ++s) xR = apR_[s].process(xR);
        fbL_ = xL; fbR_ = xR;

        // 노치: 원음 + allpass출력 (위상상쇄로 노치 생성)
        float wetL = 0.5f * (left[n]  + xL);
        float wetR = 0.5f * (right[n] + xR);

        left[n]  = left[n]  * (1.0f - mix_) + wetL * mix_;
        right[n] = right[n] * (1.0f - mix_) + wetR * mix_;
    }
}

void Phaser::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::InOut:    inOut_ = (value >= 0.5f); break;
        case Param::Rate:     rateRaw_ = value; updateRate(); break;
        case Param::BpmSync:  bpmSync_ = (value >= 0.5f); updateRate(); break;
        case Param::Depth:    depth_ = dsp::clampf(value, 0.0f, 1.0f); break;
        case Param::Freq:     centerHz_ = value; break;
        case Param::Feedback: feedback_ = dsp::clampf(value, 0.0f, 0.95f); break;
        case Param::Phase:    phase_ = dsp::clampf(value, 0.0f, 1.0f); break;
        case Param::Mix:      mix_ = dsp::clampf(value, 0.0f, 1.0f); break;
        default: break;
    }
}

void Phaser::reset() {
    for (int s = 0; s < kStages; ++s) { apL_[s].reset(); apR_[s].reset(); }
    fbL_ = fbR_ = 0.0f;
    lfo_.reset(0.0f);
}

} // namespace fx
