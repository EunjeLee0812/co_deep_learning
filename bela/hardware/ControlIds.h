// ControlIds.h
// Model 84 패널을 기준으로 한 신스 파라미터(컨트롤) 정의.
// 이 enum 은 하드웨어와 디스플레이가 공유하는 "파라미터 ID 공간"이다.
// (추후 shared/protocol 로 옮겨 디스플레이 쪽과 단일 출처로 관리하면 좋음)
#pragma once

namespace hw {

// ---- 연속(노브/페이더) + 이산(스위치) 컨트롤 전체 목록 ----
// 앞쪽(LfoRate~VcaLevel)은 아날로그 노브/페이더, 뒤쪽은 스위치다.
enum class ControlId : int {
    // LFO
    LfoRate = 0,    // LFO 속도
    LfoDelay,       // LFO 페이드인 딜레이
    // DCO (오실레이터)
    DcoLfo,         // LFO -> 피치 변조량(비브라토)
    DcoPwm,         // 펄스폭 변조량
    DcoSub,         // 서브 오실레이터 레벨
    DcoNoise,       // 노이즈 레벨
    // LPF (로우패스 필터)
    LpfFreq,        // 컷오프 주파수
    LpfRes,         // 레조넌스
    LpfEnv,         // 엔벨로프 -> 컷오프 변조량
    LpfLfo,         // LFO -> 컷오프 변조량
    LpfTrack,       // 키보드 트래킹량
    // 공통
    Volume,         // 마스터 볼륨
    // PITCH (피치휠 라우팅)
    PitchDco,       // 피치휠 -> DCO 깊이
    PitchLfo,       // 피치휠 -> LFO 깊이
    // MOD (모드휠)
    ModLfo,         // 모드휠 -> LFO 깊이
    // GLIDE
    GlideTime,      // 포르타멘토 시간
    // ENV (ADSR)
    EnvA, EnvD, EnvS, EnvR,
    // VCA
    VcaLevel,       // VCA 레벨

    // ----- 여기서부터 스위치(이산값) -----
    DcoRange,       // 16' / 8' / 4'        (3 포지션)
    DcoPwmSource,   // PWM 소스: LFO / MAN   (2 포지션)
    DcoWaveSquare,  // 사각파 on/off        (2 포지션)
    DcoWaveSaw,     // 톱니파 on/off        (2 포지션)
    LpfEnvPolarity, // 엔벨로프 극성 +/-     (2 포지션)
    Voicing,        // POLY1 / POLY2 / UNISON (3 포지션)
    VcaShape,       // VCA: ENV / GATE       (2 포지션)

    Count
};

// 컨트롤이 따르는 값 변환 커브
enum class Curve : int {
    Linear,   // min..max 선형
    Exp       // 지수(기하). 주파수/시간처럼 청감상 지수적인 양에 사용 (min>0 필요)
};

// 컨트롤 1개의 메타데이터
struct ControlSpec {
    ControlId   id;
    float       min;          // 스위치는 0
    float       max;          // 스위치는 (포지션수-1)
    Curve       curve;
    float       smoothingMs;  // 아날로그 평활 시간(ms). 스위치는 0
    bool        isSwitch;     // true면 이산 스위치
    int         positions;    // 스위치 포지션 수(아날로그는 0)
    const char* name;
};

// 컨트롤 스펙 조회. (구현은 ControlIds.cpp)
const ControlSpec& getSpec(ControlId id);

// 아날로그 노브/페이더 개수 (스위치 직전까지) — 채널 매핑 루프에 사용
constexpr int kNumAnalogControls = static_cast<int>(ControlId::DcoRange);

} // namespace hw
