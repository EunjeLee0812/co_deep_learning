// ChordPerformer.h
// ───────────────────────────────────────────────────────────────────────────
// 연주 로직: TrillFrame(센서 4개) → "지금 어떤 코드를 발음/해제할지" 이벤트로 변환.
//
//  - 오른쪽 보이싱 바 터치 상승엣지 = Trigger (코드 발음).
//    그 순간 링/퀄리티/복잡도 바 상태를 읽어 코드를 "래치"하고,
//    직전 코드 문맥을 넘겨 텐션을 고른다.
//  - 보이싱 바를 떼는 하강엣지 = Release (게이트 오프 → 릴리즈).
//  - 세기(touchSize) = 벨로시티. 보이싱 바 높이 = 보이싱 폭.
//
// SynthEngine 은 매 블록 poll() 을 호출해 Action 을 받고, 보이스 풀에 반영한다.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "MusicTheory.h"
#include "../hardware/TrillInput.h"  // hw::TrillFrame

namespace syn {

class ChordPerformer {
public:
    enum class Action { None, Trigger, Release };

    void reset() {
        prevTouched_ = false;
        prev_ = PrevChordContext{};
    }

    // 센서 스냅샷을 받아 이번 블록에 할 일을 결정.
    //  반환 Action == Trigger 이면 outChord / outVelocity 가 채워짐.
    //  Action == Release 이면 코드는 비움(전체 게이트 오프).
    Action poll(const hw::TrillFrame& f, Chord& outChord, float& outVelocity) {
        const bool touched = f.voicingActive;

        // 상승엣지: 새 코드 발음
        if (touched && !prevTouched_) {
            prevTouched_ = true;

            int rootPc = rootPcFromRing(f.ringActive ? f.ringPos : lastRingPos_);
            if (f.ringActive) lastRingPos_ = f.ringPos;

            Quality   q    = qualityFromBar(f.qualityActive ? f.qualityPos : 0.0f);
            float     cpos = f.complexityActive ? f.complexityPos : 0.16f;
            ChordType type = chordTypeFromBar(cpos);
            float     width= f.voicingPos;     // 0(좁음)~1(넓음)

            // 복잡도 바 최상단(Dom7Tension 구간)을 0..1 '모호도'로 재매핑.
            // 아래쪽은 이산 코드 타입, 위쪽은 모호도 파라미터로 동작.
            float ambiguity = 0.5f;
            if (type == ChordType::Dom7Tension) {
                const float lo = 5.0f / 6.0f;          // 6등분 중 마지막 칸 시작점
                ambiguity = (cpos - lo) / (1.0f - lo);
                if (ambiguity < 0.0f) ambiguity = 0.0f;
                if (ambiguity > 1.0f) ambiguity = 1.0f;
            }

            Chord c = buildChord(rootPc, q, type, width, prev_, ambiguity);
            outChord    = c;
            outVelocity = mapStrengthToVelocity(f.voicingStrength);

            prev_ = makeContext(c);            // 다음 텐션 선택용 문맥 갱신
            return Action::Trigger;
        }

        // 하강엣지: 해제
        if (!touched && prevTouched_) {
            prevTouched_ = false;
            return Action::Release;
        }

        return Action::None;
    }

private:
    // 터치 면적(세기) → 벨로시티. 약하게 쳐도 들리도록 바닥값 0.15.
    static float mapStrengthToVelocity(float s) {
        if (s < 0.0f) s = 0.0f; if (s > 1.0f) s = 1.0f;
        return 0.15f + 0.85f * s;
    }

    bool  prevTouched_  = false;
    float lastRingPos_  = 0.0f;       // 링에서 손 뗀 사이에도 마지막 root 유지
    PrevChordContext prev_;           // 직전 코드(텐션 예측 문맥)
};

} // namespace syn
