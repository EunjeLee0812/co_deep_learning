// render.cpp — Bela 진입점 (setup / render / cleanup)
// 각 파트를 조립한다: 연주센서(Trill) + 패널(노브/스위치) + 디스플레이 → 신스엔진.
#include <Bela.h>
#include "synthesis/SynthEngine.h"
#include "hardware/HardwareInput.h"
#include "hardware/TrillInput.h"
#include "comm/CommHandler.h"

static SynthEngine       gSynth;
static hw::HardwareInput gHardware;  // 노브/스위치 (파라미터)
static hw::TrillInput    gTrill;     // 링/바    (연주)
static CommHandler       gComm;      // 디스플레이 수신

// Trill 은 I2C(느림) → 오디오 스레드와 분리된 AuxiliaryTask 에서 읽는다.
static AuxiliaryTask gTrillTask = nullptr;
static void trillReadLoop(void*) { gTrill.poll(); }

// 블록 렌더용 스테레오 버퍼 (Bela 블록은 보통 16~128 프레임)
static const unsigned int kMaxFrames = 1024;
static float gBufL[kMaxFrames];
static float gBufR[kMaxFrames];

bool setup(BelaContext* context, void* userData) {
    if (!gSynth.setup(context->audioSampleRate, context->audioFrames)) return false;
    if (!gHardware.setup(context, &gSynth)) return false; // 패널 → 엔진(ParameterSink)
    if (!gTrill.setup(context))             return false; // 연주 센서
    if (!gComm.setup())                     return false; // 디스플레이

    gTrillTask = Bela_createAuxiliaryTask(trillReadLoop, 50, "trill-read");
    if (!gTrillTask) return false;
    return true;
}

void render(BelaContext* context, void* userData) {
    const unsigned int frames = context->audioFrames;

    // 1) 디스플레이에서 들어온 파라미터 변경 반영
    gComm.poll(gSynth);
    // 2) 패널(노브/스위치) 입력 반영 — 내부에서 엔진 setParameter 호출
    gHardware.process(context);
    // 3) Trill I2C 읽기 예약(비동기). 결과는 다음 블록부터 snapshot 으로 반영됨.
    Bela_scheduleAuxiliaryTask(gTrillTask);
    // 4) 최신 연주센서 스냅샷으로 코드 트리거/해제
    gSynth.applyPerformance(gTrill.snapshot());

    // 5) 오디오 블록 생성 (보이스 → 이펙터 → 마스터볼륨)
    unsigned int n = frames;
    if (n > kMaxFrames) n = kMaxFrames;
    gSynth.render(gBufL, gBufR, n);

    // 6) 출력 채널에 기록 (ch0=L, ch1=R, 그 외=L)
    for (unsigned int i = 0; i < n; ++i) {
        for (unsigned int ch = 0; ch < context->audioOutChannels; ++ch) {
            float s = (ch == 1) ? gBufR[i] : gBufL[i];
            audioWrite(context, i, ch, s);
        }
    }
}

void cleanup(BelaContext* context, void* userData) {
    gComm.cleanup();
    gTrill.cleanup();
    gSynth.cleanup();
    // hw::HardwareInput 은 별도 cleanup() 없음 (RAII)
}
