// ChannelStrip.cpp — EQ + 컴프 + Gain/Out 묶음
#include "ChannelStrip.h"

namespace fx {

bool ChannelStrip::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    bool ok = true;
    ok &= eq_.setup(sampleRate, maxBlockSize);
    ok &= comp_.setup(sampleRate, maxBlockSize);
    // 내부 서브이펙트는 항상 켜둔다(스트립 자체 bypass 로 전체 우회).
    eq_.setBypass(false);
    comp_.setBypass(false);
    inGain_.setup(sampleRate, 20.0f);
    outGain_.setup(sampleRate, 20.0f);
    inGain_.snap(1.0f);
    outGain_.snap(1.0f);
    reset();
    return ok;
}

void ChannelStrip::runEq(float* l, float* r, unsigned int n) {
    eq_.process(l, r, n);
}

void ChannelStrip::runComp(float* l, float* r, unsigned int n, bool useEqSidechain) {
    // EQ 가 HP>SC 모드이고 EQ 가 컴프 앞단일 때만 사이드체인 주입.
    if (useEqSidechain && eq_.hpRoute() == Equalizer::HpRoute::ToSidechain)
        comp_.setSidechain(eq_.sidechainL(), eq_.sidechainR(), n);
    comp_.process(l, r, n);
}

void ChannelStrip::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;

    // 입력 게인
    for (unsigned int n = 0; n < numFrames; ++n) {
        float g = inGain_.next();
        left[n]  *= g;
        right[n] *= g;
    }

    if (order_ == Order::EqThenComp) {
        runEq(left, right, numFrames);
        runComp(left, right, numFrames, /*useEqSidechain=*/true);
    } else {
        // Comp→EQ : 사이드체인 EQ 출력이 아직 없으므로 메인 입력으로 검출.
        runComp(left, right, numFrames, /*useEqSidechain=*/false);
        runEq(left, right, numFrames);
    }

    // 출력 레벨
    for (unsigned int n = 0; n < numFrames; ++n) {
        float g = outGain_.next();
        left[n]  *= g;
        right[n] *= g;
    }
}

void ChannelStrip::setParameter(int paramId, float value) {
    if (paramId >= kStripBase) {
        switch (static_cast<StripParam>(paramId - kStripBase)) {
            case StripParam::InputGainDb:   inGain_.setTarget(dsp::dbToLin(value)); break;
            case StripParam::OutputLevelDb: outGain_.setTarget(dsp::dbToLin(value)); break;
            case StripParam::RoutingOrder:  order_ = (value >= 0.5f) ? Order::CompThenEq : Order::EqThenComp; break;
        }
    } else if (paramId >= kCompBase) {
        comp_.setParameter(paramId - kCompBase, value);
    } else {
        eq_.setParameter(paramId - kEqBase, value);
    }
}

void ChannelStrip::reset() {
    eq_.reset();
    comp_.reset();
}

void ChannelStrip::cleanup() {
    eq_.cleanup();
    comp_.cleanup();
}

} // namespace fx
