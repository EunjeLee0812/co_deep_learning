#include "ModulationSet.h"

namespace fx {

bool ModulationSet::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    bool ok = true;
    ok &= flanger_.setup(sampleRate, maxBlockSize);
    ok &= phaser_.setup(sampleRate, maxBlockSize);
    ok &= chorus_.setup(sampleRate, maxBlockSize);
    return ok;
}

void ModulationSet::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    // order_ 순서대로 직렬 처리 (각 모듈은 자기 InOut/bypass 를 알아서 처리)
    for (int slot = 0; slot < 3; ++slot) {
        if (Effect* m = moduleOf(order_[slot]))
            m->process(left, right, numFrames);
    }
}

void ModulationSet::setParameter(int paramId, float value) {
    if (paramId >= kSetBase) {
        switch (static_cast<SetParam>(paramId - kSetBase)) {
            case SetParam::LfoSyncFlanger: flanger_.setLfoSync(value >= 0.5f); break;
            case SetParam::LfoSyncPhaser:  phaser_.setLfoSync(value >= 0.5f);  break;
            case SetParam::SweepRateLeft:  sweepRateLeft_  = value; applySweepRates(); break;
            case SetParam::SweepRateRight: sweepRateRight_ = value; applySweepRates(); break;
            case SetParam::LinkSweepRate:  linkSweepRate_  = (value >= 0.5f); applySweepRates(); break;
            case SetParam::Order0: order_[0] = static_cast<ModuleId>((int)value); break;
            case SetParam::Order1: order_[1] = static_cast<ModuleId>((int)value); break;
            case SetParam::Order2: order_[2] = static_cast<ModuleId>((int)value); break;
        }
        return;
    }
    // 서브모듈로 라우팅: 100단위 구간
    const int sub = paramId / 100;          // 0=flanger,1=phaser,2=chorus
    const int subId = paramId - sub * 100;
    switch (sub) {
        case 0: flanger_.setParameter(subId, value); break;
        case 1: phaser_.setParameter(subId, value);  break;
        case 2: chorus_.setParameter(subId, value);  break;
        default: break;
    }
}

Effect* ModulationSet::moduleOf(ModuleId id) {
    switch (id) {
        case ModuleId::Flanger: return &flanger_;
        case ModuleId::Phaser:  return &phaser_;
        case ModuleId::Chorus:  return &chorus_;
    }
    return nullptr;
}

void ModulationSet::applySweepRates() {
    // TODO: 좌우 분리 스윕을 어떻게 매핑할지 결정 (예: 플렌저=Left, 페이저=Right).
    //       링크면 둘 다 sweepRateLeft_ 사용.
    const float l = sweepRateLeft_;
    const float r = linkSweepRate_ ? sweepRateLeft_ : sweepRateRight_;
    flanger_.setSweepRate(l);
    phaser_.setSweepRate(r);
}

void ModulationSet::reset() {
    flanger_.reset(); phaser_.reset(); chorus_.reset();
}

void ModulationSet::cleanup() {
    flanger_.cleanup(); phaser_.cleanup(); chorus_.cleanup();
}

} // namespace fx
