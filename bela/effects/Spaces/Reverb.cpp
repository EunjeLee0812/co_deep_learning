#include "Reverb.h"

namespace fx {

bool Reverb::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    inputGain_.setup(sampleRate, 20.0f);
    wetDryMix_.setup(sampleRate, 20.0f);
    inputGain_.snap(1.0f);
    wetDryMix_.snap(0.3f);
    // TODO: predelay 링버퍼 할당 (sampleRate * 0.25 샘플), room/hall 코어 초기화
    reset();
    return true;
}

void Reverb::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    for (unsigned int n = 0; n < numFrames; ++n) {
        const float g   = inputGain_.next();
        const float mix = wetDryMix_.next();
        float dryL = left[n] * g;
        float dryR = right[n] * g;

        // TODO(구현자): predelay → room/hall 코어 → 블렌딩 → 내부 EQ 거쳐 wetL/wetR 산출
        float wetL = 0.0f; // = reverbCore(dryL ...)
        float wetR = 0.0f;

        left[n]  = dryL * (1.0f - mix) + wetL * mix;
        right[n] = dryR * (1.0f - mix) + wetR * mix;
    }
}

void Reverb::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::InputGainDb:  inputGain_.setTarget(std::pow(10.0f, value / 20.0f)); break;
        case Param::PreDelayMs:   preDelayMs_ = value; /* TODO: predelay 탭 위치 갱신 */ break;
        case Param::RoomLevel:    roomLevel_  = value; break;
        case Param::HallLevel:    hallLevel_  = value; break;
        case Param::RoomSize:     roomSize_   = value; break;
        case Param::HallSize:     hallSize_   = value; break;
        case Param::WetDryMix:    wetDryMix_.setTarget(value); break;
        case Param::EqEnable:     eqEnabled_  = (value >= 0.5f); break;
        case Param::EqLowGainDb:  /* TODO: low shelf gain */  break;
        case Param::EqMidGainDb:  /* TODO: mid peak gain */   break;
        case Param::EqMidFreqHz:  /* TODO: mid peak freq */   break;
        case Param::EqHighGainDb: /* TODO: high shelf gain */ break;
        default: break;
    }
}

void Reverb::reset() {
    // TODO: predelay 버퍼/코어 딜레이라인/EQ 상태 0으로
}

void Reverb::cleanup() {
    // TODO: 동적 할당 해제
}

} // namespace fx
