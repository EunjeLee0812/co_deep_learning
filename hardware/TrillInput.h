// TrillInput.h
// ───────────────────────────────────────────────────────────────────────────
// Trill 센서 입력 파트 (하드웨어 담당이 채울 부분).
//
// [신규 배치]  Trill Ring 1개 + Trill Bar 4개 (왼쪽 1 + 오른쪽 3)
//   Ring        : 5도권으로 "어떤 코드(베이스 루트)"인지 선택. 이산 + 래치.
//   Bar(왼쪽1)  : 베이스(1음) 중심. 한 옥타브를 연속으로.
//   Bar(오른3)  : 코드 보이싱. 가운데가 각각 5음 / 8음(옥타브) / 3음.
//
//   [하드웨어]  Trill Ring / Bar (I2C)
//        │  readI2C() → 터치 위치/개수/면적
//        ▼
//   [TrillInput::poll()]  ← 하드웨어 담당이 구현 (AuxiliaryTask 안에서)
//        │  TrillFrame 에 0..1 로 정리
//        ▼
//   [SynthEngine]  snapshot() 으로 한 블록에 한 번 읽어 루트/보이싱/발음 생성
//
// ★ 중요 (프로젝트 원칙):
//   Trill 은 I2C 라 readI2C() 가 느리다. 절대 오디오 render 스레드에서 직접 읽지 말 것.
//   Bela AuxiliaryTask 안에서 poll() 을 돌리고, 결과(TrillFrame)만 오디오 쪽이 읽는다.
//   단일 작성자(AuxiliaryTask) / 단일 독자(audio) 라 별도 락 없이 안전.
//
// ★ 바(Bar) 위치 방향 규약 (합성 파트가 이 약속에 의존함):
//   pos 0.0 = 맨 위(가장 낮은 음) · 0.5 = 가운데(기준음) · 1.0 = 맨 아래(가장 높은 음)
//   → 물리 장착 방향상 위/아래가 뒤집혀 있으면 poll() 에서 (1.0 - raw) 로 보정할 것.
//
// 합성 담당(이은제)은 아래 게터의 "의미"만 알면 된다. 전기/I2C 처리는 전부
// 하드웨어 담당이 .cpp 의 TODO 에 채운다. 게터 시그니처(=계약)는 바꾸지 말 것.
// ───────────────────────────────────────────────────────────────────────────
#pragma once

#include <Bela.h>

namespace hw {

// 한 연주 바(Trill Bar)의 터치 상태.
struct BarTouch {
    bool  active   = false; // 터치 중인가 = 게이트 온(이 바의 음이 울림). 떼면 릴리즈.
    float pos      = 0.0f;  // 0=맨위(낮음) · 0.5=가운데(기준음) · 1=맨아래(높음)
    float strength = 0.0f;  // 0..1 터치 면적(세기). [선택] 0 이면 엔진이 풀 볼륨 처리.
};

// 한 시점의 모든 연주 센서 상태를 0..1 로 정규화해 담는 스냅샷.
// 합성 엔진은 매 오디오 블록 시작에 snapshot() 으로 이 구조체 하나만 읽는다.
struct TrillFrame {
    // ── Trill Ring : 5도권(circle of fifths). 어떤 코드(베이스 루트)인가 ──
    //   ringActive : 링에 손가락이 닿아 있나 (닿아 있는 동안 루트가 실시간 갱신)
    //   ringPos    : 0.0 = 12시 방향(C), 시계방향 증가, 1.0 직전에서 한 바퀴.
    //                12등분 → 5도권 순서(C-G-D-A-E-B-F#-Db-Ab-Eb-Bb-F).
    //   ※ 손을 떼면(ringActive=false) 엔진이 마지막 루트를 래치(계속 유지)한다.
    bool  ringActive = false;
    float ringPos    = 0.0f;

    // ── 연주 바 4개 (왼쪽 1 + 오른쪽 3). 가운데(0.5)가 각 보이싱 기준음 ──
    //   bass : 왼쪽 바        → 루트(1음) 중심  · 베이스 음역
    //   r5   : 오른쪽 1번 바  → 5음 중심
    //   r8   : 오른쪽 2번 바  → 8음(옥타브) 중심
    //   r3   : 오른쪽 3번 바  → 3음 중심
    BarTouch bass;
    BarTouch r5;
    BarTouch r8;
    BarTouch r3;
};

class TrillInput {
public:
    // 오디오 시작 전 1회. I2C 버스/주소 설정, 5개 센서 초기화, AuxiliaryTask 생성 등.
    // 성공 시 true. (구현: TrillInput.cpp)
    bool setup(BelaContext* context);

    // 모든 Trill 센서를 readI2C() 로 읽어 내부 TrillFrame 을 갱신.
    // ★ 반드시 AuxiliaryTask 컨텍스트에서 호출 (오디오 스레드 아님).
    void poll();

    // 오디오 스레드가 한 블록에 한 번 읽는 스냅샷(현재 래치된 값 복사).
    TrillFrame snapshot() const { return frame_; }

    void cleanup();

    // ───────────────────────────────────────────────────────────────────────
    // get_* 게터 — 합성 담당이 쓰는 "의미 단위" API.
    // (snapshot() 으로 한꺼번에 받아도 되고, 개별 게터로 받아도 된다. 같은 값.)
    // ───────────────────────────────────────────────────────────────────────
    // 5도권 링: 코드(베이스 루트) 선택
    bool  get_Ring_active() const { return frame_.ringActive; }
    float get_Ring_pos()    const { return frame_.ringPos; }      // 0=12시(C)

    // 베이스 바(왼쪽)
    bool  get_BassBar_active() const { return frame_.bass.active; }
    float get_BassBar_pos()    const { return frame_.bass.pos; }  // 0=위(낮음)..1=아래(높음)

    // 오른쪽 바 3개 (5음 / 8음 / 3음 중심)
    bool  get_R5_active() const { return frame_.r5.active; }
    float get_R5_pos()    const { return frame_.r5.pos; }
    bool  get_R8_active() const { return frame_.r8.active; }
    float get_R8_pos()    const { return frame_.r8.pos; }
    bool  get_R3_active() const { return frame_.r3.active; }
    float get_R3_pos()    const { return frame_.r3.pos; }

private:
    TrillFrame frame_;  // poll() 이 갱신, snapshot()/get_* 가 읽음. (단일 작성자/단일 독자)
};

} // namespace hw
