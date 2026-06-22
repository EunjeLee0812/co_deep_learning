// TrillInput.h
// ───────────────────────────────────────────────────────────────────────────
// Trill 센서 입력 파트 (하드웨어 담당이 채울 부분).
//
// 이 클래스는 "연주용 센서"(Trill Ring 1개 + Trill Bar 3개)의 위치/세기를
// 0..1 정규화된 값으로 합성 엔진에 넘겨주는 단일 창구다.
//
//   [하드웨어]  Trill Ring / Bar (I2C)
//        │  readI2C() → 터치 위치/개수/면적
//        ▼
//   [TrillInput::poll()]  ← 하드웨어 담당이 구현 (AuxiliaryTask 안에서)
//        │  TrillFrame 에 0..1 로 정리
//        ▼
//   [SynthEngine]  snapshot() 으로 한 블록에 한 번 읽어 코드/보이싱 생성
//
// ★ 중요 (프로젝트 원칙과 동일):
//   Trill 은 I2C 라 readI2C() 가 느리다. 절대 오디오 render 스레드에서 직접 읽지 말 것.
//   Bela AuxiliaryTask 안에서 poll() 을 돌리고, 결과(TrillFrame)만 오디오 쪽이 읽는다.
//   단일 작성자(AuxiliaryTask) / 단일 독자(audio) 라서 별도 락 없이 안전하다.
//
// 합성 담당(이은제)은 아래 get_* 게터의 "의미"만 알면 된다. 전기/I2C 처리는 전부
// 하드웨어 담당이 .cpp 의 TODO 에 채운다. 게터 시그니처(=계약)는 바꾸지 말 것.
// ───────────────────────────────────────────────────────────────────────────
#pragma once

#include <Bela.h>

namespace hw {

// 한 시점의 모든 연주 센서 상태를 0..1 로 정규화해 담는 스냅샷.
// 합성 엔진은 매 오디오 블록 시작에 snapshot() 으로 이 구조체 하나만 읽는다.
struct TrillFrame {
    // ── Trill Ring : 5도권(circle of fifths). 어떤 음을 베이스(root)로 할지 ──
    //   ringActive : 링에 손가락이 닿아 있나 (코드 root 가 유효한가)
    //   ringPos    : 0.0 = 12시 방향(C), 시계방향으로 증가, 1.0 직전에서 한 바퀴.
    //                12등분해서 5도권 순서(C-G-D-A-E-B-F#-Db-Ab-Eb-Bb-F)로 매핑됨.
    bool  ringActive = false;
    float ringPos    = 0.0f;

    // ── 가로 Trill Bar : 코드 퀄리티 (사진의 "M  m  Aug  dim") ──
    //   qualityPos : 0.0(왼쪽) → 1.0(오른쪽). 4등분: Major / minor / aug / dim.
    bool  qualityActive = false;
    float qualityPos    = 0.0f;

    // ── 왼쪽 세로 Trill Bar : 코드 복잡도/종류 ──
    //   complexityPos : 0.0(맨 아래) → 1.0(맨 위). 6등분:
    //     0 Power(파워코드)  1 Triad(3화음)  2 Add9
    //     3 Maj7            4 Dom7          5 Dom7 + Tension(텐션)
    bool  complexityActive = false;
    float complexityPos    = 0.0f;

    // ── 오른쪽 세로 Trill Bar : 보이싱 폭 + 세기 (★ 발음 트리거 바) ──
    //   voicingActive   : 이 바를 만지는 동안 = 게이트 온(코드가 울림). 떼면 릴리즈.
    //   voicingPos      : 0.0(맨 아래, 좁은 보이싱) → 1.0(맨 위, 넓은 보이싱)
    //   voicingStrength : 0.0~1.0. Trill 의 터치 "면적(touchSize)"을 정규화한 값.
    //                     세게(넓게) 누를수록 1.0 에 가깝고 → 더 큰 소리(벨로시티).
    bool  voicingActive   = false;
    float voicingPos      = 0.0f;
    float voicingStrength = 0.0f;
};

class TrillInput {
public:
    // 오디오 시작 전 1회. I2C 버스/주소 설정, 4개 센서 초기화, AuxiliaryTask 생성 등.
    // 성공 시 true. (구현: TrillInput.cpp)
    bool setup(BelaContext* context);

    // 모든 Trill 센서를 readI2C() 로 읽어 내부 TrillFrame 을 갱신.
    // ★ 반드시 AuxiliaryTask 컨텍스트에서 호출 (오디오 스레드 아님).
    //   하드웨어 담당이 TrillInput.cpp 에서 구현한다.
    void poll();

    // 오디오 스레드가 한 블록에 한 번 읽는 스냅샷(현재 래치된 값 복사).
    TrillFrame snapshot() const { return frame_; }

    void cleanup();

    // ───────────────────────────────────────────────────────────────────────
    // get_* 게터 — 합성 담당이 쓰는 "의미 단위" API.
    // (snapshot() 으로 한꺼번에 받아도 되고, 개별 게터로 받아도 된다. 같은 값.)
    // 하드웨어 담당은 poll() 안에서 아래 의미에 맞게 frame_ 필드만 채우면 된다.
    // ───────────────────────────────────────────────────────────────────────

    // 5도권 링: 베이스 음 선택
    bool  get_TrillRing_active() const { return frame_.ringActive; }
    float get_TrillRing_pos()    const { return frame_.ringPos; }      // 0=12시(C)

    // 가로 바: 코드 퀄리티 (M/m/aug/dim)
    bool  get_QualityBar_active() const { return frame_.qualityActive; }
    float get_QualityBar_pos()    const { return frame_.qualityPos; }  // 0..1 좌→우

    // 왼쪽 세로 바: 코드 복잡도 (power→triad→add9→M7→dom7→dom7+tension)
    bool  get_ComplexityBar_active() const { return frame_.complexityActive; }
    float get_ComplexityBar_pos()    const { return frame_.complexityPos; } // 0..1 아래→위

    // 오른쪽 세로 바: 보이싱 폭 + 세기 (발음 트리거)
    bool  get_VoicingBar_active()   const { return frame_.voicingActive; }   // 터치=게이트온
    float get_VoicingBar_pos()      const { return frame_.voicingPos; }      // 0..1 좁→넓
    float get_VoicingBar_strength() const { return frame_.voicingStrength; } // 0..1 세기

private:
    TrillFrame frame_;  // poll() 이 갱신, snapshot()/get_* 가 읽음. (단일 작성자/단일 독자)
};

} // namespace hw
