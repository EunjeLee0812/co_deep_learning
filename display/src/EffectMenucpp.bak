#include "EffectMenu.h"
#include <cstddef>

namespace disp {

// ===========================================================================
//  8개 이펙트 정의. 이미지(Effects Palette)의 배치 그대로:
//    상단: CHORUS, FLANGER, PHASER, COMPRESSOR
//    하단: GRAPHIC EQ, REVERB, DELAY, DISTORTION
//
//  paramId 오프셋 규약(effects/ 와 동일):
//    ModulationSet : Flanger 0~, Phaser 100~, Chorus 200~, 세트글로벌 300~
//    ChannelStrip  : Equalizer 0~, Compressor 100~, 스트립글로벌 200~
//
//  ※ hexId / hexComp / picNormal / picGlow / tN·nN·jN 은 Nextion Editor 의
//    실제 컴포넌트와 일치시켜야 한다. 아래 값은 예시이니 Editor 에 맞게 조정.
//  ※ 각 이펙트당 대표 파라미터 4개만 채움. 나머지는 TODO 로 확장.
// ===========================================================================
static const EffectDef kEffects[EffectMenu::kNumEffects] = {
  // -------- CHORUS (ModulationSet 의 Chorus, base 200) --------
  { "CHORUS", EngineTarget::Modulation, 1, "b0", 0, 1, {
      { "RATE",  EngineTarget::Modulation, 201, 0.05f, 8.0f, "t0","n0","j0" }, // Chorus::RateHz
      { "DEPTH", EngineTarget::Modulation, 202, 0.0f, 20.0f, "t1","n1","j1" }, // Chorus::DepthMs
      { "MIX",   EngineTarget::Modulation, 206, 0.0f,  1.0f, "t2","n2","j2" }, // Chorus::Mix
      { "FBACK", EngineTarget::Modulation, 204, 0.0f, 0.9f,  "t3","n3","j3" }, // Chorus::Feedback
    }, 4 },
  // -------- FLANGER (ModulationSet 의 Flanger, base 0) --------
  { "FLANGER", EngineTarget::Modulation, 2, "b1", 2, 3, {
      { "MANUAL",   EngineTarget::Modulation, 1, 0.2f, 4.0f,  "t0","n0","j0" }, // Flanger::ManualMs
      { "SWEEP",    EngineTarget::Modulation, 2, 0.0f, 100.f, "t1","n1","j1" }, // Flanger::SweepDepth
      { "FEEDBACK", EngineTarget::Modulation, 4, 0.0f, 10.0f, "t2","n2","j2" }, // Flanger::FeedbackAmt
      { "MIX",      EngineTarget::Modulation, 7, 0.0f, 1.0f,  "t3","n3","j3" }, // Flanger::MixDryThru
    }, 4 },
  // -------- PHASER (ModulationSet 의 Phaser, base 100) --------
  { "PHASER", EngineTarget::Modulation, 3, "b2", 4, 5, {
      { "MANUAL",   EngineTarget::Modulation, 101, 0.1f, 10.0f, "t0","n0","j0" }, // Phaser::ManualKHz
      { "STAGES",   EngineTarget::Modulation, 102, 4.0f, 12.0f, "t1","n1","j1" }, // Phaser::Stages
      { "SWEEP",    EngineTarget::Modulation, 103, 0.0f, 100.f, "t2","n2","j2" }, // Phaser::SweepDepth
      { "FEEDBACK", EngineTarget::Modulation, 105, 0.0f, 10.0f, "t3","n3","j3" }, // Phaser::FeedbackAmt
    }, 4 },
  // -------- COMPRESSOR (ChannelStrip 의 Compressor, base 100) --------
  { "COMPRESSOR", EngineTarget::ChannelStrip, 4, "b3", 6, 7, {
      { "THRESH", EngineTarget::ChannelStrip, 101, -50.0f, 0.0f, "t0","n0","j0" }, // Comp::ThresholdDb
      { "RATIO",  EngineTarget::ChannelStrip, 102, 1.0f, 20.0f,  "t1","n1","j1" }, // Comp::Ratio
      { "MIX",    EngineTarget::ChannelStrip, 105, 0.0f, 1.0f,   "t2","n2","j2" }, // Comp::DryWetMix
      { "MAKEUP", EngineTarget::ChannelStrip, 106, 0.0f, 24.0f,  "t3","n3","j3" }, // Comp::MakeupDb
    }, 4 },
  // -------- GRAPHIC EQ (ChannelStrip 의 Equalizer, base 0) --------
  { "GRAPHIC EQ", EngineTarget::ChannelStrip, 5, "b4", 8, 9, {
      { "LO",   EngineTarget::ChannelStrip, 4, -15.0f, 15.0f, "t0","n0","j0" }, // Eq::LoShelfGainDb
      { "MID",  EngineTarget::ChannelStrip, 6, -15.0f, 15.0f, "t1","n1","j1" }, // Eq::MidGainDb
      { "HI",   EngineTarget::ChannelStrip, 9, -15.0f, 15.0f, "t2","n2","j2" }, // Eq::HiShelfGainDb
      { "HPF",  EngineTarget::ChannelStrip, 1, 20.0f, 300.0f, "t3","n3","j3" }, // Eq::HpFreqHz
    }, 4 },
  // -------- REVERB --------
  { "REVERB", EngineTarget::Reverb, 6, "b5", 10, 11, {
      { "PRE-DELAY", EngineTarget::Reverb, 1, 0.0f, 250.0f, "t0","n0","j0" }, // Reverb::PreDelayMs
      { "ROOM",      EngineTarget::Reverb, 2, 0.0f, 1.0f,   "t1","n1","j1" }, // Reverb::RoomLevel
      { "HALL",      EngineTarget::Reverb, 3, 0.0f, 1.0f,   "t2","n2","j2" }, // Reverb::HallLevel
      { "WET/DRY",   EngineTarget::Reverb, 6, 0.0f, 1.0f,   "t3","n3","j3" }, // Reverb::WetDryMix
    }, 4 },
  // -------- DELAY --------
  { "DELAY", EngineTarget::Delay, 7, "b6", 12, 13, {
      { "L TIME",  EngineTarget::Delay, 10, 0.0f, 2000.0f, "t0","n0","j0" }, // Delay::LeftDelayMs
      { "R TIME",  EngineTarget::Delay, 20, 0.0f, 2000.0f, "t1","n1","j1" }, // Delay::RightDelayMs
      { "L FBACK", EngineTarget::Delay, 14, 0.0f, 1.1f,    "t2","n2","j2" }, // Delay::LeftFeedback
      { "BLUR",    EngineTarget::Delay, 4,  0.0f, 1.0f,    "t3","n3","j3" }, // Delay::DelayBlur
    }, 4 },
  // -------- DISTORTION --------
  { "DISTORTION", EngineTarget::Distortion, 8, "b7", 14, 15, {
      { "DRIVE",  EngineTarget::Distortion, 0, 0.0f, 1.0f,    "t0","n0","j0" }, // Distortion::Drive
      { "TONE",   EngineTarget::Distortion, 1, 500.0f, 12000.f,"t1","n1","j1" },// Distortion::ToneHz
      { "OUTPUT", EngineTarget::Distortion, 2, -24.0f, 6.0f,  "t2","n2","j2" }, // Distortion::OutputDb
      { "MIX",    EngineTarget::Distortion, 3, 0.0f, 1.0f,    "t3","n3","j3" }, // Distortion::Mix
    }, 4 },
};

const EffectDef& EffectMenu::effect(int index) const {
    if (index < 0) index = 0;
    if (index >= kNumEffects) index = kNumEffects - 1;
    return kEffects[index];
}

int EffectMenu::effectIndexForComponent(unsigned char componentId) const {
    for (int i = 0; i < kNumEffects; ++i)
        if (kEffects[i].hexId == componentId) return i;
    return -1;
}

} // namespace disp
