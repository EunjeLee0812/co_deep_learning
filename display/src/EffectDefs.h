// EffectDefs.h — 디스플레이 UI 가 쓰는 "이펙트/파라미터 정의 테이블"
// ───────────────────────────────────────────────────────────────────────────
// 각 이펙트 헤더의 enum class Param 주석(단위/범위)이 최종 기준이고,
// 이 파일은 그것을 UI 관점(이름, 범위, 기본값, 이산/연속, 페이지 배치)으로
// 정리한 사본이다. 이펙트 쪽 Param 이 바뀌면 여기도 같이 고칠 것.
//
// 값 변환 규약:
//   Nextion 슬라이더는 0..1000 정수를 보낸다.
//   Bela 쪽에서  real = min + (raw/1000) * (max - min)  선형 변환.
//   steps > 0 인 파라미터(스위치/모드 선택)는  real = round(raw/1000*(steps-1)).
//   → Nextion 은 값의 "의미"를 몰라도 되고, 정의는 Bela 한 곳에만 있다.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "../../effects/PluginChain.h"

namespace disp {

struct ParamDef {
    const char* name;   // 디스플레이 라벨 (짧게)
    float       min;
    float       max;
    float       def;    // 기본값 (실제 단위)
    int         steps;  // 0=연속, N>=2 = N단계 이산 (0..N-1 정수 전송)
};

struct EffectDef {
    const char*     name;       // 아이콘 아래 표기
    int             category;   // 0=ChannelStrip, 1=Modulation, 2=Spaces
    int             numParams;
    const ParamDef* params;     // 인덱스 == 각 이펙트의 Param enum 값
};

// 카테고리 이름 (카테고리 선택 팝업)
[[maybe_unused]] static const char* kCategoryNames[3] = { "CH STRIP", "MODULATION", "SPACES" };
// 카테고리별 이펙트 (이펙트 선택 팝업의 버튼 순서)
[[maybe_unused]] static const fx::EffectType kCategoryEffects[3][3] = {
    { fx::EffectType::EQ,     fx::EffectType::Comp,    fx::EffectType::Dist   },
    { fx::EffectType::Chorus, fx::EffectType::Flanger, fx::EffectType::Phaser },
    { fx::EffectType::Reverb, fx::EffectType::Delay,   fx::EffectType::Delay  }, // Spaces 는 2개뿐 (3번째 버튼 숨김)
};
[[maybe_unused]] static const int kCategoryCount[3] = { 3, 3, 2 };

// ── 파라미터 테이블 (인덱스 = 각 이펙트 Param enum 과 1:1) ──────────────────

static const ParamDef kEqParams[] = {
    { "ENABLE",  0, 1, 1, 2 },        // 0/1
    { "HP FREQ", 20, 300, 80, 0 },    // Hz
    { "HP>SC",   0, 1, 0, 2 },        // 0=HP>EQ 1=HP>SC
    { "LO FREQ", 35, 300, 100, 0 },
    { "LO GAIN", -15, 15, 0, 0 },     // dB
    { "MID FREQ",250, 8000, 1000, 0 },
    { "MID GAIN",-15, 15, 0, 0 },
    { "MID BELL",0, 1, 1, 2 },
    { "HI FREQ", 4000, 16000, 8000, 0 },
    { "HI GAIN", -15, 15, 0, 0 },
};
static const ParamDef kCompParams[] = {
    { "ENABLE",  0, 1, 1, 2 },
    { "THRESH",  -50, 0, -18, 0 },    // dBFS
    { "RATIO",   0, 2, 0, 3 },        // 이산 3단: 아래 특수 매핑 참고(3/5/10)
    { "ATTACK",  0.1f, 100, 10, 0 },  // ms
    { "RELEASE", 10, 1000, 120, 0 },  // ms
    { "MIX",     0, 1, 1, 0 },
    { "MAKEUP",  0, 24, 0, 0 },       // dB
};
static const ParamDef kDistParams[] = {
    { "GAIN",    -24, 12, 0, 0 },     // dB (표시용 하한 -24)
    { "OUT",     -24, 12, 0, 0 },
    { "TYPE",    0, 2, 0, 3 },        // Tube/Soft/Hard
    { "EQ POS",  0, 2, 0, 3 },        // Off/Pre/Post
    { "EQ MORPH",0, 1, 0.5f, 0 },     // HP..BP..LP 연속
    { "EQ FREQ", 50, 12000, 800, 0 },
    { "EQ RESO", 0, 1, 0.2f, 0 },
    { "DRIVE",   0, 1, 0.3f, 0 },
    { "MIX",     0, 1, 1, 0 },
};
static const ParamDef kChorusParams[] = {
    { "IN/OUT",  0, 1, 1, 2 },
    { "RATE",    0.05f, 8, 0.8f, 0 }, // Hz (sync off 기준)
    { "SYNC",    0, 1, 0, 2 },
    { "DELAY 1", 5, 30, 12, 0 },      // ms
    { "DELAY 2", 5, 30, 18, 0 },
    { "DEPTH",   0, 1, 0.5f, 0 },
    { "FEEDBK",  0, 0.9f, 0.2f, 0 },
    { "LPF",     1000, 18000, 12000, 0 },
    { "MIX",     0, 1, 0.5f, 0 },
};
static const ParamDef kFlangerParams[] = {
    { "IN/OUT",  0, 1, 1, 2 },
    { "RATE",    0.05f, 8, 0.3f, 0 },
    { "SYNC",    0, 1, 0, 2 },
    { "DEPTH",   0, 1, 0.6f, 0 },
    { "FEEDBK",  0, 0.95f, 0.4f, 0 },
    { "PHASE",   0, 1, 0.5f, 0 },
    { "MIX",     0, 1, 0.5f, 0 },
};
static const ParamDef kPhaserParams[] = {
    { "IN/OUT",  0, 1, 1, 2 },
    { "RATE",    0.05f, 8, 0.4f, 0 },
    { "SYNC",    0, 1, 0, 2 },
    { "DEPTH",   0, 1, 0.6f, 0 },
    { "FREQ",    100, 5000, 800, 0 },
    { "FEEDBK",  0, 0.95f, 0.3f, 0 },
    { "PHASE",   0, 1, 0.5f, 0 },
    { "MIX",     0, 1, 0.5f, 0 },
};
static const ParamDef kReverbParams[] = {
    { "GAIN",    -24, 12, 0, 0 },
    { "OUT",     -24, 12, 0, 0 },
    { "TYPE",    0, 1, 0, 2 },        // Plate/Hall
    { "SIZE",    0, 1, 0.5f, 0 },
    { "DECAY",   0, 1, 0.5f, 0 },
    { "LOW CUT", 20, 1000, 80, 0 },
    { "HI CUT",  1000, 20000, 8000, 0 },
    { "SPIN",    0, 5, 0.5f, 0 },
    { "SPIN DP", 0, 1, 0.2f, 0 },
    { "MIX",     0, 1, 0.3f, 0 },
};
static const ParamDef kDelayParams[] = {
    { "GAIN",    -24, 12, 0, 0 },
    { "OUT",     -24, 12, 0, 0 },
    { "FEEDBK",  0, 0.98f, 0.35f, 0 },
    { "TEMPO",   20, 300, 120, 0 },   // BPM
    { "SYNC",    0, 1, 0, 2 },
    { "LINK",    0, 1, 1, 2 },
    { "PINGPONG",0, 1, 0, 2 },
    { "HI CUT",  1000, 20000, 10000, 0 },
    { "LOW CUT", 20, 2000, 100, 0 },
    { "MIX",     0, 1, 0.3f, 0 },
    { "L TIME",  1, 4000, 350, 0 },   // ms (sync off)
    { "L DIV",   0, 6, 2, 7 },        // 노트분할 (sync on)
    { "R TIME",  1, 4000, 350, 0 },
    { "R DIV",   0, 6, 2, 7 },
};

// EffectType 순서와 인덱스 일치할 것!
static const EffectDef kEffects[(int)fx::EffectType::Count] = {
    { "EQ",      0, 10, kEqParams      },
    { "COMP",    0,  7, kCompParams    },
    { "DIST",    0,  9, kDistParams    },
    { "CHORUS",  1,  9, kChorusParams  },
    { "FLANGER", 1,  7, kFlangerParams },
    { "PHASER",  1,  8, kPhaserParams  },
    { "REVERB",  2, 10, kReverbParams  },
    { "DELAY",   2, 14, kDelayParams   },
};

// 슬라이더 raw(0..1000) → 이펙트에 넣을 실제 값
inline float rawToReal(const ParamDef& p, int raw) {
    if (raw < 0)    raw = 0;
    if (raw > 1000) raw = 1000;
    const float t = raw / 1000.0f;
    if (p.steps >= 2) {
        int idx = (int)(t * (p.steps - 1) + 0.5f);
        return (float)idx;    // 이산: 0..steps-1 정수 (Ratio 는 아래 특수 처리)
    }
    return p.min + t * (p.max - p.min);
}
// 실제 값 → 슬라이더 raw(0..1000)  (초기 상태를 화면에 반영할 때)
inline int realToRaw(const ParamDef& p, float real) {
    float t;
    if (p.steps >= 2) t = (p.steps > 1) ? real / (float)(p.steps - 1) : 0.0f;
    else              t = (p.max > p.min) ? (real - p.min) / (p.max - p.min) : 0.0f;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return (int)(t * 1000.0f + 0.5f);
}

// Compressor::Param::Ratio 는 "비율값 자체(3/5/10)"를 받으므로 특수 매핑.
inline float compRatioFromIndex(int idx) {
    return (idx <= 0) ? 3.0f : (idx == 1 ? 5.0f : 10.0f);
}

// ── Nextion 그림 리소스 ID ──────────────────────────────────────────────────
// TODO(디스플레이 팀원): Nextion Editor 에 육각형 아이콘 이미지를 등록한 뒤,
// 실제 Picture ID 로 채울 것. [0]=일반, [1]=선택(글로우) 버전.
// 인덱스 = EffectType.  (배경 이미지 ID 는 kBgPic)
static const int kPicNormal[(int)fx::EffectType::Count]   = { 1, 2, 3, 4, 5, 6, 7, 8 };
static const int kPicSelected[(int)fx::EffectType::Count] = { 11,12,13,14,15,16,17,18 };
static const int kBgPic = 0;   // 메인 페이지 배경 전체 이미지 (드래그 잔상 지우기에 사용)

} // namespace disp
