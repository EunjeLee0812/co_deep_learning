// MusicTheory.h
// ───────────────────────────────────────────────────────────────────────────
// 연주 센서(링/바)의 0..1 값을 "어떤 음들을 울릴지"(MIDI 노트 묶음)로 바꾸는
// 음악 이론 계층. 순수 함수 중심 — 오디오/하드웨어 의존 없음.
//
//   링(5도권)   → root(베이스 음)
//   퀄리티 바    → Quality (M/m/aug/dim)
//   복잡도 바    → ChordType (power/triad/add9/maj7/dom7/dom7+tension)
//   보이싱 바    → 보이싱 폭(좁음↔넓음) → 옥타브 배치/오픈/텐션 노출
//
// 텐션 선택은 "직전 코드"를 기억해서 공통음(common tone)·루트 진행을 고려한다.
// (자세한 알고리즘 설명은 ChordPerformer / 동봉 설계노트 참고)
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include <cmath>
#include <array>

namespace syn {

// 한 코드가 가질 수 있는 최대 노트 수(루트+서브구조+텐션 + 보이싱 더블링 여유).
constexpr int kMaxChordNotes = 8;

// 코드 퀄리티 (가로 바: M / m / aug / dim)
enum class Quality : int { Major = 0, Minor = 1, Augmented = 2, Diminished = 3 };

// 코드 종류/복잡도 (왼쪽 세로 바: 아래→위)
enum class ChordType : int {
    Power = 0,     // 루트 + 5도 (3도 없음, 퀄리티 무시)
    Triad = 1,     // 3화음 (퀄리티대로)
    Add9  = 2,     // 3화음 + 9th
    Maj7  = 3,     // 3화음 + 장7도(11반음)
    Dom7  = 4,     // 3화음 + 단7도(10반음) → 도미넌트 성격
    Dom7Tension = 5// Dom7 + 텐션(9/♯11/13/♭9/♯9/♭13 중 문맥상 선택)
};

// 빌드 결과: 정렬된 MIDI 노트 목록 + 메타.
struct Chord {
    std::array<int, kMaxChordNotes> notes{}; // MIDI 노트 번호(낮은→높은)
    int   count   = 0;                        // 유효 노트 수
    int   rootPc  = 0;                        // 루트 피치클래스 0..11 (텐션 문맥용)
    int   bassMidi= 48;                       // 최저음(서브 오실레이터가 따라갈 음)

    void clear() { count = 0; }
    void add(int midi) { if (count < kMaxChordNotes) notes[count++] = midi; }
};

// 직전 코드의 피치클래스 집합(공통음 계산용). 12비트 마스크로 표현.
// bit p 가 1 이면 피치클래스 p 가 직전 코드에 있었다는 뜻.
struct PrevChordContext {
    unsigned short pcMask = 0; // 직전 코드의 피치클래스 집합
    int  rootPc   = -1;        // 직전 루트(-1 = 없음/첫 코드)
    bool valid    = false;
};

// ── MIDI ↔ 주파수 ──────────────────────────────────────────────────────────
inline float midiToHz(float midi) {
    return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
}

// ── 5도권: 링 위치(0=12시) → 루트 피치클래스(0..11) ─────────────────────────
// 12시 C 에서 시계방향 5도씩: C,G,D,A,E,B,F#,Db,Ab,Eb,Bb,F.
// 세그먼트 i 의 피치클래스 = (i * 7) mod 12.  (5도 = 7반음)
int rootPcFromRing(float ringPos);

// ── 바 위치 → 이산 선택 ─────────────────────────────────────────────────────
Quality   qualityFromBar(float pos);     // 4등분
ChordType chordTypeFromBar(float pos);   // 6등분

// ── 코드 빌드 (이 함수가 음악 이론의 심장) ──────────────────────────────────
//   rootPc      : 5도권에서 고른 루트 피치클래스(0..11)
//   q           : 퀄리티
//   type        : 복잡도/종류
//   voicingWidth: 0(좁음)~1(넓음). 옥타브 배치/오픈 보이싱/텐션 노출 정도.
//   prev        : 직전 코드 문맥(텐션 선택·공통음 유지에 사용)
//   octaveBase  : 베이스가 놓일 대략적 MIDI 영역(기본 C3=48 부근)
// 반환: 울릴 MIDI 노트 묶음.
Chord buildChord(int rootPc,
                 Quality q,
                 ChordType type,
                 float voicingWidth,
                 const PrevChordContext& prev,
                 int octaveBase = 48);

// 빌드된 코드로부터 다음 호출에 넘길 직전 문맥을 만든다.
PrevChordContext makeContext(const Chord& c);

} // namespace syn
