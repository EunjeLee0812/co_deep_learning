// Reverb.h — 리버브 (레퍼런스: Clearmountain's Spaces)
//
// 디스플레이에서 받는 것:
//   - 룸 계열 리버브 양, 홀 계열 리버브 양 (두 공간을 블렌딩)
//   - 프리딜레이(ms), wet/dry
//   - 리버브 출력에 거는 내부 EQ
// 입력 오디오를 받아 잔향이 더해진 출력을 낸다.
#pragma once
#include "../Effect.h"

namespace fx {

class Reverb : public Effect {
public:
    // value 범위는 주석 참고. 디스플레이가 이 정수 ID 로 값을 보낸다.
    enum class Param : int {
        InputGainDb  = 0,   // -inf .. +12  (dB)
        PreDelayMs   = 1,   // 0 .. 250     (ms)
        RoomLevel    = 2,   // 0.0 .. 1.0   룸 계열 잔향 양
        HallLevel    = 3,   // 0.0 .. 1.0   홀 계열 잔향 양
        RoomSize     = 4,   // 0.0 .. 1.0   룸 코어 크기/감쇠
        HallSize     = 5,   // 0.0 .. 1.0   홀 코어 크기/감쇠
        WetDryMix    = 6,   // 0.0(dry) .. 1.0(wet)
        EqEnable     = 7,   // 0 / 1
        EqLowGainDb  = 8,   // -15 .. +15   (low shelf)
        EqMidGainDb  = 9,   // -15 .. +15   (mid peak)
        EqMidFreqHz  = 10,  // 200 .. 8000
        EqHighGainDb = 11,  // -15 .. +15   (high shelf)
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;
    void cleanup() override;

private:
    // ---- 디스플레이로부터 받은 파라미터 상태 ----
    SmoothedValue inputGain_;
    SmoothedValue wetDryMix_;
    float preDelayMs_ = 0.0f;
    float roomLevel_  = 0.3f;
    float hallLevel_  = 0.0f;
    float roomSize_   = 0.5f;
    float hallSize_   = 0.7f;
    bool  eqEnabled_  = false;

    // TODO(구현자): 아래 DSP 블록을 구현/연결할 것
    //  1) PreDelay: 링버퍼 (max 250ms). 입력을 지연시킨 뒤 리버브 코어로 보냄.
    //  2) Room 코어 : 짧고 조밀한 잔향 (예: Schroeder comb+allpass 또는 작은 FDN).
    //  3) Hall 코어 : 길고 부드러운 잔향 (예: 큰 FDN). roomSize/hallSize 로 감쇠 제어.
    //  4) 두 코어 출력을 roomLevel/hallLevel 로 가중합 → reverbOut.
    //  5) 내부 EQ : reverbOut 에 3밴드(low shelf/mid peak/high shelf) 적용. eqEnabled 일 때만.
    //  6) WetDry : out = dry*(1-mix) + (eq(reverbOut))*mix, inputGain 반영.
    //  스테레오 디코릴레이션(좌우 다른 딜레이 탭)으로 공간감 줄 것.
};

} // namespace fx
