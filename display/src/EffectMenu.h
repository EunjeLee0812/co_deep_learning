// EffectMenu.h — 디스플레이에 뜨는 8개 이펙트와 각 파라미터 정의.
//   화면쪽(컴포넌트 이름/번호, 그림 id)과 엔진쪽(EngineTarget, paramId, 범위)을 잇는 다리.
//   * 컴포넌트 이름/번호/그림 id 는 Nextion Editor 에서 만든 것과 반드시 일치해야 함 (README 참고)
#pragma once
#include "EffectParamSink.h"

namespace disp {

// 화면 아래쪽 슬라이더 1칸에 대응하는 파라미터 정의
struct ParamDef {
    const char* label;       // 표시 이름 (예: "PRE-DELAY")
    EngineTarget target;     // 어느 엔진 이펙트로 보낼지
    int         paramId;     // 그 이펙트 안에서의 paramId (오프셋 포함)
    float       min, max;    // 디스플레이 0~100 ↔ 실제값 매핑 범위
    const char* labelComp;   // 라벨 텍스트 컴포넌트 (예: "t0")
    const char* valueComp;   // 숫자 표시 컴포넌트 (예: "n0")
    const char* progComp;    // 프로그레스바 컴포넌트 (예: "j0")
};

// 이펙트 1개(육각형) 정의
struct EffectDef {
    const char*  name;        // "REVERB"
    EngineTarget target;
    unsigned char hexId;      // 육각형 버튼의 Nextion 컴포넌트 .id (터치 매칭용)
    const char*   hexComp;    // 육각형 버튼 이름 (예: "b5") — 그림 전환용
    int           picNormal;  // 일반 상태 그림 id
    int           picGlow;    // 선택(흰 글로우) 상태 그림 id
    ParamDef      params[4];  // 화면 하단 4칸에 들어갈 파라미터
    int           numParams;
};

class EffectMenu {
public:
    static constexpr int kNumEffects = 8;

    const EffectDef& effect(int index) const;          // 0..7
    int numEffects() const { return kNumEffects; }

    // 터치로 들어온 컴포넌트 .id 가 어느 이펙트인지 찾기 (없으면 -1)
    int effectIndexForComponent(unsigned char componentId) const;
};

} // namespace disp
