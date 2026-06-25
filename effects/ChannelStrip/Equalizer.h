// Equalizer.h — 채널스트립 EQ 섹션
//   HP(하이패스) + Lo Shelf + Mid Peak + Hi Shelf. HP 라우팅(HP>EQ / HP>SC) 지원.
// ───────────────────────────────────────────────────────────────────────────
// 디스플레이 노브 매핑(사용자 스펙):
//   Highpass 주파수, LoShelf 주파수/게인, MidPeak 주파수/게인, HiShelf 주파수/게인,
//   HP 라우팅 스위치(HP>EQ / HP>SC).
//
// HP>EQ : 하이패스를 메인 오디오 경로에 적용(저음이 실제로 깎임).
// HP>SC : 하이패스 신호를 컴프 사이드체인으로만 보냄. 귀에는 저음 그대로,
//         컴프만 저음에 덜 반응 → 킥/베이스로 인한 펌핑 방지.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"
#include <vector>

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
        MidBell       = 7,  // 0/1 (사용 안하면 1 고정)
        HiShelfFreqHz = 8,  // 4000 .. 16000
        HiShelfGainDb = 9,  // -15 .. +15
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;

    // 컴프 사이드체인용 접근자. HP>SC 일 때 process 가 채워둔 HP 통과 신호.
    // (HP>EQ 일 때는 메인 출력과 동일한 신호가 들어 있음 = 일반 사이드체인)
    HpRoute      hpRoute()      const { return hpRoute_; }
    const float* sidechainL()   const { return scL_.data(); }
    const float* sidechainR()   const { return scR_.data(); }

private:
    bool    enabled_   = true;
    float   hpFreq_    = 80.0f;
    HpRoute hpRoute_   = HpRoute::ToEq;
    float   loFreq_    = 100.0f,  loGain_ = 0.0f;
    float   midFreq_   = 1000.0f, midGain_= 0.0f;
    bool    midBell_   = true;
    float   hiFreq_    = 8000.0f, hiGain_ = 0.0f;

    // 채널별 바이쿼드 4개 (HP, lowshelf, peak, highshelf)
    dsp::Biquad hpL_, hpR_, loL_, loR_, midL_, midR_, hiL_, hiR_;

    std::vector<float> scL_, scR_;   // 사이드체인 출력 버퍼 (maxBlock 크기)

    void recalc();   // 파라미터 → 바이쿼드 계수 재계산
};

} // namespace fx
