// ModulationSet.h — 플렌저 + 페이저 + 코러스를 하나로 묶은 모듈레이션 세트
//   (레퍼런스: Clearmountain's Phases 의 "MODULE CONFIG" 처럼 순서 지정 가능)
//
// 디스플레이 → Bela 파라미터 ID 규약 (한 세트가 하나의 ID 공간을 가짐):
//   0   ~ 99   : Flanger  (서브 ID = paramId - kFlangerBase)
//   100 ~ 199  : Phaser   (서브 ID = paramId - kPhaserBase)
//   200 ~ 299  : Chorus   (서브 ID = paramId - kChorusBase)
//   300 ~      : 세트 글로벌 (아래 SetParam)
// 즉 submodule = paramId / 100. 이렇게 하면 디스플레이/통신 레이어가 단순해진다.
#pragma once
#include "Effect.h"
#include "Flanger.h"
#include "Phaser.h"
#include "Chorus.h"

namespace fx {

class ModulationSet : public Effect {
public:
    static constexpr int kFlangerBase = 0;
    static constexpr int kPhaserBase  = 100;
    static constexpr int kChorusBase  = 200;
    static constexpr int kSetBase     = 300;

    // 세트 글로벌 파라미터 (paramId = kSetBase + 아래 값)
    enum class SetParam : int {
        LfoSyncFlanger = 0,  // 0/1
        LfoSyncPhaser  = 1,  // 0/1
        SweepRateLeft  = 2,  // Hz 또는 노트분할
        SweepRateRight = 3,  // Hz
        LinkSweepRate  = 4,  // 0/1  좌우 sweep rate 링크
        Order0         = 5,  // 체인 1번 슬롯 모듈 (ModuleId)
        Order1         = 6,  // 체인 2번 슬롯
        Order2         = 7,  // 체인 3번 슬롯
    };

    enum class ModuleId : int { Flanger = 0, Phaser = 1, Chorus = 2 };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override; // ID 구간으로 라우팅
    void reset() override;
    void cleanup() override;

    // 직접 접근이 필요할 때 (테스트/단독 제어)
    Flanger& flanger() { return flanger_; }
    Phaser&  phaser()  { return phaser_; }
    Chorus&  chorus()  { return chorus_; }

private:
    Flanger flanger_;
    Phaser  phaser_;
    Chorus  chorus_;

    // 체인 순서. 기본: Flanger → Phaser → Chorus
    ModuleId order_[3] = { ModuleId::Flanger, ModuleId::Phaser, ModuleId::Chorus };

    float sweepRateLeft_  = 0.5f;
    float sweepRateRight_ = 0.5f;
    bool  linkSweepRate_  = false;

    Effect* moduleOf(ModuleId id);    // id → 해당 서브이펙트 포인터
    void    applySweepRates();        // 링크/좌우 sweep rate 를 서브모듈에 반영
};

} // namespace fx
