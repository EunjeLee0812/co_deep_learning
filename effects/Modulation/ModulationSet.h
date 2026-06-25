// ModulationSet.h — 플랜저 + 페이저 + 코러스 묶음 (순서 지정 + 템포 브로드캐스트)
// ───────────────────────────────────────────────────────────────────────────
// 파라미터 ID 규약 (한 세트가 하나의 ID 공간, submodule = paramId / 100):
//   0   ~ 99   : Flanger  (서브 ID = paramId - kFlangerBase)
//   100 ~ 199  : Phaser   (서브 ID = paramId - kPhaserBase)
//   200 ~ 299  : Chorus   (서브 ID = paramId - kChorusBase)
//   300 ~      : 세트 글로벌 (SetParam)
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
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

    // 세트 글로벌 (paramId = kSetBase + 값)
    enum class SetParam : int {
        TempoBpm = 0,  // 20..300  세 모듈 BpmSync 에 공통 적용
        Order0   = 1,  // 체인 1번 슬롯 (ModuleId)
        Order1   = 2,  // 체인 2번 슬롯
        Order2   = 3,  // 체인 3번 슬롯
    };

    enum class ModuleId : int { Flanger = 0, Phaser = 1, Chorus = 2 };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;
    void cleanup() override;

    Flanger& flanger() { return flanger_; }
    Phaser&  phaser()  { return phaser_; }
    Chorus&  chorus()  { return chorus_; }

private:
    Flanger flanger_;
    Phaser  phaser_;
    Chorus  chorus_;

    ModuleId order_[3] = { ModuleId::Flanger, ModuleId::Phaser, ModuleId::Chorus };

    Effect* moduleOf(ModuleId id);
};

} // namespace fx
