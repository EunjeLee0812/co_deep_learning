#include "SwitchControl.h"

namespace hw {

void SwitchControl::configure(int numPositions, int debounceBlocks) {
    numPositions_ = numPositions;
    debounce_     = debounceBlocks;
    initialised_  = false;
}

bool SwitchControl::update(int rawPosition) {
    if (rawPosition < 0) rawPosition = 0;
    if (rawPosition >= numPositions_) rawPosition = numPositions_ - 1;

    if (!initialised_) {            // 첫 블록은 그대로 확정
        stable_ = candidate_ = rawPosition;
        candidateCount_ = debounce_;
        initialised_ = true;
        return true;
    }

    if (rawPosition == candidate_) {
        if (candidateCount_ < debounce_) ++candidateCount_;
    } else {
        candidate_ = rawPosition;   // 새 후보 등장 → 카운트 리셋
        candidateCount_ = 1;
    }

    // 후보가 충분히 안정됐고, 확정값과 다르면 확정 갱신
    if (candidateCount_ >= debounce_ && candidate_ != stable_) {
        stable_ = candidate_;
        return true;
    }
    return false;
}

} // namespace hw
