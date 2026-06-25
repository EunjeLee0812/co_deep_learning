// ChannelStrip.h — EQ + 컴프 + (공통 Gain/Out) 채널스트립
// ───────────────────────────────────────────────────────────────────────────
// 파라미터 ID 규약 (한 스트립이 하나의 ID 공간):
//   0   ~ 99   : Equalizer  (서브 ID = paramId - kEqBase)
//   100 ~ 199  : Compressor (서브 ID = paramId - kCompBase)
//   200 ~      : 스트립 글로벌 (StripParam, 서브 ID = paramId - kStripBase)
//
// 라우팅:
//   in → ×Gain → [순서: EQ→Comp 또는 Comp→EQ] → ×Out
//   EQ 의 HP>SC 가 켜져 있으면 EQ 의 사이드체인 출력을 컴프 검출 입력으로 연결.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"
#include "Equalizer.h"
#include "Compressor.h"

namespace fx {

class ChannelStrip : public Effect {
public:
    static constexpr int kEqBase    = 0;
    static constexpr int kCompBase  = 100;
    static constexpr int kStripBase = 200;

    enum class Order : int { EqThenComp = 0, CompThenEq = 1 };

    // 스트립 글로벌 (paramId = kStripBase + 값)
    enum class StripParam : int {
        InputGainDb   = 0,  // -inf .. +12  공통 Gain
        OutputLevelDb = 1,  // -inf .. +12  공통 Out
        RoutingOrder  = 2,  // 0=EQ>COMP, 1=COMP>EQ
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;
    void cleanup() override;

    Equalizer&  eq()   { return eq_; }
    Compressor& comp() { return comp_; }

private:
    Equalizer  eq_;
    Compressor comp_;
    Order order_ = Order::EqThenComp;
    SmoothedValue inGain_, outGain_;

    // EQ→Comp 순서일 때 HP>SC 사이드체인을 연결하는 헬퍼.
    void runEq(float* l, float* r, unsigned int n);
    void runComp(float* l, float* r, unsigned int n, bool useEqSidechain);
};

} // namespace fx
