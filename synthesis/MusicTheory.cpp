// MusicTheory.cpp — 코드 빌드 + 텐션 선택(표+티켓 방식) + 보이싱 펼치기
#include "MusicTheory.h"
#include "CompTables.h"      // ★추가: 컴핑 확률 표(kBase / kClash / kCandOffset)
#include <algorithm>

namespace syn {
namespace {

inline int clampi(int v, int lo, int hi) { return std::min(hi, std::max(lo, v)); }
inline bool pcInMask(int pc, unsigned short mask) { return (mask >> (pc % 12)) & 1u; }

// ── 가벼운 난수 (실시간 안전, 헤더 의존 없음) ────────────────────────────────
// xorshift32. setup 때 시드 한 번. render 안에서 새로 만들 필요 없음.
inline unsigned int& rngState() { static unsigned int s = 0x1234567u; return s; }
inline float nextRand01() {
    unsigned int x = rngState();
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    rngState() = x;
    return (x & 0xFFFFFF) / 16777216.0f;   // 0..1
}

// 퀄리티별 3화음 인터벌(반음, 루트 기준)
void triadOffsets(Quality q, int out[3], int& n) {
    switch (q) {
        case Quality::Major:      out[0]=0; out[1]=4; out[2]=7; n=3; break;
        case Quality::Minor:      out[0]=0; out[1]=3; out[2]=7; n=3; break;
        case Quality::Augmented:  out[0]=0; out[1]=4; out[2]=8; n=3; break;
        case Quality::Diminished: out[0]=0; out[1]=3; out[2]=6; n=3; break;
    }
}

// 후보 오프셋 → 후보 인덱스(없으면 -1)
inline int candIndexOf(int off) {
    for (int i = 0; i < kCompN; ++i) if (kCandOffset[i] == off) return i;
    return -1;
}
// 변형텐션(b9=13, #9=15, b13=20) 여부
inline bool isAlteredOffset(int off) { return off==13 || off==15 || off==20; }
// 자연텐션(9=14, 13=21, #11=18) 여부
inline bool isNaturalOffset(int off) { return off==14 || off==21 || off==18; }

// ── 텐션 선택 (표 + 티켓 방식) ───────────────────────────────────────────────
// 옛 chooseTensions 의 손튜닝 점수표를 "데이터로 만든 확률표(kBase/kClash)"와
// "이전 코드 공통음(prev.pcMask)" 로 대체한 버전.
//
//  티켓 = pow(기본확률, sharpness)
//         × (이전 코드와 공통음이면 보너스)
//         × (해결 진행이면 변형텐션 / 그 외엔 자연텐션 보너스)
//         × (이미 고른 음들과의 충돌 정도 kClash)
//  → 티켓 비례 룰렛으로 하나 뽑고, 뽑을 때마다 다시 계산해서 반복.
//
//  ambiguity(0=또렷,1=모호) 가 두 가지를 정함:
//    - 몇 개나 더 얹을지(모호할수록 많이)
//    - sharpness(또렷할수록 큼 → 높은확률 음에 몰림 / 모호할수록 평탄)
//
//  fixedOffs/nFixed : buildChord 가 이미 놓은 화음 구성음(루트·3도·5도·b7).
//                     이것들과의 충돌도 티켓 계산에 반영한다.
int chooseTensions(int rootPc, const PrevChordContext& prev,
                   const int* fixedOffs, int nFixed,
                   float voicingWidth, float ambiguity,
                   int outOffsets[], int maxOut) {
    // 0) 이미 놓인 음 표시(충돌 계산용)
    bool chosen[kCompN] = { false };
    for (int k = 0; k < nFixed; ++k) {
        int ci = candIndexOf(fixedOffs[k]);
        if (ci >= 0) chosen[ci] = true;
    }

    // 1) 루트 진행 판정 (해결 V→I = 4도 상행 = +5 반음)
    bool resolving = false;
    if (prev.valid && prev.rootPc >= 0) {
        int motion = ((rootPc - prev.rootPc) % 12 + 12) % 12;
        resolving = (motion == 5);
    }

    // 2) ambiguity → 개수 / 날카로움
    if (ambiguity < 0.0f) ambiguity = 0.0f;
    if (ambiguity > 1.0f) ambiguity = 1.0f;
    int   nWanted   = clampi(1 + (int)std::lround(ambiguity*2.0f + voicingWidth*1.0f), 1, 4);
    float sharpness = 1.0f + (1.0f - ambiguity) * 3.0f;

    int nOut = 0;
    for (int pick = 0; pick < nWanted && nOut < maxOut; ++pick) {
        // (a) 후보별 티켓 계산
        float tickets[kCompN] = { 0 };
        float total = 0.0f;
        for (int i = 0; i < kCompN; ++i) {
            if (chosen[i]) continue;
            int off = kCandOffset[i];
            if (off < 13) continue;            // 텐션(>=13)만 추가 대상
            if (kBase[i] <= 0.0f) continue;

            float t = std::pow(kBase[i], sharpness);

            // 공통음 보너스: 이 음의 피치클래스가 직전 코드에 있었으면 ↑
            int pc = (rootPc + off) % 12;
            if (prev.valid && pcInMask(pc, prev.pcMask)) t *= 2.5f;

            // 진행 보너스
            if (prev.valid) {
                if (resolving && isAlteredOffset(off)) t *= 1.5f;
                if (!resolving && isNaturalOffset(off)) t *= 1.2f;
            }

            // 이미 고른 음들과의 충돌
            for (int j = 0; j < kCompN; ++j)
                if (chosen[j]) t *= kClash[i][j];

            tickets[i] = t;
            total += t;
        }
        if (total <= 0.0f) break;

        // (b) 티켓 비례 룰렛
        float draw = nextRand01() * total;
        float acc = 0.0f; int winner = -1;
        for (int i = 0; i < kCompN; ++i) {
            if (tickets[i] <= 0.0f) continue;
            acc += tickets[i];
            if (draw <= acc) { winner = i; break; }
        }
        if (winner < 0) break;

        chosen[winner] = true;
        outOffsets[nOut++] = kCandOffset[winner];
    }

    std::sort(outOffsets, outOffsets + nOut);
    return nOut;
}

// ── 보이싱 펼치기 (변경 없음) ────────────────────────────────────────────────
void spread(int bassMidi, const int* offs, int nOff, float width, Chord& out) {
    out.clear();
    for (int k = 0; k < nOff; ++k) {
        int off  = offs[k];
        int lift = 0;
        bool isTension = (off >= 13);
        if (isTension && width >= 0.33f) lift += 12;
        if (k >= 2 && (k % 2 == 0) && width >= 0.66f) lift += 12;
        int midi = bassMidi + off + lift;
        while (midi > 96) midi -= 12;
        out.add(midi);
    }
    std::sort(out.notes.begin(), out.notes.begin() + out.count);
    int w = 0;
    for (int r = 0; r < out.count; ++r)
        if (w == 0 || out.notes[w-1] != out.notes[r]) out.notes[w++] = out.notes[r];
    out.count = w;
    out.bassMidi = (out.count > 0) ? out.notes[0] : bassMidi;
}

} // namespace

int rootPcFromRing(float ringPos) {
    float p = ringPos - std::floor(ringPos);
    int seg = (int)std::lround(p * 12.0f) % 12;
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
                 const PrevChordContext& prev, float ambiguity, int octaveBase) {
    int offs[kMaxChordNotes];
    int n = 0;

    if (type == ChordType::Power) {
        offs[n++] = 0;
        offs[n++] = 7;
        if (voicingWidth >= 0.5f) offs[n++] = 12;
    } else {
        int tri[3], tn; triadOffsets(q, tri, tn);
        for (int i = 0; i < tn; ++i) offs[n++] = tri[i];

        switch (type) {
            case ChordType::Add9: offs[n++] = 14; break;
            case ChordType::Maj7: offs[n++] = 11; break;
            case ChordType::Dom7: offs[n++] = 10; break;
            case ChordType::Dom7Tension: {
                offs[n++] = 10;                                  // 단7
                int nFixed = n;                                  // 여기까지가 고정 구성음
                int t[kMaxChordNotes];
                int tnum = chooseTensions(rootPc, prev, offs, nFixed,
                                          voicingWidth, ambiguity,
                                          t, kMaxChordNotes - n);
                for (int i = 0; i < tnum && n < kMaxChordNotes; ++i) offs[n++] = t[i];
                break;
            }
            default: break; // Triad
        }
    }

    int bassMidi = octaveBase + rootPc;
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
