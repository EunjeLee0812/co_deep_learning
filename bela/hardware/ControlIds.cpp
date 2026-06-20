#include "ControlIds.h"

namespace hw {

// 각 컨트롤의 물리 범위/커브 정의.
// 범위·단위는 자유롭게 조정 가능. 여기 값이 곧 디스플레이/엔진이 받는 실제 단위다.
//                 id                     min     max    curve         smMs sw  pos name
static const ControlSpec kSpecs[] = {
    { ControlId::LfoRate,        0.05f,  20.0f, Curve::Exp,    30, false, 0, "LFO Rate(Hz)"   },
    { ControlId::LfoDelay,        0.0f,   5.0f, Curve::Linear, 30, false, 0, "LFO Delay(s)"   },
    { ControlId::DcoLfo,          0.0f,   1.0f, Curve::Linear, 20, false, 0, "DCO LFO"        },
    { ControlId::DcoPwm,          0.0f,   1.0f, Curve::Linear, 20, false, 0, "DCO PWM"        },
    { ControlId::DcoSub,          0.0f,   1.0f, Curve::Linear, 20, false, 0, "DCO Sub"        },
    { ControlId::DcoNoise,        0.0f,   1.0f, Curve::Linear, 20, false, 0, "DCO Noise"      },
    { ControlId::LpfFreq,        20.0f,20000.0f,Curve::Exp,    25, false, 0, "LPF Freq(Hz)"   },
    { ControlId::LpfRes,          0.0f,  0.95f, Curve::Linear, 25, false, 0, "LPF Res"        },
    { ControlId::LpfEnv,          0.0f,   1.0f, Curve::Linear, 25, false, 0, "LPF Env"        },
    { ControlId::LpfLfo,          0.0f,   1.0f, Curve::Linear, 25, false, 0, "LPF LFO"        },
    { ControlId::LpfTrack,        0.0f,   1.0f, Curve::Linear, 25, false, 0, "LPF Track"      },
    { ControlId::Volume,          0.0f,   1.0f, Curve::Linear, 30, false, 0, "Volume"         },
    { ControlId::PitchDco,        0.0f,   1.0f, Curve::Linear, 20, false, 0, "Pitch->DCO"     },
    { ControlId::PitchLfo,        0.0f,   1.0f, Curve::Linear, 20, false, 0, "Pitch->LFO"     },
    { ControlId::ModLfo,          0.0f,   1.0f, Curve::Linear, 20, false, 0, "Mod->LFO"       },
    { ControlId::GlideTime,       0.0f,   2.0f, Curve::Linear, 30, false, 0, "Glide(s)"       },
    { ControlId::EnvA,          0.001f,  10.0f, Curve::Exp,    20, false, 0, "Env Attack(s)"  },
    { ControlId::EnvD,          0.001f,  10.0f, Curve::Exp,    20, false, 0, "Env Decay(s)"   },
    { ControlId::EnvS,            0.0f,   1.0f, Curve::Linear, 20, false, 0, "Env Sustain"    },
    { ControlId::EnvR,          0.001f,  10.0f, Curve::Exp,    20, false, 0, "Env Release(s)" },
    { ControlId::VcaLevel,        0.0f,   1.0f, Curve::Linear, 20, false, 0, "VCA Level"      },
    // ----- 스위치 -----
    { ControlId::DcoRange,        0.0f,   2.0f, Curve::Linear,  0, true,  3, "DCO Range"      },
    { ControlId::DcoPwmSource,    0.0f,   1.0f, Curve::Linear,  0, true,  2, "PWM Source"     },
    { ControlId::DcoWaveSquare,   0.0f,   1.0f, Curve::Linear,  0, true,  2, "Wave Square"    },
    { ControlId::DcoWaveSaw,      0.0f,   1.0f, Curve::Linear,  0, true,  2, "Wave Saw"       },
    { ControlId::LpfEnvPolarity,  0.0f,   1.0f, Curve::Linear,  0, true,  2, "Env Polarity"   },
    { ControlId::Voicing,         0.0f,   2.0f, Curve::Linear,  0, true,  3, "Voicing"        },
    { ControlId::VcaShape,        0.0f,   1.0f, Curve::Linear,  0, true,  2, "VCA Shape"      },
};

static_assert(sizeof(kSpecs)/sizeof(kSpecs[0]) == static_cast<int>(ControlId::Count),
              "ControlSpec 테이블 개수가 ControlId 개수와 다릅니다");

const ControlSpec& getSpec(ControlId id) {
    return kSpecs[static_cast<int>(id)];
}

} // namespace hw
