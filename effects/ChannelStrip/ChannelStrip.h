// ChannelStrip.h — EQ + 컴프 + 드라이브 + 출력레벨을 묶은 채널스트립
//   (레퍼런스: Apogee Symphony ECS Channel Strip)
//
// 파라미터 ID 규약 (한 스트립이 하나의 ID 공간):
//   0   ~ 99   : Equalizer  (서브 ID = paramId - kEqBase)
//   100 ~ 199  : Compressor (서브 ID = paramId - kCompBase)
//   200 ~      : 스트립 글로벌 (StripParam)
#pragma once
#include "../Effect.h"
#include "Equalizer.h"
#include "Compressor.h"

namespace fx {

class ChannelStrip : public Effect {
public:
    static constexpr int kEqBase   = 0;
    static constexpr int kCompBase = 100;
    static constexpr int kStripBase= 200;

    enum class Order : int { EqThenComp = 0, CompThenEq = 1 }; // EQ>COMP / COMP>EQ

    enum class StripParam : int {
        Drive         = 0,  // 0 .. 10
        OutputLevelDb = 1,  // -20 .. +20
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
    Order order_      = Order::EqThenComp;
    float drive_      = 0.0f;
    SmoothedValue outputGain_;
    // TODO(구현자):
    //  - order_ 에 따라 eq→comp 또는 comp→eq 순서로 process
    //  - EQ 의 HP>SC 라우팅이면 EQ 의 사이드체인 출력을 comp.setSidechain 으로 연결
    //  - drive: 출력 전 비선형 새츄레이션(0이면 통과)
    //  - outputGain: 마지막에 적용
    void applyDrive(float* l, float* r, unsigned int n); // TODO
};

} // namespace fx
