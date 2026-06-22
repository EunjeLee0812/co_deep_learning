// MusicTheory.cpp — 코드 빌드 + 텐션 선택 + 보이싱 펼치기
#include "MusicTheory.h"
#include <algorithm>

namespace syn {
namespace {

inline int clampi(int v, int lo, int hi) { return std::min(hi, std::max(lo, v)); }
inline bool pcInMask(int pc, unsigned short mask) { return (mask >> (pc % 12)) & 1u; }

// 퀄리티별 3화음 인터벌(반음, 루트 기준)
void triadOffsets(Quality q, int out[3], int& n) {
    switch (q) {
        case Quality::Major:      out[0]=0; out[1]=4; out[2]=7; n=3; break;
        case Quality::Minor:      out[0]=0; out[1]=3; out[2]=7; n=3; break;
        case Quality::Augmented:  out[0]=0; out[1]=4; out[2]=8; n=3; break;
        case Quality::Diminished: out[0]=0; out[1]=3; out[2]=6; n=3; break;
    }
}

// ── 텐션 선택 ───────────────────────────────────────────────────────────────
// 도미넌트7 코드 위에 얹을 텐션을 "직전 코드 문맥"을 보고 고른다.
// 후보(루트 기준 반음, 상부구조 >12): ♭9=13, 9=14, ♯9=15, ♯11=18, ♭13=20, 13=21
//
// 점수 규칙(높을수록 선택):
//  (1) 공통음 유지: 텐션의 피치클래스가 "직전 코드"에 있었으면 +2.0
//      → 연속한 코드가 음을 공유 → 흐릿하고 모호한(jazzy) 연결.
//  (2) 루트 진행: 직전→현재 루트가 4도 상행(=완전5도 하행, V→I 해결)이면 변형텐션
//      (♭9/♯9/♭13)에 +1.5 (긴장→해결 느낌). 순차/그 외엔 자연텐션(9/13/♯11)에 +1.0.
//  (3) 첫 코드(문맥 없음): 9 와 13 에 약한 가산 (+0.8) — 부드럽고 색채감 있는 기본값.
//  (4) 미세 기본 선호(결정성): 9>13>♯11>♭9>♯9>♭13.
//
// nTensions = 1 + round(voicingWidth*2) → 1~3개. 보이싱이 넓을수록 텐션을 더 노출.
int chooseTensions(int rootPc, const PrevChordContext& prev,
                   float voicingWidth, int outOffsets[3]) {
    struct Cand { int off; float base; };
    // base = 미세 기본 선호 (4)
    const Cand cands[6] = {
        {14, 0.60f}, // 9
        {21, 0.50f}, // 13
        {18, 0.40f}, // #11
        {13, 0.30f}, // b9
        {15, 0.20f}, // #9
        {20, 0.10f}, // b13
    };
    const bool altered[6] = { false,false,false, true,true,true }; // b9,#9,b13 = 변형

    // 루트 진행 판정
    bool resolving = false; // V→I (4도 상행 = +5 반음)
    bool stepwise  = false;
    if (prev.valid && prev.rootPc >= 0) {
        int motion = ((rootPc - prev.rootPc) % 12 + 12) % 12;
        resolving = (motion == 5);
        stepwise  = (motion == 2 || motion == 10);
    }

    float score[6];
    for (int i = 0; i < 6; ++i) {
        float s = cands[i].base;
        int pc = (rootPc + cands[i].off) % 12;
        if (prev.valid && pcInMask(pc, prev.pcMask)) s += 2.0f;          // (1) 공통음
        if (prev.valid) {
            if (resolving &&  altered[i]) s += 1.5f;                     // (2) 해결→변형
            if ((stepwise || !resolving) && !altered[i]) s += 1.0f;      //     그외→자연
        } else {
            if (cands[i].off == 14 || cands[i].off == 21) s += 0.8f;     // (3) 첫코드 기본
        }
        score[i] = s;
    }

    int nWanted = clampi(1 + (int)std::lround(voicingWidth * 2.0f), 1, 3);

    // 점수 상위 nWanted 개 선택(중복 피치클래스 방지)
    int n = 0;
    bool used[6] = { false,false,false,false,false,false };
    unsigned short chosenPc = 0;
    for (int pick = 0; pick < nWanted; ++pick) {
        int best = -1; float bestScore = -1e9f;
        for (int i = 0; i < 6; ++i) {
            if (used[i]) continue;
            int pc = (rootPc + cands[i].off) % 12;
            if ((chosenPc >> pc) & 1u) continue; // 같은 피치클래스 텐션 중복 방지
            if (score[i] > bestScore) { bestScore = score[i]; best = i; }
        }
        if (best < 0) break;
        used[best] = true;
        chosenPc |= (unsigned short)(1u << ((rootPc + cands[best].off) % 12));
        outOffsets[n++] = cands[best].off;
    }
    std::sort(outOffsets, outOffsets + n);
    return n;
}

// ── 보이싱 펼치기 ───────────────────────────────────────────────────────────
// close-position 오프셋 목록 → 실제 MIDI 노트. width 로 옥타브 배치를 연다.
//  - width↑ : 텐션은 한 옥타브 위로 띄우고(상부구조), 안쪽 성부를 벌려 오픈 보이싱.
//  - 루트(오프셋 0)는 항상 최저 베이스로 고정(서브 오실레이터가 따라감).
void spread(int bassMidi, const int* offs, int nOff, float width, Chord& out) {
    out.clear();
    for (int k = 0; k < nOff; ++k) {
        int off  = offs[k];
        int lift = 0;
        bool isTension = (off >= 13);                 // 상부구조 텐션
        if (isTension && width >= 0.33f) lift += 12;  // 넓어지면 텐션을 위로 띄움
        if (k >= 2 && (k % 2 == 0) && width >= 0.66f) lift += 12; // 안쪽 성부 오픈
        int midi = bassMidi + off + lift;
        while (midi > 96) midi -= 12;                 // 음역 클램프
        out.add(midi);
    }
    // 정렬 + 정확히 같은 MIDI 중복 제거
    std::sort(out.notes.begin(), out.notes.begin() + out.count);
    int w = 0;
    for (int r = 0; r < out.count; ++r)
        if (w == 0 || out.notes[w-1] != out.notes[r]) out.notes[w++] = out.notes[r];
    out.count = w;
    out.bassMidi = (out.count > 0) ? out.notes[0] : bassMidi;
}

} // namespace

int rootPcFromRing(float ringPos) {
    // 0..1 을 12세그먼트로. 0=12시. 세그먼트 i 의 피치클래스 = (i*7)%12.
    float p = ringPos - std::floor(ringPos);            // wrap to 0..1
    int seg = (int)std::lround(p * 12.0f) % 12;          // 가장 가까운 세그먼트
    if (seg < 0) seg += 12;
    return (seg * 7) % 12;
}

Quality qualityFromBar(float pos) {
    int z = clampi((int)(pos * 4.0f), 0, 3);
    return static_cast<Quality>(z);
}

ChordType chordTypeFromBar(float pos) {
    int z = clampi((int)(pos * 6.0f), 0, 5);
    return static_cast<ChordType>(z);
}

Chord buildChord(int rootPc, Quality q, ChordType type, float voicingWidth,
                 const PrevChordContext& prev, int octaveBase) {
    // 1) close-position 오프셋 목록 구성
    int offs[kMaxChordNotes];
    int n = 0;

    if (type == ChordType::Power) {
        offs[n++] = 0;  // 루트
        offs[n++] = 7;  // 5도
        if (voicingWidth >= 0.5f) offs[n++] = 12; // 넓으면 루트 옥타브(파워코드 옥타브)
    } else {
        int tri[3], tn; triadOffsets(q, tri, tn);
        for (int i = 0; i < tn; ++i) offs[n++] = tri[i];

        switch (type) {
            case ChordType::Add9: offs[n++] = 14; break;             // +9
            case ChordType::Maj7: offs[n++] = 11; break;             // +장7
            case ChordType::Dom7: offs[n++] = 10; break;             // +단7(도미넌트)
            case ChordType::Dom7Tension: {
                offs[n++] = 10;                                       // 단7
                int t[3]; int tnum = chooseTensions(rootPc, prev, voicingWidth, t);
                for (int i = 0; i < tnum && n < kMaxChordNotes; ++i) offs[n++] = t[i];
                break;
            }
            default: break; // Triad
        }
    }

    // 2) 베이스 MIDI: octaveBase 옥타브에 루트 피치클래스 배치
    int bassMidi = octaveBase + rootPc;

    // 3) 펼치기
    Chord c;
    spread(bassMidi, offs, n, voicingWidth, c);
    c.rootPc = rootPc;
    return c;
}

PrevChordContext makeContext(const Chord& c) {
    PrevChordContext ctx;
    ctx.pcMask = 0;
    for (int i = 0; i < c.count; ++i)
        ctx.pcMask |= (unsigned short)(1u << (((c.notes[i] % 12) + 12) % 12));
    ctx.rootPc = c.rootPc;
    ctx.valid  = (c.count > 0);
    return ctx;
}

} // namespace syn
