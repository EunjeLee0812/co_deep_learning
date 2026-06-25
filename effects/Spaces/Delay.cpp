// Delay.cpp — 스테레오 딜레이 구현
#include "Spaces/Delay.h"

namespace fx {

static constexpr float kMaxDelayMs = 4000.0f;

bool Delay::setup(float sampleRate, unsigned int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    int maxSamples = (int)(kMaxDelayMs * 0.001f * sampleRate) + 8;

    for (Channel* c : { &left_, &right_ }) {
        c->line.setup(maxSamples);
        c->fbLowCut.setSampleRate(sampleRate);
        c->fbHighCut.setSampleRate(sampleRate);
        c->fbLowCut.setCutoff(120.0f);     // HP: 120Hz 이하 컷
        c->fbHighCut.setCutoff(8000.0f);   // LP: 8kHz 이상 컷
        c->delaySamples.setup(sampleRate, 60.0f); // 시간 변경 시 60ms 램프(글리치 방지)
    }
    left_.delaySamples.snap(msToSamples(333.0f));
    right_.delaySamples.snap(msToSamples(333.0f));

    inGain_.setup(sampleRate, 20.0f);
    outGain_.setup(sampleRate, 20.0f);
    mix_.setup(sampleRate, 20.0f);
    inGain_.snap(1.0f);
    outGain_.snap(1.0f);
    mix_.snap(0.3f);

    recalcTimes();
    reset();
    return true;
}

// sync/link/tempo 를 반영해 좌우 딜레이 목표 샘플수를 갱신.
void Delay::recalcTimes() {
    float lMs, rMs;
    if (bpmSync_) {
        lMs = dsp::noteDivSeconds(left_.noteDiv,  tempoBpm_) * 1000.0f;
        rMs = dsp::noteDivSeconds(right_.noteDiv, tempoBpm_) * 1000.0f;
    } else {
        lMs = left_.timeMs;
        rMs = right_.timeMs;
    }
    if (link_) rMs = lMs;   // 링크 시 우=좌

    lMs = dsp::clampf(lMs, 1.0f, kMaxDelayMs);
    rMs = dsp::clampf(rMs, 1.0f, kMaxDelayMs);
    left_.delaySamples.setTarget(msToSamples(lMs));
    right_.delaySamples.setTarget(msToSamples(rMs));
}

void Delay::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    for (unsigned int n = 0; n < numFrames; ++n) {
        const float g   = inGain_.next();
        const float mix = mix_.next();
        const float og  = outGain_.next();

        float inL = left[n]  * g;
        float inR = right[n] * g;

        float dL = left_.delaySamples.next();
        float dR = right_.delaySamples.next();

        float wetL = left_.line.read(dL);
        float wetR = right_.line.read(dR);

        if (mode_ == Mode::PingPong) {
            // 교차 피드백: 좌 출력이 우 입력으로, 우 출력이 좌 입력으로 → 바운스.
            float fbToLeft  = right_.fbHighCut.lp(right_.fbLowCut.hp(wetR)) * feedback_;
            float fbToRight = left_.fbHighCut.lp(left_.fbLowCut.hp(wetL))  * feedback_;
            left_.line.write(inL + fbToLeft);
            right_.line.write(inR + fbToRight);
        } else {
            // 일반: 각 채널 자기 피드백(필터 통과).
            float fbL = left_.fbHighCut.lp(left_.fbLowCut.hp(wetL))   * feedback_;
            float fbR = right_.fbHighCut.lp(right_.fbLowCut.hp(wetR)) * feedback_;
            left_.line.write(inL + fbL);
            right_.line.write(inR + fbR);
        }

        left[n]  = (left[n]  * (1.0f - mix) + wetL * mix) * og;
        right[n] = (right[n] * (1.0f - mix) + wetR * mix) * og;
    }
}

void Delay::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::Gain:        inGain_.setTarget(dsp::dbToLin(value)); break;
        case Param::Out:         outGain_.setTarget(dsp::dbToLin(value)); break;
        case Param::Feedback:    feedback_ = dsp::clampf(value, 0.0f, 0.98f); break;
        case Param::TempoBpm:    tempoBpm_ = value; recalcTimes(); break;
        case Param::BpmSync:     bpmSync_ = (value >= 0.5f); recalcTimes(); break;
        case Param::Link:        link_ = (value >= 0.5f); recalcTimes(); break;
        case Param::Mode:        mode_ = (value >= 0.5f) ? Mode::PingPong : Mode::Normal; break;
        case Param::HighCut:     left_.fbHighCut.setCutoff(value); right_.fbHighCut.setCutoff(value); break;
        case Param::LowCut:      left_.fbLowCut.setCutoff(value);  right_.fbLowCut.setCutoff(value);  break;
        case Param::Mix:         mix_.setTarget(dsp::clampf(value, 0.0f, 1.0f)); break;
        case Param::LeftTimeMs:  left_.timeMs  = value; recalcTimes(); break;
        case Param::LeftDiv:     left_.noteDiv = (int)value; recalcTimes(); break;
        case Param::RightTimeMs: right_.timeMs = value; recalcTimes(); break;
        case Param::RightDiv:    right_.noteDiv= (int)value; recalcTimes(); break;
        default: break;
    }
}

void Delay::reset() {
    left_.line.reset();  right_.line.reset();
    left_.fbLowCut.reset();  left_.fbHighCut.reset();
    right_.fbLowCut.reset(); right_.fbHighCut.reset();
}

void Delay::cleanup() {}

} // namespace fx
