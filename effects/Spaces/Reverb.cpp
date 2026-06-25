// Reverb.cpp — 플레이트/홀 리버브 구현 (Freeverb 계열 + 스핀 변조)
#include "Reverb.h"
#include <algorithm>

namespace fx {

// 44.1kHz 기준 Freeverb 튜닝값. 실제 샘플레이트로 스케일해 사용.
static const int kCombTuning[8]    = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
static const int kAllpassTuning[4] = { 556, 441, 341, 225 };
static constexpr int kStereoSpread = 23;   // 우채널 딜레이 오프셋(스테레오 디코릴레이션)

bool Reverb::setup(float sampleRate, unsigned int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    const float scale = sampleRate / 44100.0f;

    // 딜레이라인 할당. size 최대(1.5배) + 스핀 변조 여유분까지 고려해 넉넉히.
    const float maxLenMul = 1.6f;
    for (int i = 0; i < kNumCombs; ++i) {
        int maxLen = (int)(kCombTuning[i] * scale * maxLenMul) + kStereoSpread + 8;
        combL_[i].line.setup(maxLen);
        combR_[i].line.setup(maxLen);
    }
    for (int i = 0; i < kNumAllpass; ++i) {
        int maxLen = (int)(kAllpassTuning[i] * scale * maxLenMul) + kStereoSpread + 64;
        apL_[i].line.setup(maxLen);
        apR_[i].line.setup(maxLen);
    }

    lowCutL_.setSampleRate(sampleRate);
    lowCutR_.setSampleRate(sampleRate);
    lowCutL_.setCutoff(80.0f);
    lowCutR_.setCutoff(80.0f);

    spinLfo_.setSampleRate(sampleRate);
    spinLfo_.setShape(dsp::Lfo::Shape::Sine);
    spinLfo_.setRate(spinHz_);

    inGain_.setup(sampleRate, 20.0f);
    outGain_.setup(sampleRate, 20.0f);
    mix_.setup(sampleRate, 20.0f);
    inGain_.snap(1.0f);
    outGain_.snap(1.0f);
    mix_.snap(0.3f);

    recalcCore();
    reset();
    return true;
}

// size/decay/type/highCut → 코어 딜레이길이·피드백·댐핑 재계산.
void Reverb::recalcCore() {
    const float scale = sampleRate_ / 44100.0f;
    const float typeMul = (type_ == Type::Plate) ? 0.7f : 1.3f; // 플레이트=촘촘/짧게, 홀=길게
    const float sizeMul = 0.6f + size_ * 0.9f;                  // 0.6 .. 1.5
    const float lenMul  = scale * typeMul * sizeMul;

    for (int i = 0; i < kNumCombs; ++i) {
        combL_[i].baseDelay = kCombTuning[i] * lenMul;
        combR_[i].baseDelay = (kCombTuning[i] + kStereoSpread) * lenMul;
    }
    for (int i = 0; i < kNumAllpass; ++i) {
        apL_[i].baseDelay = kAllpassTuning[i] * lenMul;
        apR_[i].baseDelay = (kAllpassTuning[i] + kStereoSpread) * lenMul;
    }

    // 피드백(잔향 길이) : plate 는 살짝 짧게, hall 은 길게.
    float decayBase = (type_ == Type::Plate) ? 0.70f : 0.74f;
    feedback_ = decayBase + decay_ * 0.265f;          // ~0.70 .. ~0.985
    feedback_ = dsp::clampf(feedback_, 0.0f, 0.985f);  // 자기발진 방지

    // 댐핑 = 피드백 경로 lowpass 코너(=HighCut). exp(-2π fc/fs).
    damp_ = std::exp(-dsp::kTwoPi * highCutHz_ / sampleRate_);
    damp_ = dsp::clampf(damp_, 0.0f, 0.95f);

    spinSamples_ = spinDepth_ * 6.0f * scale; // 변조 깊이(최대 ~6샘플)
}

void Reverb::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    for (unsigned int n = 0; n < numFrames; ++n) {
        const float g   = inGain_.next();
        const float mix = mix_.next();
        const float og  = outGain_.next();

        float dryL = left[n];
        float dryR = right[n];

        // 리버브 send : 입력게인 → 모노합(콤은 모노 send 가 자연스러움) → LowCut HP
        float sendL = lowCutL_.hp(dryL * g);
        float sendR = lowCutR_.hp(dryR * g);
        float send  = (sendL + sendR) * 0.5f;

        // 스핀 LFO (좌우 다른 위상으로 변조)
        float lfo  = spinLfo_.next();
        float modL = lfo * spinSamples_;
        float modR = spinLfo_.tap(0.25f) * spinSamples_;

        // 콤필터 병렬 합산
        float accL = 0.0f, accR = 0.0f;
        for (int i = 0; i < kNumCombs; ++i) {
            accL += combL_[i].process(send, feedback_, damp_);
            accR += combR_[i].process(send, feedback_, damp_);
        }
        accL *= (1.0f / kNumCombs);
        accR *= (1.0f / kNumCombs);

        // 앨패스 직렬(확산) + 스핀 변조
        for (int i = 0; i < kNumAllpass; ++i) {
            accL = apL_[i].process(accL, modL);
            accR = apR_[i].process(accR, modR);
        }

        float wetL = accL;
        float wetR = accR;

        left[n]  = (dryL * (1.0f - mix) + wetL * mix) * og;
        right[n] = (dryR * (1.0f - mix) + wetR * mix) * og;
    }
}

void Reverb::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::Gain:      inGain_.setTarget(dsp::dbToLin(value)); break;
        case Param::Out:       outGain_.setTarget(dsp::dbToLin(value)); break;
        case Param::Type:      type_ = (value >= 0.5f) ? Type::Hall : Type::Plate; recalcCore(); break;
        case Param::Size:      size_  = dsp::clampf(value, 0.0f, 1.0f); recalcCore(); break;
        case Param::Decay:     decay_ = dsp::clampf(value, 0.0f, 1.0f); recalcCore(); break;
        case Param::LowCut:    lowCutL_.setCutoff(value); lowCutR_.setCutoff(value); break;
        case Param::HighCut:   highCutHz_ = value; recalcCore(); break;
        case Param::Spin:      spinHz_ = value; spinLfo_.setRate(spinHz_); break;
        case Param::SpinDepth: spinDepth_ = dsp::clampf(value, 0.0f, 1.0f); recalcCore(); break;
        case Param::Mix:       mix_.setTarget(dsp::clampf(value, 0.0f, 1.0f)); break;
        default: break;
    }
}

void Reverb::reset() {
    for (int i = 0; i < kNumCombs; ++i)   { combL_[i].line.reset(); combL_[i].store = 0.0f;
                                            combR_[i].line.reset(); combR_[i].store = 0.0f; }
    for (int i = 0; i < kNumAllpass; ++i) { apL_[i].line.reset(); apR_[i].line.reset(); }
    lowCutL_.reset(); lowCutR_.reset();
    spinLfo_.reset(0.0f);
}

void Reverb::cleanup() {}

} // namespace fx
