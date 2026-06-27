// TrillInput.cpp
#include "TrillInput.h"
#include <Bela.h>
#include <libraries/Trill/Trill.h>
#include <algorithm>
#include <cmath>

namespace hw {

// 1. 실제 센서 객체 및 백그라운드 태스크 선언 (센서 5개로 변경)
static Trill gRing;
static Trill gBass;
static Trill gR5;
static Trill gR8;
static Trill gR3;
static AuxiliaryTask gReadTask;

namespace {
    // 터치 면적을 0.0 ~ 1.0 벨로시티로 정규화할 때 쓸 기준값
    constexpr float kStrengthFullScale = 0.05f; // 손가락 면적에 맞게 나중에 조절하세요.

    inline float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

    // Bar 센서 반복 읽기 코드를 줄여주는 헬퍼 함수
    void readBar(Trill& s, hw::BarTouch& out, bool flip) {
        s.readI2C();
        if (s.getNumTouches() > 0) {
            out.active   = true;
            float p      = clamp01(s.touchLocation(0));
            out.pos      = flip ? (1.0f - p) : p; // flip=true면 위아래 반전
            out.strength = clamp01(s.touchSize(0) / kStrengthFullScale);
        } else {
            out.active   = false;
            out.strength = 0.0f;
        }
    }

    // 2. 백그라운드에서 무한히 센서를 읽어올 루프 함수
    // (I2C 통신은 느려서 오디오 스레드에서 직접 읽으면 소리가 끊깁니다)
    void readLoop(void* arg) {
        TrillInput* input = static_cast<TrillInput*>(arg);
        while(!Bela_stopRequested()) {
            input->poll();  // 실제 센서 읽기 호출
            usleep(10000);  // 10ms 대기 (초당 약 100회 읽기, CPU 부담 완화)
        }
    }
} // namespace

bool TrillInput::setup(BelaContext* /*context*/) {
    // 3. 각 센서 초기화 (I2C 버스 1번 사용)
    // 주소(Address)는 하드웨어 점퍼/펌웨어 세팅과 일치해야 합니다.
    // [임시] Ring 미사용 — 주석처리. (정식 버전선 아래 줄 주석 풀기) [원복]
    // if (gRing.setup(1, Trill::RING, 0x38) != 0) { rt_printf("Ring 센서 연결 실패!\n"); return false; }
    //if (gBass.setup(1, Trill::BAR,  0x20) != 0) { rt_printf("Bass 바 연결 실패!\n"); return false; }
    if (gR5.setup(1, Trill::BAR,    0x21) != 0) { rt_printf("R5 바 연결 실패!\n"); return false; }
    //if (gR8.setup(1, Trill::BAR,    0x22) != 0) { rt_printf("R8 바 연결 실패!\n"); return false; }
    //if (gR3.setup(1, Trill::BAR,    0x23) != 0) { rt_printf("R3 바 연결 실패!\n"); return false; }

    // 센서 모드를 위치와 면적을 둘 다 읽는 CENTROID 모드로 설정
    // gRing.setMode(Trill::CENTROID);   // [임시] Ring 미사용 [원복]
    //gBass.setMode(Trill::CENTROID);
    gR5.setMode(Trill::CENTROID);
    //gR8.setMode(Trill::CENTROID);
    //gR3.setMode(Trill::CENTROID);

    // 4. 백그라운드 태스크 생성 및 실행
    gReadTask = Bela_createAuxiliaryTask(readLoop, 50, "trill-read", this);
    Bela_scheduleAuxiliaryTask(gReadTask);

    frame_ = TrillFrame{};
    rt_printf("✅ Trill R5 바(0x21) 1개 초기화 완료!\n");  // [임시] 원래 5개
    return true;
}

void TrillInput::poll() {
    // 5. 각 센서의 상태를 읽어서 프레임(frame_)에 0.0 ~ 1.0 값으로 예쁘게 담습니다.
    
    // [임시] Ring 미사용 — 항상 비활성으로 둠. (정식 버전선 아래 블록 주석 풀기) [원복]
    frame_.ringActive = false;
    // gRing.readI2C();
    // if (gRing.getNumTouches() > 0) {
    //     frame_.ringActive = true;
    //     frame_.ringPos = gRing.touchLocation(0);
    // } else {
    //     frame_.ringActive = false;
    // }

    // [Bars] 각 바 읽기
    // 센서를 하드웨어에 달았을 때 위아래가 뒤집혀 있다면 false를 true로 바꿔주세요.
    //
    // ★ 주의: setup()에서 초기화한 센서만 읽어야 함. 초기화 안 된 Trill 객체를
    //   readI2C() 하면 안 되므로, 지금은 R5 만 읽고 나머지는 비활성으로 둔다.
    readBar(gR5, frame_.r5, false);   // [임시] R5(0x21) 하나만 사용

    // [원복] 나머지 바를 setup()에서 켠 뒤 아래 주석을 풀 것:
    // readBar(gBass, frame_.bass, false);
    // readBar(gR8,   frame_.r8,   false);
    // readBar(gR3,   frame_.r3,   false);

    // [임시] 미사용 바는 명시적으로 비활성 (이전 잔상값 방지)
    frame_.bass.active = false;
    frame_.r8.active   = false;
    frame_.r3.active   = false;
}

void TrillInput::cleanup() {
    // 프로그램 종료 시 특별히 처리할 것은 없습니다.
}

} // namespace hw
