#pragma once
#include <vector>

class KarplusStrong {
public:
    KarplusStrong();
    // 초기화 시 최대 버퍼를 잡기 위해 주파수를 받습니다.
    void setup(float sampleRate, float frequency, float feedback = 0.994f);
    void pluck();
    // 런타임에 호출되는 이 함수에서는 메모리 할당을 하지 않습니다.
    void setFrequency(float sampleRate, float frequency, float feedback = -1.0f);
    float process();

private:
    std::vector<float> delayLine;
    std::vector<float> wavetable;
    
    int logicalSize;   // 현재 음정에 해당하는 실제 딜레이 길이
    int writePtr;
    int readPtr;
    float feedback;
    bool isPlucking;
};