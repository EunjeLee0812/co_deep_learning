// Reverb.h — 플레이트/홀 리버브 (Freeverb 계열 comb+allpass 코어 + 스핀 모듈레이션)
// ───────────────────────────────────────────────────────────────────────────
// 디스플레이 노브 매핑(사용자 스펙):
//   공통 : Gain(입력), Out(출력 레벨)
//   탭   : Type (Plate / Hall 선택)
//   노브 : Size, Decay, LowCut(리버브 들어가기 전), HighCut, Spin, SpinDepth, Mix
//
// 신호 흐름(스테레오):
//   in → ×Gain → [LowCut HP] → [HighCut 는 코어 감쇠로 반영] → reverb 코어(comb×8→allpass×4)
//      → wet.  out = (dry×(1-Mix) + wet×Mix) × Out
//   Spin : 코어 allpass 딜레이를 LFO 로 미세 변조 → 잔향이 살아 움직이는 느낌.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../Effect.h"
#include "../Dsp.h"

namespace fx {

class Reverb : public Effect {
public:
    enum class Type : int { Plate = 0, Hall = 1 };

    // 디스플레이가 이 정수 ID 로 값을 보낸다. (value 단위는 주석 참고)
    enum class Param : int {
        Gain      = 0,   // -inf..+12 dB  입력 게인
        Out       = 1,   // -inf..+12 dB  출력 레벨
        Type      = 2,   // 0=Plate, 1=Hall
        Size      = 3,   // 0..1  공간 크기(딜레이 길이 스케일)
        Decay     = 4,   // 0..1  잔향 길이(피드백)
        LowCut    = 5,   // 20..1000 Hz   리버브 send 전 하이패스
        HighCut   = 6,   // 1000..20000 Hz 코어 내부 댐핑(고역 감쇠) 코너
        Spin      = 7,   // 0..5 Hz   잔향 변조 속도
        SpinDepth = 8,   // 0..1      변조 깊이
        Mix       = 9,   // 0(dry)..1(wet)
        NumParams
    };

    bool setup(float sampleRate, unsigned int maxBlockSize) override;
    void process(float* left, float* right, unsigned int numFrames) override;
    void setParameter(int paramId, float value) override;
    void reset() override;
    void cleanup() override;

private:
    // ── 코어 구성요소 ──
    // Comb : 댐핑(고역감쇠) 포함 피드백 콤필터. 잔향의 밀도/길이 담당.
    struct Comb {
        dsp::DelayLine line;
        float store = 0.0f;     // 댐핑 1-pole 상태
        float baseDelay = 0.0f; // 기준 딜레이(샘플)
        inline float process(float in, float feedback, float damp) {
            float out = line.read(baseDelay);
            store = out + (store - out) * damp;   // 피드백 경로 lowpass(=HighCut)
            line.write(in + store * feedback);
            return out;
        }
    };
    // Allpass : 잔향을 매끄럽게 확산. spin 으로 딜레이 변조.
    struct Allpass {
        dsp::DelayLine line;
        float baseDelay = 0.0f;
        inline float process(float in, float modSamples) {
            float d = baseDelay + modSamples;
            float bufout = line.read(d);
            float out = -in + bufout;
            line.write(in + bufout * 0.5f);
            return out;
        }
    };

    static constexpr int kNumCombs = 8;
    static constexpr int kNumAllpass = 4;

    Comb     combL_[kNumCombs],  combR_[kNumCombs];
    Allpass  apL_[kNumAllpass],  apR_[kNumAllpass];

    dsp::OnePole lowCutL_, lowCutR_;   // 리버브 send 전 하이패스
    dsp::Lfo     spinLfo_;

    SmoothedValue inGain_, outGain_, mix_;

    // 파라미터 상태
    Type  type_      = Type::Hall;
    float size_      = 0.5f;
    float decay_     = 0.6f;
    float highCutHz_ = 8000.0f;
    float spinHz_    = 0.5f;
    float spinDepth_ = 0.0f;

    // 파생 계수(파라미터 바뀔 때만 재계산)
    float feedback_  = 0.84f;
    float damp_      = 0.2f;
    float spinSamples_ = 0.0f;

    void recalcCore();   // size/decay/type/highcut → 딜레이길이/피드백/댐핑 재계산
};

} // namespace fx
