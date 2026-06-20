// Delay.h — 스테레오 딜레이 (레퍼런스: 이미지 4 / Left·Right Delay)
//
// 디스플레이에서 받는 것:
//   - 좌우 독립 딜레이 타임(ms) + 노트분할(템포 sync), 오프셋, 스핀
//   - 글로벌: BPM, Sync, Tap, Spin Crossfeed, Pitch Pre-Spin, Delay Blur
//   - 링크 토글(LinkSpin/LinkOffset/LinkEq), 좌우 EQ(미드 파라메트릭)
//   - 피드백(반복) — 패널엔 안 보이지만 딜레이 필수라 포함
#pragma once
#include "Effect.h"

namespace fx {

class Delay : public Effect {
public:
    enum class Param : int {
        // ---- 글로벌 ----
        TempoBpm        = 0,   // 20 .. 300
        SyncEnable      = 1,   // 0/1  켜지면 노트분할 기반으로 딜레이타임 계산
        SpinCrossfeed   = 2,   // 0/1
        PitchPreSpin    = 3,   // 0/1
        DelayBlur       = 4,   // 0.0 .. 1.0  (반복음 번짐/디퓨전)
        LinkSpin        = 5,   // 0/1  좌우 스핀 링크
        LinkOffset      = 6,   // 0/1
        LinkEq          = 7,   // 0/1
        // ---- LEFT ----
        LeftDelayMs     = 10,  // 0 .. 4000 (sync off 일 때 직접값)
        LeftNoteDiv     = 11,  // 0..6 (1/1,1/2,1/4,1/8,1/16,dot,triplet) — sync on
        LeftOffsetMs    = 12,  // -50 .. +50
        LeftSpin        = 13,  // -inf..0 dB 또는 0..1 (모듈레이션 양)
        LeftFeedback    = 14,  // 0.0 .. ~1.1 (자기발진 주의)
        LeftEqEnable    = 15,  // 0/1
        LeftEqMidFreqHz = 16,  // 200 .. 8000
        LeftEqGainDb    = 17,  // -15 .. +15
        LeftEqQ         = 18,  // 0.3 .. 4.0
        // ---- RIGHT ----
        RightDelayMs     = 20,
        RightNoteDiv     = 21,
        RightOffsetMs    = 22,
        RightSpin        = 23,
        RightFeedback    = 24,
        RightEqEnable    = 25,
        RightEqMidFreqHz = 26,
        RightEqGainDb    = 27,
        RightEqQ         = 28,
        NumParams
    };

    // 노트 분할 종류 (LeftNoteDiv/RightNoteDiv 값 매핑)
    enum class NoteDiv : int { Whole=0, Half=1, Quarter=2, Eighth=3, Sixteenth=4, Dotted=5, Triplet=6 };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;
    void cleanup() override;

private:
    // 한 채널 분량의 상태를 묶어둠 (좌/우 각각)
    struct Channel {
        SmoothedValue delaySamples; // 현재 딜레이 길이(샘플) — 스무딩해서 글리치 방지
        float offsetMs   = 0.0f;
        float spin       = 0.0f;
        float feedback   = 0.3f;
        bool  eqEnabled  = false;
        float eqFreqHz   = 1000.0f;
        float eqGainDb   = 0.0f;
        float eqQ        = 0.7f;
        NoteDiv noteDiv  = NoteDiv::Quarter;
        // TODO: 딜레이 링버퍼, 분수지연 보간(allpass/linear), 스핀용 LFO, 미드 EQ 바이쿼드
    };
    Channel left_, right_;

    float tempoBpm_     = 120.0f;
    bool  syncEnabled_  = false;
    bool  spinCrossfeed_= false;
    bool  pitchPreSpin_ = false;
    float delayBlur_    = 0.0f;
    bool  linkSpin_=false, linkOffset_=false, linkEq_=false;

    // TODO(구현자):
    //  - syncEnabled 면 (tempoBpm, noteDiv, offset) 로 딜레이 샘플수 계산해 delaySamples.setTarget
    //  - process: 좌우 링버퍼 read/write, feedback, spin(LFO로 read위치 흔들기), blur(디퓨전),
    //    crossfeed(좌피드백을 우로/우를 좌로), pitchPreSpin(스핀 전 피치시프트), 좌우 EQ 적용
    //  - link* 가 켜지면 우 채널 해당 파라미터를 좌값으로 강제
    void applyLink(); // TODO: 링크 반영 헬퍼
    float msToSamples(float ms) const { return ms * 0.001f * sampleRate_; }
};

} // namespace fx
