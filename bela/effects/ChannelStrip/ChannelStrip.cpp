#include "ChannelStrip.h"

namespace fx {

bool ChannelStrip::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    outputGain_.setup(sampleRate, 20.0f);
    outputGain_.snap(1.0f);
    bool ok = eq_.setup(sampleRate, maxBlockSize) && comp_.setup(sampleRate, maxBlockSize);
    return ok;
}

void ChannelStrip::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;

    // TODO: EQ 가 HP>SC 면 사이드체인 신호를 뽑아 comp_.setSidechain(...) 로 넘길 것

    if (order_ == Order::EqThenComp) {
        eq_.process(left, right, numFrames);
        comp_.process(left, right, numFrames);
    } else {
        comp_.process(left, right, numFrames);
        eq_.process(left, right, numFrames);
    }

    applyDrive(left, right, numFrames);

    for (unsigned int n = 0; n < numFrames; ++n) {
        const float g = outputGain_.next();
        left[n]  *= g;
        right[n] *= g;
    }
}

void ChannelStrip::applyDrive(float* l, float* r, unsigned int n) {
    if (drive_ <= 0.0f) return;
    // TODO(구현자): tanh 등 비선형 새츄레이션. drive_ 로 강도 조절.
    (void)l; (void)r; (void)n;
}

void ChannelStrip::setParameter(int paramId, float value) {
    if (paramId >= kStripBase) {
        switch (static_cast<StripParam>(paramId - kStripBase)) {
            case StripParam::Drive:         drive_ = value; break;
            case StripParam::OutputLevelDb: outputGain_.setTarget(std::pow(10.0f, value / 20.0f)); break;
            case StripParam::RoutingOrder:  order_ = static_cast<Order>((int)value); break;
        }
        return;
    }
    if (paramId >= kCompBase) comp_.setParameter(paramId - kCompBase, value);
    else                      eq_.setParameter(paramId - kEqBase, value);
}

void ChannelStrip::reset()   { eq_.reset(); comp_.reset(); }
void ChannelStrip::cleanup() { eq_.cleanup(); comp_.cleanup(); }

} // namespace fx
