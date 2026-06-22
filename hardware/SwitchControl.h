// SwitchControl.h — 스위치(2/3 포지션) 1개. 채터링 제거(디바운스) 포함.
#pragma once

namespace hw {

class SwitchControl {
public:
    // numPositions: 2 또는 3, debounceBlocks: 같은 값이 N블록 연속이어야 확정
    void configure(int numPositions, int debounceBlocks = 3);

    // 이번 블록에 디코딩된 포지션(0..numPositions-1)을 넣는다.
    // 반환: 디바운스 후 포지션이 바뀌었으면 true
    bool update(int rawPosition);

    int  position() const { return stable_; }

    // 디스플레이에서 강제 설정
    void setFromDisplay(int pos) { stable_ = pos; }

private:
    int numPositions_  = 2;
    int debounce_      = 3;
    int stable_        = 0;  // 확정된 포지션
    int candidate_     = 0;  // 후보 포지션
    int candidateCount_= 0;  // 후보가 연속된 횟수
    bool initialised_  = false;
};

} // namespace hw
