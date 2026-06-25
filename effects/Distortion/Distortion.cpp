// Distortion.cpp — 디스토션 구현
#include "Distortion/Distortion.h"

namespace fx {

bool Distortion::setup(float sampleRate, unsigned int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    svfL_.setSampleRate(sampleRate); svfR_.setSampleRate(sampleRate);
    svfL_.setCutoff(2000.0f); svfR_.setCutoff(2000.0f);
    svfL_.setResonance(0.2f); svfR_.setResonance(0.2f);
    dcL_.setSampleRate(sampleRate); dcR_.setSampleRate(sampleRate);
    dcL_.setCutoff(15.0f); dcR_.setCutoff(15.0f);   // DC 블로커 코너

    inGain_.setup(sampleRate, 20.0f);
    outGain_.setup(sampleRate, 20.0f);
    mix_.setup(sampleRate, 20.0f);
    drive_.setup(sampleRate, 20.0f);
    inGain_.snap(1.0f);
    outGain_.snap(1.0f);
    mix_.snap(1.0f);
    drive_.snap(0.0f);
    reset();
    return true;
}

// 파형 정형. driveGain 은 process 에서 곱해 들어온 상태.
inline float Distortion::shape(float x) const {
    switch (type_) {
        case Type::HardClip:
            return dsp::clampf(x, -1.0f, 1.0f);
        case Type::SoftClip:
            return std::tanh(x);
        case Type::Tube:
        default: {
            // 비대칭 새츄레이션 → 짝수 배음(진공관 느낌). 출력 DC 는 process 에서 차단.
            float bias = 0.18f;
            float y = std::tanh(x + bias);
            return y;
        }
    }
}

// 모핑 SVF: morph 0=HP, 0.5=BP, 1=LP 연속 블렌딩.
inline float Distortion::morphFilter(dsp::StateVariableFilter& svf, float x) const {
    float lp, bp, hp;
    svf.process(x, lp, bp, hp);
    if (eqMorph_ < 0.5f) {
        float t = eqMorph_ * 2.0f;        // 0..1
        return hp * (1.0f - t) + bp * t;  // HP → BP
    } else {
        float t = (eqMorph_ - 0.5f) * 2.0f;
        return bp * (1.0f - t) + lp * t;  // BP → LP
    }
}

void Distortion::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    for (unsigned int n = 0; n < numFrames; ++n) {
        const float g    = inGain_.next();
        const float mix  = mix_.next();
        const float og   = outGain_.next();
        const float drv  = 1.0f + drive_.next() * 24.0f;  // 1..25배

        float dryL = left[n]  * g;
        float dryR = right[n] * g;

        float wL = dryL, wR = dryR;

        if (eqPos_ == EqPos::Pre) { wL = morphFilter(svfL_, wL); wR = morphFilter(svfR_, wR); }

        wL = shape(wL * drv);
        wR = shape(wR * drv);

        if (eqPos_ == EqPos::Post) { wL = morphFilter(svfL_, wL); wR = morphFilter(svfR_, wR); }

        // tube 비대칭으로 생긴 DC 제거
        if (type_ == Type::Tube) { wL = dcL_.hp(wL); wR = dcR_.hp(wR); }

        left[n]  = (dryL * (1.0f - mix) + wL * mix) * og;
        right[n] = (dryR * (1.0f - mix) + wR * mix) * og;
    }
}

void Distortion::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::Gain:    inGain_.setTarget(dsp::dbToLin(value)); break;
        case Param::Out:     outGain_.setTarget(dsp::dbToLin(value)); break;
        case Param::Type:    type_ = static_cast<Type>((int)dsp::clampf(value, 0.0f, 2.0f)); break;
        case Param::EqPos:   eqPos_ = static_cast<EqPos>((int)dsp::clampf(value, 0.0f, 2.0f)); break;
        case Param::EqMorph: eqMorph_ = dsp::clampf(value, 0.0f, 1.0f); break;
        case Param::EqFreq:  svfL_.setCutoff(value); svfR_.setCutoff(value); break;
        case Param::EqReso:  svfL_.setResonance(value); svfR_.setResonance(value); break;
        case Param::Drive:   drive_.setTarget(dsp::clampf(value, 0.0f, 1.0f)); break;
        case Param::Mix:     mix_.setTarget(dsp::clampf(value, 0.0f, 1.0f)); break;
        default: break;
    }
}

void Distortion::reset() {
    svfL_.reset(); svfR_.reset();
    dcL_.reset();  dcR_.reset();
}

} // namespace fx
