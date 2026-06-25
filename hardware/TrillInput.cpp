// TrillInput.cpp
// ───────────────────────────────────────────────────────────────────────────
// ★ 이 파일은 "하드웨어 담당"이 채우는 스켈레톤이다. ★
// 합성 담당(이은제)은 TrillFrame 의 의미(TrillInput.h)만 알면 되고,
// 아래 TODO 는 하드웨어 담당이 채운다. (스켈레톤 상태에서도 빌드/무음 실행됨)
//
// [신규 배치] Trill Ring 1 + Trill Bar 4 (왼쪽 1 = Bass, 오른쪽 3 = R5/R8/R3)
//
// 채워야 할 것:
//   1) 5개 Trill 센서를 I2C 로 setup (Ring=Trill::RING, Bar=Trill::BAR)
//   2) AuxiliaryTask 안에서 주기적으로 readI2C() → 위치/면적 읽기
//   3) raw 값을 TrillFrame 규약(0..1, 방향)에 맞춰 채우기
//      ★ 바 방향: pos 0=맨위(낮은음) / 0.5=가운데(기준) / 1=맨아래(높은음).
//        장착이 뒤집혀 있으면 (1.0f - raw) 로 보정.
//
// 참고: Bela Trill 라이브러리 (대략)
//   #include <libraries/Trill/Trill.h>
//   Trill ring; ring.setup(1 /*i2cBus*/, Trill::RING, 0x38 /*addr*/);
//   ring.readI2C();
//   unsigned int n = ring.getNumTouches();
//   float loc  = ring.touchLocation(0);  // 0..1
//   float size = ring.touchSize(0);      // 터치 면적(정규화 필요)
//
// I2C 주소는 센서마다 다르게(점퍼/펌웨어) 배정해야 5개를 한 버스에 물릴 수 있다.
// ───────────────────────────────────────────────────────────────────────────
#include "TrillInput.h"
// #include <Bela.h>
// #include <libraries/Trill/Trill.h>
#include <algorithm>
#include <cmath>

namespace hw {

// TODO(하드웨어): 실제 센서 객체/상태.
// 예: static Trill gRing, gBass, gR5, gR8, gR3;
//     static AuxiliaryTask gReadTask;

namespace {
// raw touchSize → 0..1 세기 정규화 기준 면적 (센서/펌웨어 의존, 실측 보정).
constexpr float kStrengthFullScale = 0.20f; // TODO(하드웨어): 실측값으로 교체
inline float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

// 한 Bar 센서를 읽어 BarTouch 로 채우는 헬퍼 패턴(주석 가이드).
// void readBar(Trill& s, BarTouch& out, bool flip) {
//     s.readI2C();
//     if (s.getNumTouches() > 0) {
//         out.active   = true;
//         float p      = clamp01(s.touchLocation(0));
//         out.pos      = flip ? (1.0f - p) : p;     // 위=낮음 규약 맞추기
//         out.strength = clamp01(s.touchSize(0) / kStrengthFullScale);
//     } else {
//         out.active = false; out.strength = 0.0f;
//     }
// }
} // namespace

bool TrillInput::setup(BelaContext* /*context*/) {
    // ── TODO(하드웨어) ──────────────────────────────────────────────────────
    // 1) 각 Trill 센서 setup. 실패하면 false.
    //    if (gRing.setup(1, Trill::RING, 0x38) != 0) return false;
    //    if (gBass.setup(1, Trill::BAR,  0x20) != 0) return false;
    //    if (gR5  .setup(1, Trill::BAR,  0x21) != 0) return false;
    //    if (gR8  .setup(1, Trill::BAR,  0x22) != 0) return false;
    //    if (gR3  .setup(1, Trill::BAR,  0x23) != 0) return false;
    //    각 센서: setMode(Trill::CENTROID);
    // 2) I2C 읽기용 AuxiliaryTask 생성 후 주기 스케줄.
    //
    // 스켈레톤: 센서 없어도 빌드/실행되도록 무음 기본 프레임.
    frame_ = TrillFrame{};
    return true;
}

void TrillInput::poll() {
    // ★ AuxiliaryTask 안에서 호출 (readI2C 가 블로킹).
    //
    // ── TODO(하드웨어) ──────────────────────────────────────────────────────
    // [Ring] 5도권 루트
    //   gRing.readI2C();
    //   if (gRing.getNumTouches() > 0) {
    //       frame_.ringActive = true;
    //       frame_.ringPos    = gRing.touchLocation(0);  // 0=12시(C) 되도록 오프셋 보정
    //   } else frame_.ringActive = false;   // 떼면 엔진이 마지막 루트 래치
    //
    // [Bars] 각 바: 위=낮음(pos 0) 규약. flip 은 실제 장착 방향 보고 결정.
    //   readBar(gBass, frame_.bass, /*flip=*/false);  // 왼쪽: 1음 중심
    //   readBar(gR5,   frame_.r5,   /*flip=*/false);  // 5음 중심
    //   readBar(gR8,   frame_.r8,   /*flip=*/false);  // 8음(옥타브) 중심
    //   readBar(gR3,   frame_.r3,   /*flip=*/false);  // 3음 중심
    //
    // (스켈레톤 상태에서는 아무것도 안 만짐 → 무음. 위 TODO 채우면 소리 남.)
    (void)kStrengthFullScale;
    (void)&clamp01;
}

void TrillInput::cleanup() {
    // TODO(하드웨어): 필요 시 센서/태스크 정리.
}

} // namespace hw
