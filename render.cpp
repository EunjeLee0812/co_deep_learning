// render.cpp — Bela 진입점 (setup / render / cleanup)
// 각 파트를 조립한다: 연주센서(Trill) + 패널(노브/버튼) + Nextion 디스플레이 → 신스엔진.
//
// ===========================================================================
// [디스플레이 통합 버전 — 이은제]
//   신호 흐름:  Voice 4개 → PluginChain(동적 이펙트 체인) → 마스터볼륨 → 출력
//   제어 흐름:  노브/페이더  ──────────────────→ SynthEngine (신스 파라미터)
//               Nextion(UART /dev/ttyS4) ──→ ChainUiController → PluginChain
//
//   스레드 배치:
//     - 오디오 스레드(render): 합성 + 이펙트 process. 할당/시리얼 절대 없음.
//     - trill-read 태스크   : I2C 폴링 (매 블록 스케줄, 기존과 동일)
//     - display 태스크      : 5ms 루프로 시리얼 폴링 + 체인 편집 + 화면 갱신
//
//   [임시] 이어폰 보호용 출력 0.2배는 유지. 스피커 최종 배치 때 삭제할 것.
// ===========================================================================
#include <Bela.h>
#include <unistd.h>                      // usleep (display 루프)
#include "synthesis/SynthEngine.h"
#include "hardware/HardwareInput.h"
#include "hardware/TrillInput.h"
#include "display/src/ChainUiController.h"

// ── Nextion 이 연결된 시리얼 장치. CONNECTION_GUIDE.md 대로 오버레이를 올리면
//    UART4 = /dev/ttyS4 (P1.20=TX, P2.20=RX) 가 생긴다. 바꾸면 여기만 수정.
static const char* kNextionDevice = "/dev/ttyS4";
static const int   kNextionBaud   = 115200;   // Nextion HMI 쪽 bauds=115200 과 일치!

static SynthEngine             gSynth;
static hw::HardwareInput       gHardware;   // 노브/스위치 (신스 파라미터)
static hw::TrillInput          gTrill;      // 링/바 (연주)
static disp::ChainUiController gDisplay;    // Nextion ↔ 이펙트 체인

// Trill: I2C(느림) → 오디오 스레드와 분리된 AuxiliaryTask 에서 읽는다.
static AuxiliaryTask gTrillTask = nullptr;
static void trillReadLoop(void*) { gTrill.poll(); }

// 디스플레이: 한 번 스케줄되면 스스로 5ms 주기로 도는 루프 태스크.
//  (시리얼 read/write 는 블로킹 가능성이 있어 오디오 스레드에서 하면 안 됨)
static AuxiliaryTask gDisplayTask = nullptr;
static void displayLoop(void*) {
    while (!Bela_stopRequested()) {
        gDisplay.update();     // 수신 파싱 → 체인 편집 → 화면 갱신 → GC
        usleep(5000);          // 5ms — 터치 반응성과 CPU 사용의 균형점
    }
}

// 블록 렌더용 스테레오 버퍼
static const unsigned int kMaxFrames = 1024;
static float gBufL[kMaxFrames];
static float gBufR[kMaxFrames];

// [임시] 이어폰 보호용 출력 감쇠. 정식 배치에선 1.0 또는 이 줄 자체 삭제.
static const float kTestOutputGain = 0.2f;

bool setup(BelaContext* context, void* userData) {
    if (!gSynth.setup(context->audioSampleRate, context->audioFrames)) return false;
    if (!gHardware.setup(context, &gSynth)) return false;
    if (!gTrill.setup(context)) return false;

    // 디스플레이 시리얼. 실패해도 신스는 살린다(이펙트 없이 드라이로 동작).
    if (!gDisplay.setup(&gSynth.effects(), kNextionDevice, kNextionBaud)) {
        rt_printf("⚠️  Nextion 시리얼(%s) 열기 실패 — 디스플레이 없이 계속 진행\n",
                  kNextionDevice);
        rt_printf("    (CONNECTION_GUIDE.md 의 디바이스 트리 오버레이 단계를 확인)\n");
    } else {
        gDisplayTask = Bela_createAuxiliaryTask(displayLoop, 40, "nextion-ui");
        if (!gDisplayTask) return false;
        Bela_scheduleAuxiliaryTask(gDisplayTask);   // 루프 태스크: 1회만 스케줄
        rt_printf("🖥️  Nextion 연결됨 (%s @ %d)\n", kNextionDevice, kNextionBaud);
    }

    gTrillTask = Bela_createAuxiliaryTask(trillReadLoop, 50, "trill-read");
    if (!gTrillTask) return false;

    rt_printf("✅ 신스 + 동적 이펙트 체인 준비 완료 (최대 %d슬롯)\n",
              fx::PluginChain::kMaxSlots);
    return true;
}

void render(BelaContext* context, void* userData) {
    const unsigned int frames = context->audioFrames;

    // 1) 패널(노브/스위치) 입력 반영 — 내부에서 gSynth.setParameter 호출
    gHardware.process(context);

    // 2) Trill I2C 읽기 예약(비동기)
    Bela_scheduleAuxiliaryTask(gTrillTask);

    // 3) 최신 연주센서 스냅샷으로 코드 트리거/해제
    gSynth.applyPerformance(gTrill.snapshot());

    // 4) 오디오 블록 생성: 보이스 → PluginChain(디스플레이로 편집된 체인) → 마스터
    unsigned int n = frames;
    if (n > kMaxFrames) n = kMaxFrames;
    gSynth.render(gBufL, gBufR, n);

    // 5) 출력 (ch0=L, ch1=R)
    for (unsigned int i = 0; i < n; ++i) {
        for (unsigned int ch = 0; ch < context->audioOutChannels; ++ch) {
            float s = (ch == 1) ? gBufR[i] : gBufL[i];
            s *= kTestOutputGain;                 // [임시] 정식 버전에선 삭제
            audioWrite(context, i, ch, s);
        }
    }
}

void cleanup(BelaContext* context, void* userData) {
    gDisplay.cleanup();
    gTrill.cleanup();
    gSynth.cleanup();   // 내부에서 PluginChain::cleanup() → 모든 인스턴스 해제
}
