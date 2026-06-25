// ChordVoicing.h — (신규) Trill 연주센서 → 연속 피치 매핑 (음악 계층, 순수 함수)
// ───────────────────────────────────────────────────────────────────────────
// 구 MusicTheory / ChordPerformer / CompTables / gen_tables 를 전부 대체하는
// "가벼운" 음악 계층. 텐션선택·보이싱추론 같은 추론 로직은 모두 버린다.
//
//   Trill Ring (5도권, 이산)  → 루트 피치클래스 (어떤 코드인지)
//   각 Trill Bar (연속)       → 기준음 ± 반음   (바이올린처럼 연속, 등간격)
//
// 오디오/하드웨어 의존이 전혀 없어 단독으로 테스트 가능하다.
//
// ★ 데이터/로직 분리 원칙(프로젝트 원칙 유지):
//   "어떤 음을 어디에 둘지"(보이싱·옥타브 배치)는 아래 상수 테이블
//   kVoiceCenterOffset[] 하나만 고치면 바뀐다. 알고리즘 코드는 건드릴 필요 없음.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include <cmath>

namespace syn {

// ── MIDI ↔ 주파수 (구 MusicTheory.h 에서 이전) ──────────────────────────────
//   midi 는 float 도 허용(연속 피치). 69 = A4 = 440Hz 기준.
inline float midiToHz(float midi) {
    return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
}

// ── 연주 바 식별자 (보이스 인덱스와 1:1) ────────────────────────────────────
//   Bass   : 왼쪽 바        → 루트(1음) 중심  · 베이스 음역
//   Fifth  : 오른쪽 1번 바  → 5음 중심
//   Octave : 오른쪽 2번 바  → 8음(옥타브) 중심
//   Third  : 오른쪽 3번 바  → 3음 중심
enum BarVoice : int { Bass = 0, Fifth = 1, Octave = 2, Third = 3, kNumBarVoices = 4 };

// ── 보이싱 정의 (1·5·8·3) — ★ 보이싱을 바꾸려면 여기만 수정 ─────────────────
// "루트 기준음(rootRef)"으로부터 각 바 '가운데'에 올 음의 반음 오프셋.
//   rootRef 는 rootRefMidi() 가 정하는 중역(中域) 루트 (C3..B3, MIDI 48..59).
//
//   Bass   = rootRef - 12  (한 옥타브 아래 = 베이스 음역)
//   Fifth  = rootRef +  7  (완전5도)
//   Octave = rootRef + 12  (옥타브)
//   Third  = rootRef + 16  (장3도를 한 옥타브 올린 10도 → 1<5<8<3 으로 쌓이는 오픈보이싱)
//            ※ '닫힌 3도'(옥타브 아래)를 원하면 +16 → +4 로만 바꾸면 됨.
//
//  예) 링에서 E 선택(rootRef=E3=52) →
//        Bass=E2(40) · Fifth=B3(59) · Octave=E4(64) · Third=G#4(68)
//        ⇒ 오른쪽 세 바 가운데 = B, E, G#  (스펙과 일치)
constexpr int kVoiceCenterOffset[kNumBarVoices] = {
    /*Bass  */ -12,
    /*Fifth */  +7,
    /*Octave*/ +12,
    /*Third */ +16,
};

// 한 바에 매핑되는 전체 음역(반음). 12 = 한 옥타브.
//   가운데(0.5) 기준 ±6반음 → 한 바에서 12음계가 전부 도달 가능(스펙 요구).
constexpr float kBarSemitoneSpan = 12.0f;

// ── 5도권: 링 위치(0=12시 C) → 세그먼트(0..11) ──────────────────────────────
// 12등분. 세그먼트 i 의 피치클래스 = (i*7) mod 12   (5도 = 7반음).
//   C-G-D-A-E-B-F#-Db-Ab-Eb-Bb-F
//
// hysteresis(떨림 방지): 직전 세그먼트(prevSeg, 없으면 -1)를 넘기면 경계에서
//   margin(세그먼트 단위) 만큼은 직전 값을 유지해 손가락 미세 떨림에 의한
//   음 깜빡임을 막는다. (이산 선택이라도 부드럽게 래치)
inline int ringToSegment(float ringPos, int prevSeg = -1, float margin = 0.15f) {
    ringPos -= std::floor(ringPos);          // 0..1 래핑
    const float f = ringPos * 12.0f;         // 0..12 (연속 세그먼트 좌표)
    int seg = static_cast<int>(f) % 12;
    if (prevSeg >= 0 && prevSeg < 12) {
        float d = f - (prevSeg + 0.5f);      // 직전 세그먼트 중심으로부터 거리
        d -= 12.0f * std::floor(d / 12.0f + 0.5f);   // -6..6 으로 원형 래핑
        if (std::fabs(d) < 0.5f + margin) seg = prevSeg;  // 경계 근처면 유지
    }
    return seg;
}

inline int segmentToRootPc(int seg) { return (seg * 7) % 12; }

// 루트 피치클래스(0..11) → '중역 루트' MIDI (C3=48 기준, 48..59 로 접음).
//   어떤 5도를 골라도 보이싱이 일정한 음역에 머물도록 한 옥타브 안으로 정규화.
inline int rootRefMidi(int rootPc) { return 48 + (rootPc % 12); }

// 바 '가운데' 기준음(MIDI 정수). barVoice = Bass/Fifth/Octave/Third.
inline int voiceCenterMidi(int rootPc, int barVoice) {
    return rootRefMidi(rootPc) + kVoiceCenterOffset[barVoice];
}

// 바 위치(0..1) → 가운데 기준 반음 오프셋(연속, 등간격).
//   pos 0   (맨 위)   = -span/2   (가장 낮음)
//   pos 0.5 (가운데)  =  0        (기준음)
//   pos 1   (맨 아래) = +span/2   (가장 높음)
// "위로 갈수록 낮고 아래로 갈수록 높다"는 스펙을 그대로 구현.
inline float barPosToOffsetSemis(float pos) {
    return (pos - 0.5f) * kBarSemitoneSpan;
}

} // namespace syn
