// EffectChain.cpp — 전체 이펙터 체인 (직렬 연결 + 파라미터 라우팅)
#include "EffectChain.h"

namespace fx {

bool EffectChain::setup(float sampleRate, unsigned int maxBlockSize) {
    bool ok = true;
    ok &= strip_.setup(sampleRate, maxBlockSize);
    ok &= drive_.setup(sampleRate, maxBlockSize);
    ok &= mod_.setup(sampleRate, maxBlockSize);
    ok &= echo_.setup(sampleRate, maxBlockSize);
    ok &= verb_.setup(sampleRate, maxBlockSize);

    // 기본 전부 BYPASS → DSP 미구현이어도 드라이 사운드가 흐른다.
    // 이펙트 담당이 각 이펙트 구현을 끝낼 때마다 setSlotBypass(slot, false) 로 켠다.
    strip_.setBypass(true);
    drive_.setBypass(true);
    mod_.setBypass(true);
    echo_.setBypass(true);
    verb_.setBypass(true);
    return ok;
}

void EffectChain::process(float* left, float* right, unsigned int numFrames) {
    // ── 기본 직렬 흐름 ──────────────────────────────────────────────────────
    // 각 이펙트는 스테레오 in-place 라 순서대로 같은 버퍼를 통과시키면 된다.
    // TODO(이펙트 담당): 순서 변경/병렬/센드리턴 등 라우팅을 여기서 조정.
    strip_.process(left, right, numFrames); // EQ / Comp / Drive (채널스트립)
    drive_.process(left, right, numFrames); // Distortion
    mod_.process(left, right, numFrames);   // Flanger / Phaser / Chorus
    echo_.process(left, right, numFrames);  // Delay
    verb_.process(left, right, numFrames);  // Reverb
}

void EffectChain::setParameter(int chainParamId, float value) {
    const int slot = chainParamId / kSlotStride;          // 1000단위 슬롯
    const int sub  = chainParamId % kSlotStride;          // 이펙트 내부 paramId
    switch (slot) {
        case Strip: strip_.setParameter(sub, value); break;
        case Drive: drive_.setParameter(sub, value); break;
        case Mod:   mod_.setParameter(sub, value);   break;
        case Echo:  echo_.setParameter(sub, value);  break;
        case Verb:  verb_.setParameter(sub, value);  break;
        default: break; // 알 수 없는 슬롯 무시
    }
}

void EffectChain::setSlotBypass(Slot s, bool bypass) {
    switch (s) {
        case Strip: strip_.setBypass(bypass); break;
        case Drive: drive_.setBypass(bypass); break;
        case Mod:   mod_.setBypass(bypass);   break;
        case Echo:  echo_.setBypass(bypass);  break;
        case Verb:  verb_.setBypass(bypass);  break;
        default: break;
    }
}

void EffectChain::reset() {
    strip_.reset(); drive_.reset(); mod_.reset(); echo_.reset(); verb_.reset();
}

void EffectChain::cleanup() {
    strip_.cleanup(); drive_.cleanup(); mod_.cleanup(); echo_.cleanup(); verb_.cleanup();
}

} // namespace fx
