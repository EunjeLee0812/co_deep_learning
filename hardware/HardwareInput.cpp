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
    // id                      analogIn  muxCh
    { ControlId::LfoRate,        0, 0 },
    { ControlId::LfoDelay,       0, 1 },
    { ControlId::DcoLfo,         0, 2 },
    { ControlId::DcoPwm,         0, 3 },
    { ControlId::DcoSub,         0, 4 },
    { ControlId::DcoNoise,       0, 5 },
    { ControlId::LpfFreq,        0, 6 },
    { ControlId::LpfRes,         0, 7 },
    { ControlId::LpfEnv,         0, 8 },
    { ControlId::LpfLfo,         0, 9 },
    { ControlId::LpfTrack,       0, 10 },
    { ControlId::Volume,         0, 11 },
    { ControlId::PitchDco,       0, 12 },
    { ControlId::PitchLfo,       0, 13 },
    { ControlId::ModLfo,         0, 14 },
    //{ ControlId::GlideTime,      0, 15 }, // 사용중인 MUX는 15채널까지밖에 없음. (근데 0~14만 사용중이라 얘는 주석처리함)

    //{ ControlId::EnvA,           4, 0 }, // 4.0  여기부터는 폐기?해야할듯?
    //{ ControlId::EnvD,           4, 1 }, // 4.1
    //{ ControlId::EnvS,           4, 2 }, // 4.2
    //{ ControlId::EnvR,           4, 3 }, // 4.3
    //{ ControlId::VcaLevel,       5, 0 }, // 5.0   (5.1~7.3 여유: 추후 확장/EQ)
};

// 스위치 → 디지털 핀. 3포지션은 핀 2개로 디코딩.
// 주의: 디지털 핀은 0/3.3V! 5V 인가 금지. 스위치 한쪽은 GND, 다른쪽은 3.3V + 풀다운 권장.
const HardwareInput::SwitchMap HardwareInput::kSwitches[] = {
    //{ ControlId::DcoRange,       {8, 1}, 1 }, // 16'/8'/4'  (D8)
    //{ ControlId::DcoPwmSource,   {9, 0}, 1 }, // LFO/MAN     (D9)
    //{ ControlId::DcoWaveSquare,  {10, 0}, 1 }, // square on/off(D10)
    //{ ControlId::DcoWaveSaw,     {11, 0}, 1 }, // saw on/off  (D11)
    //{ ControlId::LpfEnvPolarity, {12, 0}, 1 }, // +/-         (D12)

    //{ ControlId::Voicing,        {6, 7}, 2 }, // POLY1/2/UNI (D6,D7) 폐기?
    //{ ControlId::VcaShape,       {8, 0}, 1 }, // ENV/GATE    (D8)    폐기?
};

const int HardwareInput::kNumPots     = sizeof(kPots)/sizeof(kPots[0]);
const int HardwareInput::kNumSwitches = sizeof(kSwitches)/sizeof(kSwitches[0]);

// --------------------------------------------------------------------------- 20260626 이상준 수정
bool HardwareInput::setup(BelaContext* context, ParameterSink* sink) {
    sink_ = sink;

    const float controlRate = context->audioSampleRate / (float)context->audioFrames;
    useMux_ = false; // 외장 MUX를 직접 제어하므로 내부 매직 기능은 끕니다.

    // 아날로그 컨트롤 구성 (ControlId의 고유 인덱스 번호 영역에 안전하게 매핑)
    for (int i = 0; i < kNumPots; ++i) {
        int idIdx = static_cast<int>(kPots[i].id);
        analog_[idIdx].configure(getSpec(kPots[i].id), controlRate);
    }

    // 스위치 구성
    for (int i = 0; i < kNumSwitches; ++i) {
        const ControlSpec& sp = getSpec(kSwitches[i].id);
        int si = switchIndex(kSwitches[i].id);
        switches_[si].configure(sp.positions, /*debounceBlocks=*/3);
    }
    pinModesSet_ = false;
    return true;
}

// --------------------------------------------------  20260626 이상준 수정
// 추가: bela의 기준전압은 4.087V인데, 실제로는 3.3V로 인가하고 있어서 0~1이 아니라 0~0.8까지만 올라가기에 0~0.8을 0~1에 강제매핑함. (mux가 3.3V라 3.3V밖에 못함)
void HardwareInput::process(BelaContext* context) {
    // 1) 똑딱이 스위치 핀 INPUT 설정
    for (int i = 0; i < kNumSwitches; ++i) {
        for (int p = 0; p < kSwitches[i].numPins; ++p) {
            pinMode(context, 0, kSwitches[i].pins[p], INPUT);
        }
    }

    // 2) MUX 제어 핀 OUTPUT 설정
    for (int p = 0; p < 4; ++p) {
        pinMode(context, 0, kMuxPins[p], OUTPUT);
    }

    // 3) 더미 리드 (노이즈 방지용)
    int dummyFrame = context->analogFrames - 2;
    if (dummyFrame >= 0) analogRead(context, dummyFrame, 0); 

    // 4) 진짜 전압 읽기 
    int lastAnalogFrame = context->analogFrames - 1;
    float raw = analogRead(context, lastAnalogFrame, 0); 

    // 👇 이 3줄을 적용하여 0.0 ~ 0.8을 0.0 ~ 1.0으로 쫙 늘려줍니다.
    raw = raw / 0.805f;  // (콘솔에서 확인하신 최댓값이 0.805라면 0.805f 로 적어주세요)
    if (raw > 1.0f) raw = 1.0f;
    if (raw < 0.0f) raw = 0.0f;

    // ==========================================================
    // 5) [핵심] 현재 채널이 아닌, '이전 채널(prevMuxCh_)'을 기준으로 매핑합니다.
    // ==========================================================
    for (int i = 0; i < kNumPots; ++i) {
        if (kPots[i].muxCh == prevMuxCh_) {  // <--- 여기가 교정 포인트!
            int idIdx = static_cast<int>(kPots[i].id);
            if (analog_[idIdx].update(raw)) {            
                emit(kPots[i].id, analog_[idIdx].value());
            }
            break; 
        }
    }

    // ==========================================================
    // 6) [핵심] 다음 블록을 위해 MUX 핀을 변경하고 기억합니다.
    // ==========================================================
    prevMuxCh_ = currentMuxCh_; // 방금 세팅할 채널을 "이전 채널"로 기억해둠
    currentMuxCh_ = (currentMuxCh_ + 1) % 15; // 다음 타겟으로 이동
    
    for (int p = 0; p < 4; ++p) {
        int bit = (currentMuxCh_ >> p) & 1;
        digitalWrite(context, 0, kMuxPins[p], bit); // 실제 물리 핀 변경 (다음 블록에서 읽힘)
    }

    // 7) ---- 똑딱이 스위치 신호 처리 (D8 ~ D12) ----
    for (int i = 0; i < kNumSwitches; ++i) {
        const int pos = readSwitchPosition(context, kSwitches[i]);
        int si = switchIndex(kSwitches[i].id);
        if (switches_[si].update(pos)) {
            emit(kSwitches[i].id, (float)switches_[si].position());
        }
    }
}

// 한 노브의 raw(0~1). 멀티플렉서 유무를 여기서 흡수한다.       << 얘는 이제 없어도 될듯?
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
    rt_printf("컨트롤 ID: %d, 현재 값: %.3f\n", static_cast<int>(id), value);	// <<<<<<<<<<<<<<< 여기 디버깅위해 코드 추가함
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
