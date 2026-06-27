// Chorus.cpp — 2보이스 코러스 구현
#include "Chorus.h"

namespace fx {

bool Chorus::setup(float sampleRate, unsigned int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    int maxS = (int)msToSamples(60.0f) + 8;   // 기준+변조+여유
    lineL_.setup(maxS);
    lineR_.setup(maxS);
    lfo_.setSampleRate(sampleRate);
    lfo_.setShape(dsp::Lfo::Shape::Sine);
    lpfL_.setSampleRate(sampleRate);
    lpfR_.setSampleRate(sampleRate);
    lpfL_.setCutoff(12000.0f);
    lpfR_.setCutoff(12000.0f);
    updateRate();
    reset();
    return true;
}

void Chorus::updateRate() {
    float hz = bpmSync_ ? (1.0f / dsp::noteDivSeconds((int)rateRaw_, tempo_)) : rateRaw_;
    lfo_.setRate(dsp::clampf(hz, 0.01f, 20.0f));
}

void Chorus::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_ || !inOut_) return;
    const float depthSamples = depth_ * msToSamples(5.0f);  // ±5ms 최대
    const float d1 = msToSamples(delay1Ms_);
    const float d2 = msToSamples(delay2Ms_);

    for (unsigned int n = 0; n < numFrames; ++n) {
        float mod = lfo_.next();              // -1..1
        float modR = lfo_.tap(0.25f);         // 우채널 위상차(스테레오 폭)

        // 좌: 보이스1(+mod), 우: 보이스2(반대위상) — 2보이스 + 스테레오 분산
        float dL1 = d1 + mod  * depthSamples;
        float dL2 = d2 - mod  * depthSamples;
        float dR1 = d1 + modR * depthSamples;
        float dR2 = d2 - modR * depthSamples;

        float wetL = 0.5f * (lineL_.read(dL1) + lineL_.read(dL2));
        float wetR = 0.5f * (lineR_.read(dR1) + lineR_.read(dR2));

        wetL = lpfL_.lp(wetL);
        wetR = lpfR_.lp(wetR);

        lineL_.write(left[n]  + wetL * feedback_);
        lineR_.write(right[n] + wetR * feedback_);

        left[n]  = left[n]  * (1.0f - mix_) + wetL * mix_;
        right[n] = right[n] * (1.0f - mix_) + wetR * mix_;
    }
}

void Chorus::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::InOut:    inOut_ = (value >= 0.5f); break;
        case Param::Rate:     rateRaw_ = value; updateRate(); break;
        case Param::BpmSync:  bpmSync_ = (value >= 0.5f); updateRate(); break;
        case Param::Delay1Ms: delay1Ms_ = value; break;
        case Param::Delay2Ms: delay2Ms_ = value; break;
        case Param::Depth:    depth_ = dsp::clampf(value, 0.0f, 1.0f); break;
        case Param::Feedback: feedback_ = dsp::clampf(value, 0.0f, 0.9f); break;
        case Param::Lpf:      lpfL_.setCutoff(value); lpfR_.setCutoff(value); break;
        case Param::Mix:      mix_ = dsp::clampf(value, 0.0f, 1.0f); break;
        default: break;
    }
}

void Chorus::reset() {
    lineL_.reset(); lineR_.reset();
    lpfL_.reset();  lpfR_.reset();
    fbStateL_ = fbStateR_ = 0.0f;
    lfo_.reset(0.0f);
}

} // namespace fx
