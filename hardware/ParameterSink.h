// ParameterSink.h
// 하드웨어(노브/스위치)와 디스플레이 둘 다 "파라미터가 바뀌었다"를 같은 방식으로
// 엔진에 알린다. 이 인터페이스를 synthesis 엔진이 구현하면, 하드웨어 입력과
// 디스플레이 입력이 동일한 파라미터 공간으로 합류한다.
//
//   하드웨어 노브 움직임 ─┐
//                         ├─> ParameterSink::setParameter(id, value) ─> 엔진
//   디스플레이 설정 ──────┘
#pragma once

namespace hw {

class ParameterSink {
public:
    virtual ~ParameterSink() = default;

    // controlId : ControlIds.h 의 ControlId enum 정수값
    // value     : 실제 단위로 스케일된 값 (Hz, 초, 0~1 등 — ControlSpec 의 단위)
    virtual void setParameter(int controlId, float value) = 0;
};

} // namespace hw
