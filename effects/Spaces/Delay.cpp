#include "Delay.h"

namespace fx {

bool Delay::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    left_.delaySamples.setup(sampleRate, 50.0f);   // 딜레이타임 변경 시 50ms 램프
    right_.delaySamples.setup(sampleRate, 50.0f);
    left_.delaySamples.snap(msToSamples(333.0f));
    right_.delaySamples.snap(msToSamples(333.0f));
    // TODO: 좌우 링버퍼 할당 (최소 4초 분량), LFO/EQ 초기화
    reset();
    return true;
}

void Delay::process(float* left, float* right, unsigned int numFrames) {
    if (bypassed_) return;
    for (unsigned int n = 0; n < numFrames; ++n) {
        // TODO(구현자): 좌우 각각
        //   readPos = writePos - delaySamples.next()  (분수지연 보간)
        //   wet = ringRead(readPos)
        //   crossfeed/spin/blur 적용
        //   ringWrite(input + wet*feedback)
        //   out = wet (필요 시 dry 믹스)  + EQ
        (void)left; (void)right; // 자리표시
    }
}

void Delay::setParameter(int paramId, float value) {
    switch (static_cast<Param>(paramId)) {
        case Param::TempoBpm:      tempoBpm_      = value; break;
        case Param::SyncEnable:    syncEnabled_   = (value >= 0.5f); break;
        case Param::SpinCrossfeed: spinCrossfeed_ = (value >= 0.5f); break;
        case Param::PitchPreSpin:  pitchPreSpin_  = (value >= 0.5f); break;
        case Param::DelayBlur:     delayBlur_     = value; break;
        case Param::LinkSpin:      linkSpin_      = (value >= 0.5f); applyLink(); break;
        case Param::LinkOffset:    linkOffset_    = (value >= 0.5f); applyLink(); break;
        case Param::LinkEq:        linkEq_        = (value >= 0.5f); applyLink(); break;

        case Param::LeftDelayMs:     left_.delaySamples.setTarget(msToSamples(value)); break;
        case Param::LeftNoteDiv:     left_.noteDiv = static_cast<NoteDiv>((int)value); break;
        case Param::LeftOffsetMs:    left_.offsetMs = value; break;
        case Param::LeftSpin:        left_.spin = value; break;
        case Param::LeftFeedback:    left_.feedback = value; break;
        case Param::LeftEqEnable:    left_.eqEnabled = (value >= 0.5f); break;
        case Param::LeftEqMidFreqHz: left_.eqFreqHz = value; break;
        case Param::LeftEqGainDb:    left_.eqGainDb = value; break;
        case Param::LeftEqQ:         left_.eqQ = value; break;

        case Param::RightDelayMs:     right_.delaySamples.setTarget(msToSamples(value)); break;
        case Param::RightNoteDiv:     right_.noteDiv = static_cast<NoteDiv>((int)value); break;
        case Param::RightOffsetMs:    right_.offsetMs = value; break;
        case Param::RightSpin:        right_.spin = value; break;
        case Param::RightFeedback:    right_.feedback = value; break;
        case Param::RightEqEnable:    right_.eqEnabled = (value >= 0.5f); break;
        case Param::RightEqMidFreqHz: right_.eqFreqHz = value; break;
        case Param::RightEqGainDb:    right_.eqGainDb = value; break;
        case Param::RightEqQ:         right_.eqQ = value; break;
        default: break;
    }
    if (syncEnabled_) {
        // TODO: tempo/noteDiv/offset 로 좌우 delaySamples 재계산
    }
}

void Delay::applyLink() {
    // TODO: linkSpin_/linkOffset_/linkEq_ 가 true 면 right_ 의 해당 값을 left_ 값으로 동기화
}

void Delay::reset() {
    // TODO: 링버퍼 0으로, LFO 위상 초기화
}

void Delay::cleanup() {
    // TODO: 버퍼 해제
}

} // namespace fx
