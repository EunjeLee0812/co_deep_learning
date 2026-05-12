#pragma once
#include <vector>

class Chorus {
public:
    Chorus();
    void setup(float sampleRate);
    
    // 파라미터 조절 함수
    void setRate(float hz);   // 흔들림 속도
    void setDepth(float ms);  // 흔들림 깊이
    void setMix(float m);     // 원음과 효과음 비율

    float process(float input);

private:
    float gSampleRate;
    std::vector<float> gBuffer;
    unsigned int gWritePtr;

    // LFO 관련 변수
    float gLfoPhase;
    float gRate;
    float gDepth;
    float gMix;
};