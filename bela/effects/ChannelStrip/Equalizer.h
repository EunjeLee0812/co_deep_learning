// Equalizer.h — 채널스트립 EQ 섹션 (레퍼런스: Apogee Symphony ECS)
//   HP 필터 + Lo Shelf + Mid Peak + Hi Shelf. HP 라우팅(HP>EQ / HP>SC) 지원.
#pragma once
#include "Effect.h"

namespace fx {

class Equalizer : public Effect {
public:
    enum class HpRoute : int { ToEq = 0, ToSidechain = 1 }; // HP>EQ / HP>SC

    enum class Param : int {
        Enable        = 0,  // 0/1
        HpFreqHz      = 1,  // 20 .. 300
        HpRouting     = 2,  // 0=HP>EQ, 1=HP>SC
        LoShelfFreqHz = 3,  // 35 .. 300
        LoShelfGainDb = 4,  // -15 .. +15
        MidFreqHz     = 5,  // 250 .. 8000
        MidGainDb     = 6,  // -15 .. +15
        MidBell       = 7,  // 0/1 (벨 ↔ 다른 형태 토글)
        HiShelfFreqHz = 8,  // 4000 .. 16000
        HiShelfGainDb = 9,  // -15 .. +15
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

    // 컴프 사이드체인용: HP>SC 일 때 HP 통과 신호를 따로 뽑아 컴프에 넘김
    HpRoute hpRoute() const { return hpRoute_; }
    // TODO(구현자): 사이드체인 신호 접근자 추가 (예: getSidechain(buf,...))

private:
    bool    enabled_   = true;
    float   hpFreq_    = 80.0f;
    HpRoute hpRoute_   = HpRoute::ToEq;
    float   loFreq_    = 100.0f,  loGain_ = 0.0f;
    float   midFreq_   = 1000.0f, midGain_= 0.0f;
    bool    midBell_   = true;
    float   hiFreq_    = 8000.0f, hiGain_ = 0.0f;
    // TODO(구현자): 4개 바이쿼드(HP, lowshelf, peak, highshelf) 좌우 각각. 계수 재계산은
    //   파라미터 변경 시에만. midBell 로 peak↔shelf 전환.
};

} // namespace fx
