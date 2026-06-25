// Compressor.cpp — 피크검출 피드포워드 컴프레서 (소프트니, 스테레오 링크, 사이드체인)
#include "ChannelStrip/Compressor.h"

namespace fx {

bool Compressor::setup(float sampleRate, unsigned int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    recalcBallistics();
    reset();
    return true;
}

void Compressor::recalcBallistics() {
    // 게인(dB) 평활용 1극 계수. attack/release.
    atkCoeff_ = std::exp(-1.0f / (attackMs_  * 0.001f * sampleRate_));
    relCoeff_ = std::exp(-1.0f / (releaseMs_ * 0.001f * sampleRate_));
}

// 입력 레벨(dB) → 정적 곡선상의 목표 게인리덕션(dB, 0 또는 음수의 절댓값).
inline float Compressor::computeTargetGRdb(float levelDb) const {
    float over = levelDb - thresholdDb_;
    if (over <= -kneeDb_ * 0.5f) {
        return 0.0f;                          // 니 아래: 무압축
    } else if (over >= kneeDb_ * 0.5f) {
        return over * (1.0f - 1.0f / ratio_); // 니 위: 풀 비율
    } else {
        // 소프트니 구간: 2차 보간
        float x = over + kneeDb_ * 0.5f;      // 0..knee
        float a = (1.0f - 1.0f / ratio_) / (2.0f * kneeDb_);
        return a * x * x;
    }
}

void Compressor::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_ || !enabled_) { hasSidechain_ = false; gainReductionDb_ = 0.0f; return; }

    const float makeupLin = dsp::dbToLin(makeupDb_);

    for (unsigned int n = 0; n < numFrames; ++n) {
        // 검출 신호: 사이드체인 있으면 그것, 없으면 메인. 스테레오 max.
        float dL = hasSidechain_ ? scL_[n] : left[n];
        float dR = hasSidechain_ ? scR_[n] : right[n];
        float det = std::fabs(dL) > std::fabs(dR) ? std::fabs(dL) : std::fabs(dR);
        float levelDb = dsp::linToDb(det);

        // 목표 GR → attack/release 평활. (압축 깊어질 땐 attack, 풀릴 땐 release)
        float targetGR = computeTargetGRdb(levelDb);   // >=0
        float coeff = (targetGR > -envDb_) ? atkCoeff_ : relCoeff_;
        // envDb_ 는 음수 게인(=-GR)로 들고 감
        float targetEnv = -targetGR;
        envDb_ = targetEnv + (envDb_ - targetEnv) * coeff;

        gainReductionDb_ = envDb_;  // 미터(음수)
        float gain = dsp::dbToLin(envDb_) * makeupLin;

        float wetL = left[n]  * gain;
        float wetR = right[n] * gain;
        left[n]  = left[n]  * (1.0f - dryWet_) + wetL * dryWet_;
        right[n] = right[n] * (1.0f - dryWet_) + wetR * dryWet_;
    }
    hasSidechain_ = false;  // 사이드체인은 블록 단위 1회성
}

void Compressor::setSidechain(const float* scL, const float* scR, unsigned int /*n*/) {
    scL_ = scL; scR_ = scR; hasSidechain_ = (scL && scR);
}

void Compressor::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::Enable:      enabled_ = (value >= 0.5f); break;
        case Param::ThresholdDb: thresholdDb_ = value; break;
        case Param::Ratio:       ratio_ = (value < 1.0f ? 1.0f : value); break;
        case Param::AttackMs:    attackMs_  = value; recalcBallistics(); break;
        case Param::ReleaseMs:   releaseMs_ = value; recalcBallistics(); break;
        case Param::DryWetMix:   dryWet_ = dsp::clampf(value, 0.0f, 1.0f); break;
        case Param::MakeupDb:    makeupDb_ = value; break;
        default: break;
    }
}

void Compressor::reset() {
    envDb_ = 0.0f;
    gainReductionDb_ = 0.0f;
    hasSidechain_ = false;
}

} // namespace fx
