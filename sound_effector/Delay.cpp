#include "Delay.h"
#include <algorithm>

Delay::Delay() : gSampleRate(44100.0f), gFeedback(0.5f), gMix(0.3f), gWritePtr(0), gDelaySamples(4410) {}

void Delay::setup(float sampleRate, float maxDelayTime) {
    gSampleRate = sampleRate;
    // 최대 시간에 맞는 버퍼 크기 할당
    gBuffer.assign((unsigned int)(sampleRate * maxDelayTime), 0.0f);
    gWritePtr = 0;
    // 기본 딜레이 시간을 0.3초로 설정
    setDelayTime(0.3f);
}

void Delay::setDelayTime(float seconds) {
    gDelaySamples = (unsigned int)(seconds * gSampleRate);
    // 버퍼 크기를 넘지 않도록 제한
    if (gDelaySamples >= gBuffer.size()) {
        gDelaySamples = gBuffer.size() - 1;
    }
}

void Delay::setFeedback(float fb) {
    // 0.0 ~ 0.95 사이로 제한 (1.0 이상이면 발진 위험)
    if (fb < 0.0f) gFeedback = 0.0f;
    else if (fb > 0.95f) gFeedback = 0.95f;
    else gFeedback = fb;
}

void Delay::setMix(float m) {
    if (m < 0.0f) gMix = 0.0f;
    else if (m > 1.0f) gMix = 1.0f;
    else gMix = m;
}

float Delay::process(float input) {
    // 1. 현재 쓰기 위치에서 딜레이된 샘플 읽기
    // (현재 위치 - 딜레이 샘플 수 + 버퍼 크기) % 버퍼 크기
    int readPtr = (int)gWritePtr - (int)gDelaySamples;
    if (readPtr < 0) readPtr += gBuffer.size();
    
    float delayedSample = gBuffer[readPtr];

    // 2. 피드백을 포함하여 버퍼에 새로운 샘플 쓰기
    gBuffer[gWritePtr] = input + (delayedSample * gFeedback);

    // 3. 쓰기 포인터 이동
    gWritePtr = (gWritePtr + 1) % gBuffer.size();

    // 4. 원음(Dry)과 지연음(Wet) 믹스하여 출력
    return (input * (1.0f - gMix)) + (delayedSample * gMix);
}