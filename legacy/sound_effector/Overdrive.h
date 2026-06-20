#pragma once

enum class OverdriveMode {
    kSoft,
    kHard
};

class Overdrive {
public:
    Overdrive();
    
    // 이펙터 설정 함수
    void setGain(float gain);
    void setMix(float mix);
    void setMode(OverdriveMode mode);

    // 샘플 처리 함수
    float process(float input);

private:
    float soft_clip(float in);
    float hard_clip(float in);

    float gGain;
    float gMix;
    OverdriveMode gMode;
};