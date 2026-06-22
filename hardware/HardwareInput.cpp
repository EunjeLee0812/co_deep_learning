#include "HardwareInput.h"
#include <Bela.h>   // analogRead, multiplexerAnalogRead, digitalRead, pinMode, BelaContext

namespace hw {

// ===========================================================================
//  배선 매핑 테이블  ── 여기가 곧 "어느 핀에 무엇을 꽂는가" 다.
//  멀티플렉서 핀 표기 x.y : x=아날로그입력(0~7), y=그 입력의 멀티플렉스 채널(0~7).
//  아래 21개 노브는 아날로그 입력 0~5번에 4채널 멀티플렉싱(=32채널 설정)으로 분배함.
//  (멀티플렉서 없이 직결 시엔 최대 8개만 가능 → 반드시 캐플릿/외장 MUX 사용)
// ===========================================================================
const HardwareInput::PotMap HardwareInput::kPots[] = {
    // id                      analogIn  muxCh   (라벨 x.y)
    { ControlId::LfoRate,        0, 0 }, // 0.0
    { ControlId::LfoDelay,       0, 1 }, // 0.1
    { ControlId::DcoLfo,         0, 2 }, // 0.2
    { ControlId::DcoPwm,         0, 3 }, // 0.3
    { ControlId::DcoSub,         1, 0 }, // 1.0
    { ControlId::DcoNoise,       1, 1 }, // 1.1
    { ControlId::LpfFreq,        1, 2 }, // 1.2
    { ControlId::LpfRes,         1, 3 }, // 1.3
    { ControlId::LpfEnv,         2, 0 }, // 2.0
    { ControlId::LpfLfo,         2, 1 }, // 2.1
    { ControlId::LpfTrack,       2, 2 }, // 2.2
    { ControlId::Volume,         2, 3 }, // 2.3
    { ControlId::PitchDco,       3, 0 }, // 3.0
    { ControlId::PitchLfo,       3, 1 }, // 3.1
    { ControlId::ModLfo,         3, 2 }, // 3.2
    { ControlId::GlideTime,      3, 3 }, // 3.3
    { ControlId::EnvA,           4, 0 }, // 4.0
    { ControlId::EnvD,           4, 1 }, // 4.1
    { ControlId::EnvS,           4, 2 }, // 4.2
    { ControlId::EnvR,           4, 3 }, // 4.3
    { ControlId::VcaLevel,       5, 0 }, // 5.0   (5.1~7.3 여유: 추후 확장/EQ)
};

// 스위치 → 디지털 핀. 3포지션은 핀 2개로 디코딩.
// 주의: 디지털 핀은 0/3.3V! 5V 인가 금지. 스위치 한쪽은 GND, 다른쪽은 3.3V + 풀다운 권장.
const HardwareInput::SwitchMap HardwareInput::kSwitches[] = {
    { ControlId::DcoRange,       {0, 1}, 2 }, // 16'/8'/4'  (D0,D1)
    { ControlId::DcoPwmSource,   {2, 0}, 1 }, // LFO/MAN     (D2)
    { ControlId::DcoWaveSquare,  {3, 0}, 1 }, // square on/off(D3)
    { ControlId::DcoWaveSaw,     {4, 0}, 1 }, // saw on/off  (D4)
    { ControlId::LpfEnvPolarity, {5, 0}, 1 }, // +/-         (D5)
    { ControlId::Voicing,        {6, 7}, 2 }, // POLY1/2/UNI (D6,D7)
    { ControlId::VcaShape,       {8, 0}, 1 }, // ENV/GATE    (D8)
};

const int HardwareInput::kNumPots     = sizeof(kPots)/sizeof(kPots[0]);
const int HardwareInput::kNumSwitches = sizeof(kSwitches)/sizeof(kSwitches[0]);

// ---------------------------------------------------------------------------
bool HardwareInput::setup(BelaContext* context, ParameterSink* sink) {
    sink_ = sink;

    // 제어 주파수 = 블록당 1회 읽으므로 (오디오 샘플레이트 / 블록크기)
    const float controlRate = context->audioSampleRate /
                              (float)context->audioFrames;

    // 멀티플렉서 캐플릿 감지: multiplexerChannels 가 2/4/8 이면 활성.
    useMux_ = (context->multiplexerChannels > 1);
    if (!useMux_ && kNumPots > 8) {
        rt_printf("[HardwareInput] 경고: 노브 %d개인데 멀티플렉서 비활성 — "
                  "최대 8개만 읽힙니다. IDE Settings 에서 멀티플렉서를 켜세요.\n",
                  kNumPots);
    }

    // 아날로그 컨트롤 구성
    for (int i = 0; i < kNumPots; ++i)
        analog_[i].configure(getSpec(kPots[i].id), controlRate);

    // 스위치 구성
    for (int i = 0; i < kNumSwitches; ++i) {
        const ControlSpec& sp = getSpec(kSwitches[i].id);
        switches_[i].configure(sp.positions, /*debounceBlocks=*/3);
    }
    pinModesSet_ = false;
    return true;
}

// ---------------------------------------------------------------------------
void HardwareInput::process(BelaContext* context) {
    // 스위치 핀을 INPUT 으로 설정 (Bela 는 블록마다 핀모드를 지정).
    for (int i = 0; i < kNumSwitches; ++i)
        for (int p = 0; p < kSwitches[i].numPins; ++p)
            pinMode(context, 0, kSwitches[i].pins[p], INPUT);

    // ---- 노브/페이더 ----
    // index i 는 kPots 와 ControlId(앞 21개) 순서가 동일하므로 그대로 사용.
    for (int i = 0; i < kNumPots; ++i) {
        const float raw = readPot(context, kPots[i]);
        if (analog_[i].update(raw)) {            // 데드밴드 넘는 변화만
            emit(kPots[i].id, analog_[i].value());
        }
    }

    // ---- 스위치 ----
    for (int i = 0; i < kNumSwitches; ++i) {
        const int pos = readSwitchPosition(context, kSwitches[i]);
        if (switches_[i].update(pos)) {
            emit(kSwitches[i].id, (float)switches_[i].position());
        }
    }
}

// 한 노브의 raw(0~1). 멀티플렉서 유무를 여기서 흡수한다.
float HardwareInput::readPot(BelaContext* context, const PotMap& p) {
    if (useMux_) {
        // 멀티플렉서: 해당 아날로그입력의 특정 mux 채널의 최신 샘플.
        // (인자 순서는 설치된 Bela 버전에서 한번 확인 권장: analogIn, muxChannel)
        return multiplexerAnalogRead(context, p.analogIn, p.muxCh);
    }
    // 직결: 블록 첫 프레임에서 읽음. analogIn 0~7 만 유효.
    return analogRead(context, 0, p.analogIn);
    // TODO(외장 MUX 사용 시): 74HC4067 등을 쓰면 digitalWrite 로 select 핀(S0~S3)을
    //   세팅한 뒤 공통 출력 1개를 analogRead 하는 방식으로 이 함수만 교체하면 됨.
}

// 스위치 핀들을 읽어 포지션(0..n-1)으로 디코딩.
int HardwareInput::readSwitchPosition(BelaContext* context, const SwitchMap& s) {
    if (s.numPins == 1) {
        return digitalRead(context, 0, s.pins[0]) ? 1 : 0;
    }
    // 2핀 → 3포지션 디코딩. on-off-on 토글 가정:
    //   pin0=1,pin1=0 -> 0 / 둘다0(center) -> 1 / pin0=0,pin1=1 -> 2
    // (실제 스위치 배선에 맞게 이 매핑만 조정하면 됨)
    const int a = digitalRead(context, 0, s.pins[0]);
    const int b = digitalRead(context, 0, s.pins[1]);
    if (a && !b) return 0;
    if (!a && b) return 2;
    return 1; // center
}

// sink(엔진) + displaySink(디스플레이 피드백) 동시 통지
void HardwareInput::emit(ControlId id, float value) {
    if (sink_)        sink_->setParameter(static_cast<int>(id), value);
    if (displaySink_) displaySink_->setParameter(static_cast<int>(id), value);
    // 주의: 여기는 오디오 스레드다. displaySink_ 구현은 절대 블로킹/할당 금지.
    //   실제 디스플레이 전송은 lock-free 큐에 넣고 별도 스레드/태스크에서 보낼 것.
}

// ---------------------------------------------------------------------------
void HardwareInput::setFromDisplay(int controlId, float value) {
    const ControlId id = static_cast<ControlId>(controlId);
    if (controlId < kNumAnalogControls) {
        analog_[controlId].setValueFromDisplay(value);
    } else {
        const int si = switchIndex(id);
        if (si >= 0 && si < kNumSwitches)
            switches_[si].setFromDisplay((int)value);
    }
    // 디스플레이발 변경은 sink_(엔진)에 바로 반영해도 됨:
    if (sink_) sink_->setParameter(controlId, value);
    // displaySink_ 로는 되돌려 보내지 않는다(디스플레이가 이미 알고 있으므로 에코 방지).
}

float HardwareInput::getValue(int controlId) const {
    const ControlId id = static_cast<ControlId>(controlId);
    if (controlId < kNumAnalogControls)
        return analog_[controlId].value();
    const int si = static_cast<int>(id) - kNumAnalogControls;
    if (si >= 0 && si < kNumSwitches)
        return (float)switches_[si].position();
    return 0.0f;
}

} // namespace hw
