// render.cpp — Bela 진입점 (setup / render / cleanup)
// 각 파트를 조립한다: 연주센서(Trill) + 패널(노브/스위치) + 디스플레이 → 신스엔진.
//
// ===========================================================================
// [임시 테스트 버전 — 이은제]
//   목적: 가변저항/페이더로 신스 파라미터를 제어하고, Trill Bar(0x21=R5) 하나만
//         눌렀을 때 피치에 맞는 신스 소리가 나는지 확인.
//   - 디스플레이(gComm) 비활성화 (지금 하드웨어에 연결 안 됨)
//   - Trill 은 R5(0x21) 한 개만 사용 (TrillInput.cpp 에서 Ring/나머지 주석처리됨)
//   - 노브 값이 바뀔 때만 "노브 값 변경됨" 로그 1회 출력
//   - 이어폰 보호용으로 최종 출력에 0.2 곱함
//
//   ▶ 나중에 정식 버전으로 되돌릴 때: 아래 [원복] 표시된 줄들의 주석을 풀고,
//     [임시] 표시된 줄들을 지우면 된다.
// ===========================================================================
#include <Bela.h>
#include "synthesis/SynthEngine.h"
#include "hardware/HardwareInput.h"
#include "hardware/TrillInput.h"
#include "hardware/ControlIds.h"   // [임시] 노브 이름 로그(getSpec().name)용 — 정식 버전선 불필요
// #include "comm/CommHandler.h"   // [원복] 디스플레이 다시 쓸 때 주석 해제

// ---------------------------------------------------------------------------
// [임시] 노브 변경 로그용 래퍼 ParameterSink.
//   엔진(gSynth)으로 값을 그대로 흘려보내면서, 동시에 콘솔에 1줄 찍는다.
//   AnalogControl 의 데드밴드 덕분에 "값이 실제로 변했을 때만" 호출된다.
// ---------------------------------------------------------------------------
class LoggingSink : public hw::ParameterSink {
public:
    void setTarget(hw::ParameterSink* t) { target_ = t; }
    void setParameter(int controlId, float value) override {
        // 노브 이름을 ControlIds 스펙에서 가져와 출력 (ID 0~14만 유효)
        const char* name = "?";
        if (controlId >= 0 && controlId < static_cast<int>(hw::ControlId::Count))
            name = hw::getSpec(static_cast<hw::ControlId>(controlId)).name;
        rt_printf("🎛️  [노브 값 변경됨] %-14s (ID:%2d) = %.3f\n", name, controlId, value);

        if (target_) target_->setParameter(controlId, value); // 엔진으로 전달(소리 반영)
    }
private:
    hw::ParameterSink* target_ = nullptr;
};

static SynthEngine       gSynth;
static hw::HardwareInput gHardware;  // 노브/스위치 (파라미터)
static hw::TrillInput    gTrill;     // 링/바    (연주)
// static CommHandler    gComm;      // [원복] 디스플레이 수신 — 다시 쓸 때 주석 해제
static LoggingSink       gLogSink;   // [임시] 노브 로그 래퍼

// Trill 은 I2C(느림) → 오디오 스레드와 분리된 AuxiliaryTask 에서 읽는다.
static AuxiliaryTask gTrillTask = nullptr;
static void trillReadLoop(void*) { gTrill.poll(); }

// 블록 렌더용 스테레오 버퍼 (Bela 블록은 보통 16~128 프레임)
static const unsigned int kMaxFrames = 1024;
static float gBufL[kMaxFrames];
static float gBufR[kMaxFrames];

// [임시] 이어폰 보호용 출력 감쇠. 정식 버전에선 마스터볼륨 노브가 담당하므로 1.0 으로.
static const float kTestOutputGain = 0.2f;

bool setup(BelaContext* context, void* userData) {
    if (!gSynth.setup(context->audioSampleRate, context->audioFrames)) return false;

    // [임시] 패널 → (로그래퍼) → 엔진.  gLogSink 가 중간에서 로그 찍고 엔진에 전달.
    gLogSink.setTarget(&gSynth);
    if (!gHardware.setup(context, &gLogSink)) return false;
    // [원복] 정식 버전에서는 아래 한 줄로 되돌린다 (로그 래퍼 제거):
    // if (!gHardware.setup(context, &gSynth)) return false;

    if (!gTrill.setup(context)) return false; // 연주 센서 (R5 0x21 하나만)

    // [원복] 디스플레이 다시 쓸 때 주석 해제:
    // if (!gComm.setup()) return false;

    gTrillTask = Bela_createAuxiliaryTask(trillReadLoop, 50, "trill-read");
    if (!gTrillTask) return false;

    rt_printf("✅ [테스트] 노브 돌리면 로그 출력 / Trill Bar(0x21) 누르면 소리 출력\n");
    return true;
}

void render(BelaContext* context, void* userData) {
    const unsigned int frames = context->audioFrames;

    // 1) [원복] 디스플레이에서 들어온 파라미터 변경 반영 — 다시 쓸 때 주석 해제:
    // gComm.poll(gSynth);

    // 2) 패널(노브/스위치) 입력 반영 — 내부에서 setParameter(=gLogSink) 호출
    gHardware.process(context);

    // 3) Trill I2C 읽기 예약(비동기). 결과는 다음 블록부터 snapshot 으로 반영됨.
    Bela_scheduleAuxiliaryTask(gTrillTask);

    // 4) 최신 연주센서 스냅샷으로 코드 트리거/해제
    //    (R5 바만 살아있으므로 보이스 1번[Fifth]만 게이트 온/오프 된다)
    gSynth.applyPerformance(gTrill.snapshot());

    // 5) 오디오 블록 생성 (보이스 → 이펙터 → 마스터볼륨)
    unsigned int n = frames;
    if (n > kMaxFrames) n = kMaxFrames;
    gSynth.render(gBufL, gBufR, n);

    // 6) 출력 채널에 기록 (ch0=L, ch1=R, 그 외=L)
    //    [임시] kTestOutputGain(0.2) 곱해 이어폰 음량 낮춤.
    for (unsigned int i = 0; i < n; ++i) {
        for (unsigned int ch = 0; ch < context->audioOutChannels; ++ch) {
            float s = (ch == 1) ? gBufR[i] : gBufL[i];
            s *= kTestOutputGain;                 // [임시] 정식 버전에선 이 줄 삭제
            audioWrite(context, i, ch, s);
        }
    }
}

void cleanup(BelaContext* context, void* userData) {
    // gComm.cleanup();   // [원복] 디스플레이 다시 쓸 때 주석 해제
    gTrill.cleanup();
    gSynth.cleanup();
    // hw::HardwareInput 은 별도 cleanup() 없음 (RAII)
}
