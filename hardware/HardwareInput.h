// HardwareInput.h
// 하드웨어 입력 파트 최상위. render() 안에서 매 블록 process() 를 호출하면:
//   - 모든 노브/페이더를 (직결 또는 멀티플렉서로) 읽어 파라미터로 변환
//   - 스위치를 디바운스해 포지션으로 변환
//   - "바뀐 것만" ParameterSink 로 통지(엔진이 소리 갱신)
//   - 동시에 디스플레이 피드백 sink 로도 통지(터치 UI를 물리 노브와 동기화)
//   - 디스플레이가 보낸 값은 setFromDisplay() 로 수용
#pragma once
#include "ControlIds.h"
#include "AnalogControl.h"
#include "SwitchControl.h"
#include "ParameterSink.h"

#include <Bela.h>

namespace hw {

class HardwareInput {
public:
    // sink        : 파라미터 변경을 받을 엔진(필수)
    // 반환        : 초기화 성공 여부
    bool setup(BelaContext* context, ParameterSink* sink);

    // 매 오디오 블록마다 호출. 내부에서 모든 컨트롤을 읽고 변경분을 통지.
    void process(BelaContext* context);

    // 디스플레이 → 하드웨어(값 주입). 프리셋 로드/터치 조작 시.
    void setFromDisplay(int controlId, float value);

    // 하드웨어 → 디스플레이 피드백을 받을 sink(선택). 물리 노브가 움직이면
    // 여기로도 통지되어 comm 레이어가 디스플레이로 전달할 수 있다.
    void setDisplayFeedbackSink(ParameterSink* s) { displaySink_ = s; }

    // 현재 값 조회 (실제 단위 / 스위치는 포지션)
    float getValue(int controlId) const;

private:
    // ---- 물리 배선 매핑 (이 두 테이블이 곧 "배선도"다. 핀만 바꾸면 됨) ----
    struct PotMap   { ControlId id; unsigned char analogIn; unsigned char muxCh; };
    struct SwitchMap{ ControlId id; unsigned char pins[2]; unsigned char numPins; };

    static const PotMap    kPots[];
    static const SwitchMap kSwitches[];
    static const int kNumPots;
    static const int kNumSwitches;

    AnalogControl analog_[kNumAnalogControls];
    SwitchControl switches_[static_cast<int>(ControlId::Count) - kNumAnalogControls];

    ParameterSink* sink_        = nullptr; // 엔진
    ParameterSink* displaySink_ = nullptr; // 디스플레이 피드백(선택)
    bool  useMux_   = false;               // 멀티플렉서 캐플릿 사용 여부
    bool  pinModesSet_ = false;

    // -- 외장 MUX 제어를 위해 추가된 변수들 --
    int currentMuxCh_ = 0;
	int prevMuxCh_ = 0;
    static constexpr int kMuxPins[4] = {0, 1, 2, 3}; // D0, D1, D2, D3 핀 번호

    // -- 20260626 이상준 추가 --
    // 한 노브의 raw(0~1) 읽기 — 직결/멀티플렉서를 추상화
    float readPot(BelaContext* context, const PotMap& p);
    // 스위치 핀들을 읽어 포지션으로 디코딩
    int   readSwitchPosition(BelaContext* context, const SwitchMap& s);

    void emit(ControlId id, float value); // sink + displaySink 통지 헬퍼
    int  switchIndex(ControlId id) const { return static_cast<int>(id) - kNumAnalogControls; }
};

} // namespace hw
