#pragma once
#include <vector>

class Delay {
public:
    Delay();
    // 초기 설정: 최대 딜레이 시간(초)을 설정하여 메모리 확보
    void setup(float sampleRate, float maxDelayTime = 2.0f);
    
    // 파라미터 조절
    void setDelayTime(float seconds);
    void setFeedback(float fb);
    void setMix(float m);

    // 샘플 처리
    float process(float input);

private:
    std::vector<float> gBuffer;
    float gSampleRate;
    float gFeedback;
    float gMix;
    
    unsigned int gWritePtr;
    unsigned int gDelaySamples;
};