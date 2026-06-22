// Types.h — 공통 타입/상수
#pragma once
#include <cstdint>

namespace core {
    using ParamId = uint16_t;           // shared/protocol 와 일치시킬 것
    constexpr int kMaxVoices = 16;      // 추후 확정
}
