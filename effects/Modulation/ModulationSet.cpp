// ModulationSet.cpp — 모듈레이션 세트 구현
#include "ModulationSet.h"

namespace fx {

bool ModulationSet::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    bool ok = true;
    ok &= flanger_.setup(sampleRate, maxBlockSize);
    ok &= phaser_.setup(sampleRate, maxBlockSize);
    ok &= chorus_.setup(sampleRate, maxBlockSize);
    // 서브모듈은 항상 활성; 세트 자체 bypass 로 전체 우회.
    flanger_.setBypass(false);
    phaser_.setBypass(false);
    chorus_.setBypass(false);
    reset();
    return ok;
}

Effect* ModulationSet::moduleOf(ModuleId id) {
    switch (id) {
        case ModuleId::Flanger: return &flanger_;
        case ModuleId::Phaser:  return &phaser_;
        case ModuleId::Chorus:  return &chorus_;
    }
    return nullptr;
}

void ModulationSet::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    for (int i = 0; i < 3; ++i) {
        Effect* m = moduleOf(order_[i]);
        if (m) m->process(left, right, numFrames);
    }
}

void ModulationSet::setParameter(int paramId, float value) {
    const int sub = paramId / 100;   // 0=Flanger,1=Phaser,2=Chorus,3+=세트
    if (sub == 0)      flanger_.setParameter(paramId - kFlangerBase, value);
    else if (sub == 1) phaser_.setParameter(paramId - kPhaserBase, value);
    else if (sub == 2) chorus_.setParameter(paramId - kChorusBase, value);
    else {
        switch (static_cast<SetParam>(paramId - kSetBase)) {
            case SetParam::TempoBpm:
                flanger_.setTempo(value); phaser_.setTempo(value); chorus_.setTempo(value);
                break;
            case SetParam::Order0: order_[0] = static_cast<ModuleId>((int)value); break;
            case SetParam::Order1: order_[1] = static_cast<ModuleId>((int)value); break;
            case SetParam::Order2: order_[2] = static_cast<ModuleId>((int)value); break;
        }
    }
}

void ModulationSet::reset() {
    flanger_.reset(); phaser_.reset(); chorus_.reset();
}

void ModulationSet::cleanup() {}

} // namespace fx
