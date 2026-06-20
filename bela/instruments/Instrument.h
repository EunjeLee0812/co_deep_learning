// Instrument.h — 악기(음원/보이스) 공통 인터페이스
#pragma once

class Instrument {
public:
    virtual ~Instrument() = default;
    virtual void  noteOn(int midiNote, float velocity) = 0;
    virtual void  noteOff(int midiNote) = 0;
    virtual float process() = 0;                 // 한 샘플 생성
    virtual void  setParam(int paramId, float value) = 0;
};
