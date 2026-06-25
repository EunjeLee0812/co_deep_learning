// Equalizer.cpp — 채널스트립 EQ 구현
#include "ChannelStrip/Equalizer.h"

namespace fx {

bool Equalizer::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    for (dsp::Biquad* b : { &hpL_,&hpR_,&loL_,&loR_,&midL_,&midR_,&hiL_,&hiR_ })
        b->setSampleRate(sampleRate);
    scL_.assign(maxBlockSize, 0.0f);
    scR_.assign(maxBlockSize, 0.0f);
    recalc();
    reset();
    return true;
}

void Equalizer::recalc() {
    // HP (Q=0.707 butterworth 류)
    hpL_.setHighpass(hpFreq_, 0.707f);
    hpR_.setHighpass(hpFreq_, 0.707f);
    // Lo shelf
    loL_.setLowShelf(loFreq_, loGain_);
    loR_.setLowShelf(loFreq_, loGain_);
    // Mid peak (Q 고정 1.0; midBell off 면 통과)
    if (midBell_) { midL_.setPeak(midFreq_, 1.0f, midGain_); midR_.setPeak(midFreq_, 1.0f, midGain_); }
    else          { midL_.setPassthrough(); midR_.setPassthrough(); }
    // Hi shelf
    hiL_.setHighShelf(hiFreq_, hiGain_);
    hiR_.setHighShelf(hiFreq_, hiGain_);
}

void Equalizer::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_ || !enabled_) {
        // 바이패스여도 사이드체인은 원음으로 채워둔다(컴프가 일반 동작하도록).
        for (unsigned int n = 0; n < numFrames; ++n) { scL_[n] = left[n]; scR_[n] = right[n]; }
        return;
    }
    const bool hpToSc = (hpRoute_ == HpRoute::ToSidechain);

    for (unsigned int n = 0; n < numFrames; ++n) {
        float xL = left[n], xR = right[n];

        // HP 통과 신호(항상 계산)
        float hpXL = hpL_.process(xL);
        float hpXR = hpR_.process(xR);

        // 메인 경로 입력 결정: HP>EQ 면 HP 적용, HP>SC 면 원음 유지.
        float mL = hpToSc ? xL : hpXL;
        float mR = hpToSc ? xR : hpXR;

        // shelves + peak 직렬
        mL = loL_.process(mL); mL = midL_.process(mL); mL = hiL_.process(mL);
        mR = loR_.process(mR); mR = midR_.process(mR); mR = hiR_.process(mR);

        left[n]  = mL;
        right[n] = mR;

        // 사이드체인: HP>SC 면 HP 통과 신호, 아니면 EQ 출력.
        scL_[n] = hpToSc ? hpXL : mL;
        scR_[n] = hpToSc ? hpXR : mR;
    }
}

void Equalizer::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::Enable:        enabled_ = (value >= 0.5f); break;
        case Param::HpFreqHz:      hpFreq_  = value; recalc(); break;
        case Param::HpRouting:     hpRoute_ = (value >= 0.5f) ? HpRoute::ToSidechain : HpRoute::ToEq; break;
        case Param::LoShelfFreqHz: loFreq_  = value; recalc(); break;
        case Param::LoShelfGainDb: loGain_  = value; recalc(); break;
        case Param::MidFreqHz:     midFreq_ = value; recalc(); break;
        case Param::MidGainDb:     midGain_ = value; recalc(); break;
        case Param::MidBell:       midBell_ = (value >= 0.5f); recalc(); break;
        case Param::HiShelfFreqHz: hiFreq_  = value; recalc(); break;
        case Param::HiShelfGainDb: hiGain_  = value; recalc(); break;
        default: break;
    }
}

void Equalizer::reset() {
    for (dsp::Biquad* b : { &hpL_,&hpR_,&loL_,&loR_,&midL_,&midR_,&hiL_,&hiR_ })
        b->reset();
}

} // namespace fx
