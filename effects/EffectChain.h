// EffectChain.h
// ───────────────────────────────────────────────────────────────────────────
// 신스 출력(스테레오)을 통과시키는 "전체 이펙터 체인" 래퍼.
//
// SynthEngine 은 이 한 클래스만 호출한다:
//     fx.setup(sr, maxBlock);
//     fx.process(left, right, numFrames);   // 매 블록
//     fx.setParameter(chainParamId, value); // 디스플레이/하드웨어에서 온 값
//
// ★ 내부 라우팅(어떤 순서로, 어떤 파라미터를 어디로)은 "이펙트 담당"이 채운다.
//   기본 신호 흐름(아래 process)과 파라미터 ID 규약은 제공하되,
//   순서 변경/세부 라우팅/슬롯 on-off 는 담당자가 TODO 에서 손본다.
//
// 기본 동작: 모든 이펙트는 처음엔 BYPASS 상태다. 그래서 DSP 가 아직 비어 있어도
//           신스 드라이 사운드가 그대로 출력된다. 담당자가 구현하며 하나씩 켠다.
//
// 체인 파라미터 ID 규약 (1000단위 슬롯):
//     0000~0999 : ChannelStrip  (sub = id - 0)      ← 내부는 100단위로 EQ/Comp
//     1000~1999 : Distortion     (sub = id - 1000)
//     2000~2999 : ModulationSet  (sub = id - 2000)  ← 내부 100단위로 Flanger/Phaser/Chorus
//     3000~3999 : Delay          (sub = id - 3000)
//     4000~4999 : Reverb         (sub = id - 4000)
//   예) Reverb WetDryMix(=6) → chainId = 4000 + 6 = 4006
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "Spaces/Reverb.h"
#include "Spaces/Delay.h"
#include "Modulation/ModulationSet.h"
#include "ChannelStrip/ChannelStrip.h"
#include "Distortion/Distortion.h"

namespace fx {

class EffectChain {
public:
    // 체인 슬롯(파라미터 라우팅 + on/off 용 식별자)
    enum Slot : int {
        Strip = 0,   // ChannelStrip : 0000~
        Drive = 1,   // Distortion   : 1000~
        Mod   = 2,   // ModulationSet: 2000~
        Echo  = 3,   // Delay        : 3000~
        Verb  = 4,   // Reverb       : 4000~
        NumSlots
    };
    static constexpr int kSlotStride = 1000;

    bool setup(float sampleRate, unsigned int maxBlockSize);
    void process(float* left, float* right, unsigned int numFrames);
    void setParameter(int chainParamId, float value);
    void setSlotBypass(Slot s, bool bypass);
    void reset();
    void cleanup();

private:
    // 신호 순서대로 둔다(기본: Strip → Drive → Mod → Echo → Verb).
    ChannelStrip  strip_;
    Distortion    drive_;
    ModulationSet mod_;
    Delay         echo_;
    Reverb        verb_;
};

} // namespace fx
