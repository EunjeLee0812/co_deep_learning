#include "KarplusStrong.h"
#include <cmath>
#include <cstdlib>

KarplusStrong::KarplusStrong() : logicalSize(0), writePtr(0), readPtr(0), feedback(0.994f), isPlucking(false) {}

void KarplusStrong::setup(float sampleRate, float frequency, float feedback) {
    // 1. 넉넉하게 1초 치(44100 샘플)를 미리 할당해서 메모리 재할당을 원천 차단합니다.
    delayLine.assign((int)sampleRate, 0.0f);
    this->feedback = feedback;
    
    // 현재 음정에 맞는 논리적 크기 설정
    setFrequency(sampleRate, frequency);

    // 2. Wavetable(Excitation)도 충분한 크기로 미리 할당
    wavetable.resize(2000); 
    for(float &s : wavetable) {
        s = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }
}

void KarplusStrong::pluck() {
    isPlucking = true;
    readPtr = 0;
}

void KarplusStrong::setFrequency(float sampleRate, float frequency, float feedback) {
    // 3. 메모리 할당(assign) 없이 정수 값만 계산합니다.
    int newSize = (int)(sampleRate / frequency);
    if(newSize < 2) newSize = 2;
    if(newSize > (int)delayLine.size()) newSize = delayLine.size();
    
    this->logicalSize = newSize;
    if(feedback >= 0) this->feedback = feedback;
    
    // 포인터가 범위를 벗어나지 않게 조정
    if(writePtr >= logicalSize) writePtr = 0;
}

float KarplusStrong::process() {
    float input = 0;

    if(isPlucking) {
        input = wavetable[readPtr];
        readPtr++;
        // wavetable 끝까지 읽으면 멈춤
        if(readPtr >= (int)wavetable.size() || readPtr >= logicalSize) isPlucking = false;
    }

    float delayedSample = delayLine[writePtr];
    
    // 4. 나머지 연산(%) 대신 빠른 if문 사용
    int nextPtr = writePtr + 1;
    if(nextPtr >= logicalSize) nextPtr = 0; 
    
    // Karplus-Strong 핵심 수식
    // $$y[n] = x[n] + \alpha \frac{y[n-L] + y[n-L-1]}{2}$$
    float output = (delayedSample + delayLine[nextPtr]) * 0.5f * feedback;
    
    delayLine[writePtr] = input + output;
    writePtr = nextPtr;

    return output;
}