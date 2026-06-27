#include <Bela.h>
#include "HardwareInput.h"
#include "ParameterSink.h"
#include "TrillInput.h"  // 👈 Trill 헤더 추가!

// 1. 임시 엔진 (노브/스위치 값 출력용)
class DummyEngine : public hw::ParameterSink {
public:
    void setParameter(int controlId, float value) override {
        rt_printf("[노브/스위치] ID: %2d | 값: %.3f\n", controlId, value);
    }
};

// 2. 전역 변수 선언
hw::HardwareInput gHardware;
DummyEngine       gDummyEngine;
hw::TrillInput    gTrillInput;   // 👈 Trill 객체 추가!

bool setup(BelaContext *context, void *userData) {
    // 하드웨어 (MUX + 스위치) 초기화
    if(!gHardware.setup(context, &gDummyEngine)) {
        rt_printf("❌ 하드웨어 입력 초기화 실패!\n");
        return false;
    }

    // Trill 센서 초기화
    if(!gTrillInput.setup(context)) {
        rt_printf("❌ Trill 센서 초기화 실패! I2C 배선과 주소를 다시 확인해주세요.\n");
        // 센서 하나가 없어도 코드가 뻗지 않게 하려면 return false; 는 하지 않습니다.
    }

    rt_printf("✅ 하드웨어 & 터치 센서 통합 테스트 시작!\n");
    return true;
}

void render(BelaContext *context, void *userData) {
    // ---------------------------------------------------
    // 1. 기존 노브 & 스위치 처리
    // ---------------------------------------------------
    gHardware.process(context);

    // ---------------------------------------------------
    // 2. Trill 터치 데이터 읽기 및 출력
    // ---------------------------------------------------
    // 이번 블록의 터치 스냅샷(현재 래치된 값)을 가져옵니다.
    hw::TrillFrame tFrame = gTrillInput.snapshot();

    // 콘솔창 도배 방지를 위한 출력 타이머 (약 200블록 = 0.05~0.1초마다 1회 출력)
    static int printCounter = 0;
    printCounter++;

    if (printCounter >= 200) {
        printCounter = 0;

// 수정된 터치 확인 조건문: 새로운 5개의 센서 상태를 확인합니다.
        if (tFrame.ringActive || tFrame.bass.active || tFrame.r5.active || tFrame.r8.active || tFrame.r3.active) {
            
            // Ring 센서 상태 출력
            if (tFrame.ringActive) {
                rt_printf("  [Ring] 위치: %.3f\n", tFrame.ringPos);
            }

            // Bass 바 상태 출력
            if (tFrame.bass.active) {
                rt_printf("  [Bass] 위치: %.3f | 세기(면적): %.3f\n", tFrame.bass.pos, tFrame.bass.strength);
            }

            // R5 바 상태 출력
            if (tFrame.r5.active) {
                rt_printf("  [R5] 위치: %.3f | 세기(면적): %.3f\n", tFrame.r5.pos, tFrame.r5.strength);
            }

            // R8 바 상태 출력
            if (tFrame.r8.active) {
                rt_printf("  [R8] 위치: %.3f | 세기(면적): %.3f\n", tFrame.r8.pos, tFrame.r8.strength);
            }

            // R3 바 상태 출력
            if (tFrame.r3.active) {
                rt_printf("  [R3] 위치: %.3f | 세기(면적): %.3f\n", tFrame.r3.pos, tFrame.r3.strength);
            }                
            rt_printf("\n"); // 보기 좋게 줄바꿈
        }
    }
}

void cleanup(BelaContext *context, void *userData) {
    // 종료 시 Trill 백그라운드 태스크 안전하게 종료
    gTrillInput.cleanup();
}
