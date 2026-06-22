// TrillInput.cpp
// ───────────────────────────────────────────────────────────────────────────
// ★ 이 파일은 "하드웨어 담당"이 채우는 스켈레톤이다. ★
// 합성 담당(이은제)은 TrillFrame 의 의미만 알면 되고, 아래 TODO 는 건드리지 않는다.
//
// 채워야 할 것:
//   1) 4개 Trill 센서(Ring 1 + Bar 3)를 I2C 로 setup
//   2) AuxiliaryTask 안에서 주기적으로 readI2C() → 위치/면적 읽기
//   3) 각 센서 raw 값을 TrillFrame 규약(0..1, 의미)에 맞춰 채우기
//
// 참고: Bela Trill 라이브러리 API (대략)
//   #include <libraries/Trill/Trill.h>
//   Trill ring; ring.setup(1 /*i2cBus*/, Trill::RING, 0x38 /*addr*/);
//   ring.readI2C();
//   unsigned int n = ring.getNumTouches();
//   float loc  = ring.touchLocation(0);  // 0..1 (RING 은 원주 위 위치)
//   float size = ring.touchSize(0);      // 터치 면적(센서/펌웨어 의존, 정규화 필요)
//   BAR 도 동일하되 Trill::BAR, touchLocation 이 막대 길이 방향 0..1.
//
// I2C 주소는 센서마다 다르게 점퍼/펌웨어로 설정해야 4개를 한 버스에 물릴 수 있다.
// (Ring/Bar 기본 주소 충돌 주의 — 하드웨어 담당이 주소 배정.)
// ───────────────────────────────────────────────────────────────────────────
#include "TrillInput.h"
// #include <Bela.h>
// #include <libraries/Trill/Trill.h>
#include <algorithm>
#include <cmath>

namespace hw {

// TODO(하드웨어): 실제 센서 객체/상태를 여기에 둔다.
// 예시(주석): static Trill gRing, gQuality, gComplexity, gVoicing;
//             static AuxiliaryTask gReadTask;

namespace {
// raw touchSize 를 0..1 세기로 정규화할 때 쓸 기준 면적.
// 센서/펌웨어마다 스케일이 달라서 실측 후 보정 필요.
constexpr float kStrengthFullScale = 0.20f; // TODO(하드웨어): 실측값으로 교체

inline float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }
} // namespace

bool TrillInput::setup(BelaContext* /*context*/) {
    // ── TODO(하드웨어) ──────────────────────────────────────────────────────
    // 1) 각 Trill 센서 setup. 실패하면 false 반환.
    //    if (gRing.setup(1, Trill::RING, 0x38) != 0) return false;
    //    if (gQuality.setup(1, Trill::BAR,  0x20) != 0) return false;
    //    ... (Complexity, Voicing 도)
    //    각 센서: setMode(Trill::CENTROID); 권장 (위치/개수/면적 모드)
    //
    // 2) I2C 읽기용 AuxiliaryTask 생성 (오디오 스레드와 분리).
    //    gReadTask = Bela_createAuxiliaryTask(readLoop, 50, "trill-read");
    //    그리고 setup 마지막 또는 render 첫 블록에서 Bela_scheduleAuxiliaryTask(gReadTask);
    //
    // 일단 센서가 없어도 빌드/실행은 되도록 기본 프레임을 둔다(무음).
    frame_ = TrillFrame{};
    return true;
}

void TrillInput::poll() {
    // ★ AuxiliaryTask 안에서 호출된다고 가정 (readI2C 가 블로킹이므로).
    //
    // ── TODO(하드웨어) ──────────────────────────────────────────────────────
    // 아래는 "이렇게 채우면 된다"는 가이드. 실제 Trill 호출로 교체.
    //
    // [Ring] 5도권 베이스
    //   gRing.readI2C();
    //   if (gRing.getNumTouches() > 0) {
    //       frame_.ringActive = true;
    //       frame_.ringPos    = gRing.touchLocation(0); // 0..1, 0=12시가 되도록
    //                                                   // 물리 장착 각도 오프셋이 있으면
    //                                                   // 여기서 보정: pos = wrap01(raw - offset)
    //   } else frame_.ringActive = false;
    //
    // [Quality Bar] M/m/aug/dim
    //   gQuality.readI2C();
    //   if (gQuality.getNumTouches() > 0) {
    //       frame_.qualityActive = true;
    //       frame_.qualityPos    = clamp01(gQuality.touchLocation(0));
    //   } else frame_.qualityActive = false;
    //
    // [Complexity Bar] power→…→dom7+tension (아래=0, 위=1)
    //   gComplexity.readI2C();
    //   if (gComplexity.getNumTouches() > 0) {
    //       frame_.complexityActive = true;
    //       // 물리적으로 "아래쪽"이 touchLocation 0 인지 1 인지 장착 방향에 따라 뒤집기:
    //       frame_.complexityPos = clamp01(gComplexity.touchLocation(0));
    //       // 만약 위가 0 이면: frame_.complexityPos = 1.0f - clamp01(...);
    //   } else frame_.complexityActive = false;
    //
    // [Voicing Bar] 보이싱 폭 + 세기 (발음 트리거)
    //   gVoicing.readI2C();
    //   if (gVoicing.getNumTouches() > 0) {
    //       frame_.voicingActive   = true;
    //       frame_.voicingPos      = clamp01(gVoicing.touchLocation(0)); // 아래=0, 위=1
    //       frame_.voicingStrength = clamp01(gVoicing.touchSize(0) / kStrengthFullScale);
    //       // 여러 손가락이면 면적 합/최댓값 등으로 세기 정의 가능(취향).
    //   } else {
    //       frame_.voicingActive   = false;
    //       frame_.voicingStrength = 0.0f;
    //   }
    //
    // ──────────────────────────────────────────────────────────────────────
    // (스켈레톤 상태에서는 아무것도 안 만진다 → 무음. 위 TODO 채우면 소리 남.)
    (void)kStrengthFullScale;
    (void)&clamp01;
}

void TrillInput::cleanup() {
    // TODO(하드웨어): 필요 시 센서/태스크 정리.
}

} // namespace hw
