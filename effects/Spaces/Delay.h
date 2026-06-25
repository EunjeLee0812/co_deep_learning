// Delay.h — 스테레오 딜레이 (좌우 독립 시간/박자, BPM sync, 링크, 필터, 핑퐁)
// ───────────────────────────────────────────────────────────────────────────
// 디스플레이 노브 매핑(사용자 스펙):
//   공통   : Gain(입력), Out(출력 레벨)
//   노브   : Feedback, LeftTime/Div, RightTime/Div, HighCut, LowCut, Mix
//   버튼   : BpmSync(켜면 노브가 '시간'아닌 '박자'), Link(좌우 값 동일), Mode(Normal/PingPong)
//   글로벌 : TempoBpm (BpmSync 박자 계산용)
//
// 동작:
//   BpmSync off → LeftTimeMs/RightTimeMs(ms) 직접 사용.
//   BpmSync on  → LeftDiv/RightDiv(노트분할) + TempoBpm 으로 딜레이타임 계산.
//   Link on     → 우 채널 시간을 좌 채널 값으로 강제.
//   HighCut/LowCut → 피드백(반복음) 경로에 적용 → 반복할수록 어두워짐.
//   PingPong    → 좌우 딜레이가 서로 교차 피드백되어 좌→우→좌 바운스.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"

namespace fx {

class Delay : public Effect {
public:
    enum class Mode : int { Normal = 0, PingPong = 1 };

    enum class Param : int {
        Gain        = 0,   // -inf..+12 dB
        Out         = 1,   // -inf..+12 dB
        Feedback    = 2,   // 0..~0.98 (반복량)
        TempoBpm    = 3,   // 20..300
        BpmSync     = 4,   // 0/1
        Link        = 5,   // 0/1  좌우 시간 동일
        Mode        = 6,   // 0=Normal, 1=PingPong
        HighCut     = 7,   // 1000..20000 Hz  피드백 LP
        LowCut      = 8,   // 20..2000 Hz     피드백 HP
        Mix         = 9,   // 0(dry)..1(wet)
        LeftTimeMs  = 10,  // 1..4000 (sync off)
        LeftDiv     = 11,  // 0..6 노트분할 (sync on)
        RightTimeMs = 12,
        RightDiv    = 13,
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;
    void cleanup() override;

private:
    struct Channel {
        dsp::DelayLine line;
        dsp::OnePole   fbLowCut;   // 피드백 경로 HP (LowCut)
        dsp::OnePole   fbHighCut;  // 피드백 경로 LP (HighCut)
        SmoothedValue  delaySamples;
        float  timeMs = 333.0f;
        int    noteDiv = 2;        // 기본 1/4
    };
    Channel left_, right_;

    SmoothedValue inGain_, outGain_, mix_;
    float feedback_  = 0.35f;
    float tempoBpm_  = 120.0f;
    bool  bpmSync_   = false;
    bool  link_      = false;
    Mode  mode_      = Mode::Normal;

    float msToSamples(float ms) const { return ms * 0.001f * sampleRate_; }
    void  recalcTimes();   // sync/link/tempo 반영해 좌우 delaySamples 목표 갱신
};

} // namespace fx
